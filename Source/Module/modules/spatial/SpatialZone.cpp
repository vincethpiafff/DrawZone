/*
  ==============================================================================

	SpatialZone.cpp
	Created: 26 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

ZonePoint::ZonePoint(int index) :
	ControllableContainer("Point " + String(index + 1))
{
	position = addPoint2DParameter("Position", "Position of this point in the box of the zone (Center + Size), from 0,0 (top left) to 1,1 (bottom right)");
	position->setBounds(0, 0, 1, 1);
	position->extendedEditorInvertY = true;

	interpolation = addEnumParameter("Interpolation", "Linear : straight segments on both sides. Quadratic : one handle (Handle Out, mirrored toward the previous point) for a smooth curve. Cubic : two independent handles (Handle In, Handle Out)");
	interpolation->addOption("Linear", LINEAR)->addOption("Quadratic", QUADRATIC)->addOption("Cubic", CUBIC);

	handleIn = addPoint2DParameter("Handle In", "Cubic only : curve control toward the previous point, in meters from this point");
	handleIn->setBounds(-100, -100, 100, 100);
	handleOut = addPoint2DParameter("Handle Out", "Quadratic and Cubic : curve control toward the next point, in meters from this point (mirrored toward the previous point for Quadratic)");
	handleOut->setBounds(-100, -100, 100, 100);
}

ZonePoint::Interpolation ZonePoint::getInterpolation() const
{
	return interpolation->getValueDataAsEnum<Interpolation>();
}

Point<float> ZonePoint::getInOffset() const
{
	switch (getInterpolation())
	{
	case QUADRATIC: return -handleOut->getPoint();
	case CUBIC: return handleIn->getPoint();
	default: return Point<float>();
	}
}

Point<float> ZonePoint::getOutOffset() const
{
	return getInterpolation() == LINEAR ? Point<float>() : handleOut->getPoint();
}

SpatialZone::SpatialZone(var params) :
	BaseItem("Zone 1"),
	pointsCC("Points"),
	zoneValues("Zone 1")
{
	shape = addEnumParameter("Shape", "Rectangle, ellipse filling the rectangle given by Center and Size (a circle when width = height), or free shape drawn point by point (see Points)");
	shape->addOption("Rectangle", RECTANGLE)->addOption("Ellipse", ELLIPSE)->addOption("Free Shape", FREE_SHAPE);

	center = addPoint2DParameter("Center", "Center of the zone, in meters from the top left corner of the scene");
	center->setDefaultPoint(4, 2.25f);
	center->extendedEditorInvertY = true; //2D editor : y goes down, like the scene (origin top left)

	size = addPoint2DParameter("Size", "Width and height of the zone, in meters");
	size->setDefaultPoint(2, 2);

	capacity = addIntParameter("Capacity", "Number of people for which this zone is considered full (Occupancy = 100 %)", 4, 1);

	exitMargin = addFloatParameter("Exit Margin", "Anti-flicker : once inside, a person only leaves this zone when further than this distance outside its border, in meters", .1f, 0);
	exitDelay = addFloatParameter("Exit Delay", "Anti-flicker : a person who leaves this zone or is lost by the tracking is still counted inside for this time, in seconds. Coming back meanwhile triggers neither Exited nor Entered", .3f, 0);

	hiddenInMonitor = addBoolParameter("Hidden", "Hide this zone in the Tracking Monitor (it still detects people)", false);

	pointsCC.includeInRecursiveSave = false; //saved by getJSONData, the number of points varies
	addChildControllableContainer(&pointsCC);

	setSceneSize(Point<float>());

	presence = zoneValues.addBoolParameter("Presence", "Is at least one person in this zone ?", false);
	presence->setControllableFeedbackOnly(true);
	personCount = zoneValues.addIntParameter("Person Count", "Number of people in this zone", 0, 0);
	personCount->setControllableFeedbackOnly(true);
	entered = zoneValues.addTrigger("Entered", "Triggered each time a person enters this zone");
	entered->setControllableFeedbackOnly(true);
	exited = zoneValues.addTrigger("Exited", "Triggered each time a person leaves this zone or is lost by the tracking inside it");
	exited->setControllableFeedbackOnly(true);
	lastEnteredId = zoneValues.addIntParameter("Last Entered Id", "Id of the last person who entered this zone (-1 = nobody yet)", -1, -1);
	lastEnteredId->setControllableFeedbackOnly(true);
	presenceTime = zoneValues.addFloatParameter("Presence Time", "Seconds since this zone became occupied without interruption (0 when empty)", 0, 0);
	presenceTime->setControllableFeedbackOnly(true);
	occupancy = zoneValues.addFloatParameter("Occupancy", "Person Count / Capacity, from 0 (empty) to 1 (full, 100 %)", 0, 0, 1);
	occupancy->setControllableFeedbackOnly(true);
	averagePosition = zoneValues.addPoint2DParameter("Average Position", "Barycenter of the people in this zone, from 0,0 (top left of the zone) to 1,1 (bottom right). Kept when the zone gets empty");
	averagePosition->setBounds(0, 0, 1, 1);
	averagePosition->setPoint(.5f, .5f);
	averagePosition->setControllableFeedbackOnly(true);
	averageDistanceToCenter = zoneValues.addFloatParameter("Average Distance To Center", "Distance between the barycenter of the people in this zone and its center, in meters. Kept when the zone gets empty", 0, 0);
	averageDistanceToCenter->setControllableFeedbackOnly(true);

	//color used to draw the zone in the Tracking Monitor
	Random r;
	setHasCustomColor(true);
	itemColor->setDefaultValue(Colour::fromHSV(r.nextFloat(), .7f, 1, 1));
}

SpatialZone::~SpatialZone()
{
	for (auto& fp : freePoints) pointsCC.removeChildControllableContainer(fp);
}

//the zone must stay inside the scene sent by the tracking system
//(finite bounds also stop the Inspector drag from jumping to millions of meters)
void SpatialZone::setSceneSize(Point<float> sceneSize)
{
	if (sceneSize.x <= 0 || sceneSize.y <= 0) sceneSize = Point<float>(maxMeters, maxMeters); //scene not received yet

	center->setBounds(0, 0, sceneSize.x, sceneSize.y);
	size->setBounds(0, 0, sceneSize.x, sceneSize.y);
}

bool SpatialZone::contains(Point<float> positionMeters, float margin) const
{
	if (isFreeShape())
	{
		if (freePoints.size() < 3) return false;
		Path outline = getOutline();
		if (outline.contains(positionMeters, .001f)) return true;
		if (margin <= 0) return false;
		Point<float> nearest;
		outline.getNearestPoint(positionMeters, nearest, AffineTransform(), .001f);
		return nearest.getDistanceFrom(positionMeters) <= margin;
	}

	Point<float> c = center->getPoint();
	Point<float> halfSize = size->getPoint() / 2 + Point<float>(margin, margin);
	Point<float> d = positionMeters - c;
	if (isEllipse())
	{
		if (halfSize.x <= 0 || halfSize.y <= 0) return false;
		return (d.x * d.x) / (halfSize.x * halfSize.x) + (d.y * d.y) / (halfSize.y * halfSize.y) <= 1;
	}
	return std::abs(d.x) <= halfSize.x && std::abs(d.y) <= halfSize.y;
}

bool SpatialZone::isEllipse() const
{
	return shape->getValueDataAsEnum<Shape>() == ELLIPSE;
}

bool SpatialZone::isFreeShape() const
{
	return shape->getValueDataAsEnum<Shape>() == FREE_SHAPE;
}

Rectangle<float> SpatialZone::getBounds() const
{
	Point<float> s = size->getPoint();
	return Rectangle<float>(s.x, s.y).withCentre(center->getPoint());
}

Array<Point<float>> SpatialZone::getFreePointsMeters() const
{
	Rectangle<float> b = getBounds();
	Array<Point<float>> result;
	for (auto& fp : freePoints)
	{
		Point<float> rel = fp->position->getPoint();
		result.add(Point<float>(b.getX() + rel.x * b.getWidth(), b.getY() + rel.y * b.getHeight()));
	}
	return result;
}

void SpatialZone::setFreePointsMeters(const Array<Point<float>>& meters)
{
	if (meters.isEmpty()) return;

	Rectangle<float> b = Rectangle<float>::findAreaContainingPoints(meters.begin(), meters.size());
	size->setPoint(b.getWidth(), b.getHeight());
	center->setPoint(b.getCentre());

	Array<Point<float>> relative;
	for (auto& m : meters)
	{
		relative.add(Point<float>(b.getWidth() > 0 ? (m.x - b.getX()) / b.getWidth() : .5f,
			b.getHeight() > 0 ? (m.y - b.getY()) / b.getHeight() : .5f));
	}
	setFreePoints(relative);
}

void SpatialZone::setFreePoints(const Array<Point<float>>& relative)
{
	//same number of points : only move them (keeps the Inspector and the undo history simple)
	if (relative.size() != freePoints.size())
	{
		for (auto& fp : freePoints) pointsCC.removeChildControllableContainer(fp);
		freePoints.clear();
		for (int i = 0; i < relative.size(); i++)
		{
			ZonePoint* fp = freePoints.add(new ZonePoint(i));
			pointsCC.addChildControllableContainer(fp);
		}
	}

	for (int i = 0; i < relative.size(); i++) freePoints[i]->position->setPoint(relative[i]);
}

Path SpatialZone::getOutline() const
{
	Path p;
	Array<Point<float>> meters = getFreePointsMeters();
	int n = meters.size();
	if (n == 0) return p;

	//each segment uses the out handle of its start point and the in handle of its end point
	p.startNewSubPath(meters[0]);
	for (int i = 0; i < n; i++)
	{
		int next = (i + 1) % n;
		Point<float> outOffset = freePoints[i]->getOutOffset();
		Point<float> inOffset = freePoints[next]->getInOffset();
		bool hasOut = !outOffset.isOrigin(), hasIn = !inOffset.isOrigin();

		if (hasOut && hasIn) p.cubicTo(meters[i] + outOffset, meters[next] + inOffset, meters[next]);
		else if (hasOut) p.quadraticTo(meters[i] + outOffset, meters[next]);
		else if (hasIn) p.quadraticTo(meters[next] + inOffset, meters[next]);
		else p.lineTo(meters[next]);
	}
	p.closeSubPath();
	return p;
}

Point<float> SpatialZone::getHandleMeters(int index, bool out) const
{
	if (!isPositiveAndBelow(index, freePoints.size())) return Point<float>();
	Rectangle<float> b = getBounds();
	Point<float> rel = freePoints[index]->position->getPoint();
	Point<float> pos(b.getX() + rel.x * b.getWidth(), b.getY() + rel.y * b.getHeight());
	return pos + (out ? freePoints[index]->getOutOffset() : freePoints[index]->getInOffset());
}

void SpatialZone::updateFromPersons(const Array<TrackedPerson>& persons, Point<float> sceneSize)
{
	uint32 now = Time::getMillisecondCounter();
	Array<int> ids;
	Point<float> sum;	// meters, people really inside only (not the ones kept by the exit delay)
	int sumCount = 0;
	if (enabled->boolValue())
	{
		//anti-flicker : the border is pushed out by the exit margin for people already inside
		for (auto& p : persons)
		{
			Point<float> meters(p.position.x * sceneSize.x, p.position.y * sceneSize.y);
			if (!contains(meters, idsInside.contains(p.id) ? exitMargin->floatValue() : 0)) continue;
			ids.add(p.id);
			lastInsideMs.set(p.id, now);
			sum += meters;
			sumCount++;
		}

		//anti-flicker : people who just left or were lost are still counted during the exit delay
		uint32 delayMs = (uint32)(exitDelay->floatValue() * 1000);
		for (int id : idsInside)
		{
			if (!ids.contains(id) && now - lastInsideMs[id] < delayMs) ids.add(id);
		}
	}

	for (int id : idsInside)
	{
		if (!ids.contains(id)) lastInsideMs.remove(id);
	}

	if (sumCount > 0)
	{
		Point<float> average = sum / (float)sumCount;
		Rectangle<float> b = getBounds();
		averagePosition->setPoint(b.getWidth() > 0 ? (average.x - b.getX()) / b.getWidth() : .5f,
			b.getHeight() > 0 ? (average.y - b.getY()) / b.getHeight() : .5f);
		averageDistanceToCenter->setValue(average.getDistanceFrom(b.getCentre()));
	}

	//count first, so the triggers below already see the new values
	if (ids.size() > 0 && idsInside.isEmpty()) presenceStartMs = now;
	presenceTime->setValue(ids.size() > 0 ? (now - presenceStartMs) / 1000.0f : 0);

	personCount->setValue(ids.size());
	presence->setValue(ids.size() > 0);
	occupancy->setValue(jmin(1.0f, ids.size() / (float)jmax(1, capacity->intValue())));

	for (int id : idsInside)
	{
		if (!ids.contains(id)) exited->trigger();
	}

	for (int id : ids)
	{
		if (idsInside.contains(id)) continue;
		lastEnteredId->setValue(id);
		entered->trigger();
	}

	idsInside = ids;
}

void SpatialZone::onContainerNiceNameChanged()
{
	BaseItem::onContainerNiceNameChanged();
	zoneValues.setNiceName(niceName);
}

void SpatialZone::onContainerParameterChangedInternal(Parameter* p)
{
	//switched to free shape without points yet : start from the corners of the box
	if (p == shape && isFreeShape() && freePoints.isEmpty() && !isCurrentlyLoadingData)
	{
		setFreePoints({ Point<float>(0, 0), Point<float>(1, 0), Point<float>(1, 1), Point<float>(0, 1) });
	}
}

var SpatialZone::getJSONData(bool includeNonOverriden)
{
	var data = BaseItem::getJSONData(includeNonOverriden);
	if (!freePoints.isEmpty())
	{
		var pointsData;
		for (auto& fp : freePoints)
		{
			var pData;
			Point<float> pos = fp->position->getPoint();
			pData.append(pos.x);
			pData.append(pos.y);
			if (fp->getInterpolation() != ZonePoint::LINEAR)
			{
				Point<float> hIn = fp->handleIn->getPoint(), hOut = fp->handleOut->getPoint();
				pData.append((int)fp->getInterpolation());
				pData.append(hIn.x);
				pData.append(hIn.y);
				pData.append(hOut.x);
				pData.append(hOut.y);
			}
			pointsData.append(pData);
		}
		data.getDynamicObject()->setProperty("freePoints", pointsData);
	}
	return data;
}

void SpatialZone::loadJSONDataInternal(var data)
{
	BaseItem::loadJSONDataInternal(data);

	//each point : x, y, then interpolation, handle in x, y, handle out x, y when it is a curve
	Array<Point<float>> relative;
	Array<var>* pointsData = data.getProperty("freePoints", var()).getArray();
	if (pointsData != nullptr)
	{
		for (auto& pData : *pointsData) relative.add(Point<float>((float)pData[0], (float)pData[1]));
	}
	setFreePoints(relative);

	for (int i = 0; i < freePoints.size(); i++)
	{
		var pData = (*pointsData)[i];
		if (pData.size() < 7) continue;
		freePoints[i]->interpolation->setValueWithData((int)pData[2]);
		freePoints[i]->handleIn->setPoint((float)pData[3], (float)pData[4]);
		freePoints[i]->handleOut->setPoint((float)pData[5], (float)pData[6]);
	}
}
