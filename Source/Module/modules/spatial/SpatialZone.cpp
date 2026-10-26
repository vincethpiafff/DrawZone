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

	setSceneSize(Point<float>());

	presence = zoneValues.addBoolParameter("Presence", "Is at least one person in this zone ?", false);
	presence->setControllableFeedbackOnly(true);
	personCount = zoneValues.addIntParameter("Person Count", "Number of people in this zone", 0, 0);
	personCount->setControllableFeedbackOnly(true);

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

void SpatialZone::updateFromPersons(const Array<TrackedPerson>& persons, Point<float> sceneSize)
{
	int count = 0;
	if (enabled->boolValue())
	{
		for (auto& p : persons)
		{
			if (contains(Point<float>(p.position.x * sceneSize.x, p.position.y * sceneSize.y))) count++;
		}
	}

	personCount->setValue(count);
	presence->setValue(count > 0);
}

void SpatialZone::onContainerNiceNameChanged()
{
	BaseItem::onContainerNiceNameChanged();
	zoneValues.setNiceName(niceName);
}
