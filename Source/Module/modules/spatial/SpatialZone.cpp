/*
  ==============================================================================

	SpatialZone.cpp
	Created: 26 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialZone::SpatialZone(var params) :
	BaseItem("Zone 1"),
	zoneValues("Zone 1")
{
	center = addPoint2DParameter("Center", "Center of the zone, in meters from the top left corner of the scene");
	center->setDefaultPoint(4, 2.25f);
	center->extendedEditorInvertY = true; //2D editor : y goes down, like the scene (origin top left)

	size = addPoint2DParameter("Size", "Width and height of the zone, in meters");
	size->setDefaultPoint(2, 2);

	capacity = addIntParameter("Capacity", "Number of people for which this zone is considered full (Occupancy = 100 %)", 4, 1);

	hiddenInMonitor = addBoolParameter("Hidden", "Hide this zone in the Tracking Monitor (it still detects people)", false);

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
}

//the zone must stay inside the scene sent by the tracking system
//(finite bounds also stop the Inspector drag from jumping to millions of meters)
void SpatialZone::setSceneSize(Point<float> sceneSize)
{
	if (sceneSize.x <= 0 || sceneSize.y <= 0) sceneSize = Point<float>(maxMeters, maxMeters); //scene not received yet

	center->setBounds(0, 0, sceneSize.x, sceneSize.y);
	size->setBounds(0, 0, sceneSize.x, sceneSize.y);
}

bool SpatialZone::contains(Point<float> positionMeters) const
{
	Point<float> c = center->getPoint();
	Point<float> halfSize = size->getPoint() / 2;
	return std::abs(positionMeters.x - c.x) <= halfSize.x && std::abs(positionMeters.y - c.y) <= halfSize.y;
}

Rectangle<float> SpatialZone::getBounds() const
{
	Point<float> s = size->getPoint();
	return Rectangle<float>(s.x, s.y).withCentre(center->getPoint());
}

void SpatialZone::updateFromPersons(const Array<TrackedPerson>& persons, Point<float> sceneSize)
{
	Array<int> ids;
	Point<float> sum;	// meters
	if (enabled->boolValue())
	{
		for (auto& p : persons)
		{
			Point<float> meters(p.position.x * sceneSize.x, p.position.y * sceneSize.y);
			if (!contains(meters)) continue;
			ids.add(p.id);
			sum += meters;
		}
	}

	if (ids.size() > 0)
	{
		Point<float> average = sum / (float)ids.size();
		Rectangle<float> b = getBounds();
		averagePosition->setPoint(b.getWidth() > 0 ? (average.x - b.getX()) / b.getWidth() : .5f,
			b.getHeight() > 0 ? (average.y - b.getY()) / b.getHeight() : .5f);
		averageDistanceToCenter->setValue(average.getDistanceFrom(b.getCentre()));
	}

	//count first, so the triggers below already see the new values
	uint32 now = Time::getMillisecondCounter();
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
