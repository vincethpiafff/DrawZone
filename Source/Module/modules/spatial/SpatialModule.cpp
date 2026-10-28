/*
  ==============================================================================

	SpatialModule.cpp
	Created: 24 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#include "TrackingData.cpp"
#include "SpatialZone.cpp"
#include "PlayerSlots.cpp"
#include "ui/TopViewRenderer.cpp"
#include "SpatialVideoOutput.cpp"
#include "ui/TrackingMonitorPanel.cpp"

SpatialModule::SpatialModule() :
	OSCModule(getDefaultTypeString(), 12000, 9000),
	zoneManager("Zones"),
	zonesValuesCC("Zones"),
	peopleValuesCC("People"),
	videoOutput(this)
{
	personCount = valuesCC.addIntParameter("Person Count", "Number of tracked people currently in the scene", 0, 0);
	personCount->setControllableFeedbackOnly(true);

	//zones are saved with the module parameters (recursive save)
	moduleParams.addChildControllableContainer(&zoneManager);
	zoneManager.selectItemWhenCreated = false;
	zoneManager.addBaseManagerListener(this);

	valuesCC.addChildControllableContainer(&zonesValuesCC);
	peopleValuesCC.saveAndLoadRecursiveData = false; //live data, nothing to save
	valuesCC.addChildControllableContainer(&peopleValuesCC);

	moduleParams.addChildControllableContainer(&playerSlots);
	moduleParams.addChildControllableContainer(&videoOutput);
	valuesCC.addChildControllableContainer(&playerSlots.slotsValues);

	startTimerHz(zoneUpdateRateHz);
}

SpatialModule::~SpatialModule()
{
	stopTimer();
	zoneManager.removeBaseManagerListener(this);
}

void SpatialModule::processMessageInternal(const OSCMessage& msg)
{
	trackingData.countMessage();

	//Augmenta OSC protocol v2, see docs/drawzone/notes/augmenta-protocol.md
	String address = msg.getAddressPattern().toString();

	if (address == "/scene")
	{
		if (msg.size() < 4) return;
		trackingData.setScene(OSCHelpers::getIntArg(msg[0]), Point<float>(OSCHelpers::getFloatArg(msg[2]), OSCHelpers::getFloatArg(msg[3])));
		trackingData.removeStalePersons(personTimeoutMs);
	}
	else if (address == "/object/enter" || address == "/object/update")
	{
		processAugmentaPerson(msg);
	}
	else if (address == "/object/leave")
	{
		if (msg.size() < 2) return;
		trackingData.removePerson(OSCHelpers::getIntArg(msg[1]));
	}
	else
	{
		return;
	}

	personCount->setValue(trackingData.getNumPersons());
}

void SpatialModule::processAugmentaPerson(const OSCMessage& msg)
{
	if (msg.size() < 15) return;

	TrackedPerson p;
	p.id = OSCHelpers::getIntArg(msg[1]);
	p.age = OSCHelpers::getFloatArg(msg[3]);
	p.position = Point<float>(OSCHelpers::getFloatArg(msg[4]), OSCHelpers::getFloatArg(msg[5]));
	p.velocity = Point<float>(OSCHelpers::getFloatArg(msg[6]), OSCHelpers::getFloatArg(msg[7]));
	p.orientation = OSCHelpers::getFloatArg(msg[8]);
	p.height = OSCHelpers::getFloatArg(msg[14]);
	p.lastUpdateMs = Time::getMillisecondCounter();

	trackingData.updatePerson(p);
}

void SpatialModule::updateZones()
{
	TrackingData::Snapshot s = trackingData.getSnapshot();

	if (s.sceneSize != zonesSceneSize)
	{
		zonesSceneSize = s.sceneSize;
		for (auto& z : zoneManager.items) z->setSceneSize(zonesSceneSize);
	}

	for (auto& z : zoneManager.items) z->updateFromPersons(s.persons, s.sceneSize);

	playerSlots.update(s.persons, s.sceneSize); //before the people, which show their slot
	updatePeople(s);
}

void SpatialModule::updatePeople(const TrackingData::Snapshot& s)
{
	//people who left : their values disappear
	for (auto it = people.begin(); it != people.end();)
	{
		bool stillTracked = false;
		for (auto& p : s.persons) if (p.id == it->first) { stillTracked = true; break; }

		if (stillTracked) it++;
		else
		{
			peopleValuesCC.removeChildControllableContainer(it->second); //owned : deleted here
			it = people.erase(it);
		}
	}

	for (auto& p : s.persons)
	{
		SpatialPerson* sp;
		auto it = people.find(p.id);
		if (it != people.end()) sp = it->second;
		else
		{
			sp = new SpatialPerson(p.id);
			peopleValuesCC.addChildControllableContainer(sp, true);
			people[p.id] = sp;
		}

		Point<float> meters(p.position.x * s.sceneSize.x, p.position.y * s.sceneSize.y);
		Array<SpatialZone*> zones;
		for (auto& z : zoneManager.items) if (z->idsInside.contains(p.id)) zones.add(z); //same membership as the zone (with its anti-flicker)
		sp->update(meters, zones);

		int slotIndex = playerSlots.getSlotIndexForPerson(p.id);
		sp->updateTracking(p, slotIndex >= 0 ? playerSlots.slots[slotIndex]->niceName : String());
	}
}

SpatialPerson::SpatialPerson(int id) :
	ControllableContainer("Person " + String(id))
{
	personId = addIntParameter("Id", "Id sent by the tracking system", id);
	personId->setControllableFeedbackOnly(true);
	position = addPoint2DParameter("Position", "Position in meters, from the top left corner of the scene");
	position->setControllableFeedbackOnly(true);
	zoneName = addStringParameter("Zone", "Zone the person is in, \"A + B\" where zones overlap, empty outside any zone", "");
	zoneName->setControllableFeedbackOnly(true);
	relativePosition = addPoint2DParameter("Relative Position", "Position in the zone, from 0,0 (top left) to 1,1 (bottom right). Where zones overlap, position in the overlapping area. -1,-1 outside any zone");
	relativePosition->setBounds(-1, -1, 1, 1);
	relativePosition->setPoint(-1, -1);
	relativePosition->setControllableFeedbackOnly(true);
	slotName = addStringParameter("Slot", "Player slot of the person, empty without a slot", "");
	slotName->setControllableFeedbackOnly(true);

	velocity = addPoint2DParameter("Velocity", "Velocity as sent by the tracking system");
	velocity->setControllableFeedbackOnly(true);
	orientation = addFloatParameter("Orientation", "Orientation as sent by the tracking system, in degrees", 0);
	orientation->setControllableFeedbackOnly(true);
	height = addFloatParameter("Height", "Height of the person as sent by the tracking system, in meters", 0);
	height->setControllableFeedbackOnly(true);
	age = addFloatParameter("Age", "Seconds since the person entered the scene, as sent by the tracking system", 0);
	age->setControllableFeedbackOnly(true);
}

void SpatialPerson::updateTracking(const TrackedPerson& p, const String& slot)
{
	slotName->setValue(slot);
	velocity->setPoint(p.velocity);
	orientation->setValue(p.orientation);
	height->setValue(p.height);
	age->setValue(p.age);
}

void SpatialPerson::update(Point<float> positionMeters, const Array<SpatialZone*>& zones)
{
	position->setPoint(positionMeters);

	if (zones.isEmpty())
	{
		zoneName->setValue("");
		relativePosition->setPoint(-1, -1);
		return;
	}

	//area shared by all the zones the person is in (the zone itself when only one)
	StringArray names;
	Rectangle<float> area = zones[0]->getBounds();
	for (auto& z : zones)
	{
		names.add(z->niceName);
		area = area.getIntersection(z->getBounds());
	}

	zoneName->setValue(names.joinIntoString(" + "));
	//clamped : with the exit margin, a person can be slightly outside the area
	relativePosition->setPoint(area.getWidth() > 0 ? jlimit(0.0f, 1.0f, (positionMeters.x - area.getX()) / area.getWidth()) : .5f,
		area.getHeight() > 0 ? jlimit(0.0f, 1.0f, (positionMeters.y - area.getY()) / area.getHeight()) : .5f);
}

void SpatialModule::timerCallback()
{
	if (trackingData.removeAllIfSilent(sourceTimeoutMs)) personCount->setValue(0);
	updateZones();
}

void SpatialModule::itemAdded(SpatialZone* zone)
{
	zonesValuesCC.addChildControllableContainer(&zone->zoneValues);
	zone->setSceneSize(zonesSceneSize);

	//a brand new zone starts at the center of the scene (not a duplicated or loaded one)
	bool isLoading = isCurrentlyLoadingData || zoneManager.isCurrentlyLoadingData;
	bool isNew = !zone->center->isOverriden;
	if (!isLoading && isNew && zonesSceneSize.x > 0 && zonesSceneSize.y > 0) zone->center->setPoint(zonesSceneSize / 2);
}

void SpatialModule::itemsAdded(Array<SpatialZone*> zones)
{
	for (auto& z : zones)
	{
		zonesValuesCC.addChildControllableContainer(&z->zoneValues);
		z->setSceneSize(zonesSceneSize);
	}
}

void SpatialModule::itemRemoved(SpatialZone* zone)
{
	zonesValuesCC.removeChildControllableContainer(&zone->zoneValues);
}

void SpatialModule::itemsRemoved(Array<SpatialZone*> zones)
{
	for (auto& z : zones) zonesValuesCC.removeChildControllableContainer(&z->zoneValues);
}
