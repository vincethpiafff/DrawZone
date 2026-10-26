/*
  ==============================================================================

	SpatialModule.cpp
	Created: 24 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#include "TrackingData.cpp"
#include "SpatialZone.cpp"
#include "ui/TrackingMonitorPanel.cpp"

SpatialModule::SpatialModule() :
	OSCModule(getDefaultTypeString(), 12000, 9000),
	zoneManager("Zones"),
	zonesValuesCC("Zones")
{
	personCount = valuesCC.addIntParameter("Person Count", "Number of tracked people currently in the scene", 0, 0);
	personCount->setControllableFeedbackOnly(true);

	//zones are saved with the module parameters (recursive save)
	moduleParams.addChildControllableContainer(&zoneManager);
	zoneManager.selectItemWhenCreated = false;
	zoneManager.addBaseManagerListener(this);

	valuesCC.addChildControllableContainer(&zonesValuesCC);

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
