/*
  ==============================================================================

	SpatialModule.cpp
	Created: 24 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#include "TrackingData.cpp"
#include "SpatialSource.cpp"
#include "SpatialZone.cpp"
#include "PlayerSlots.cpp"
#include "ui/TopViewRenderer.cpp"
#include "SpatialVideoOutput.cpp"
#include "ui/TrackingMonitorPanel.cpp"

SpatialModule::SpatialModule() :
	OSCModule(getDefaultTypeString(), 12000, 9000),
	sceneCC("Scene"),
	zoneManager("Zones", SpatialSection::ZONES),
	zonesValuesCC("Zones"),
	peopleValuesCC("People"),
	videoOutput(this)
{
	personCount = valuesCC.addIntParameter("Person Count", "Number of tracked people currently in the scene", 0, 0);
	personCount->setControllableFeedbackOnly(true);

	sceneSizeMode = sceneCC.addEnumParameter("Size Mode", "Auto : the scene is the Augmenta scene (/scene message). Manual : the scene size is set below, for sources without a scene (PSN, TUIO) or to frame several sources");
	sceneSizeMode->addOption("Auto (Augmenta)", SCENE_AUTO)->addOption("Manual", SCENE_MANUAL);
	sceneSizeManual = sceneCC.addPoint2DParameter("Size", "Manual : width and height of the scene, in meters");
	sceneSizeManual->setBounds(.1f, .1f, 1000, 1000);
	sceneSizeManual->setDefaultPoint(10, 6);

	//a new module starts with an Augmenta source on its OSC input (sessions without sources too : nothing to load)
	moduleParams.addChildControllableContainer(&sourceManager);
	sourceManager.addBaseManagerListener(this);
	sourceManager.addItem(new AugmentaSource(), var(), false);

	moduleParams.addChildControllableContainer(&sceneCC);
	updateSceneSize();

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

	setupInspectorSections();

	startTimerHz(zoneUpdateRateHz);
}

SpatialModule::~SpatialModule()
{
	stopTimer();
	zoneManager.removeBaseManagerListener(this);
	sourceManager.removeBaseManagerListener(this);
}

void SpatialModule::processMessageInternal(const OSCMessage& msg)
{
	//the OSC input feeds the first enabled Augmenta source ; locked : a source is not deleted while it parses
	const ScopedLock sl(sourceManager.items.getLock());
	if (AugmentaSource* a = sourceManager.getAugmentaSource()) a->processMessage(msg, this);
}

void SpatialModule::updateSceneSize()
{
	sceneAuto = sceneSizeMode->getValueDataAsEnum<SceneSizeMode>() == SCENE_AUTO;
	sceneSizeManual->setEnabled(!sceneAuto);
	if (!sceneAuto) trackingData.setSceneSize(sceneSizeManual->getPoint());
	else
	{
		//back to the Augmenta scene, if one was received
		const ScopedLock sl(sourceManager.items.getLock());
		AugmentaSource* a = sourceManager.getAugmentaSource();
		trackingData.setSceneSize(a != nullptr ? a->augmentaSceneSize : Point<float>());
	}
}

void SpatialModule::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	OSCModule::onControllableFeedbackUpdateInternal(cc, c);
	if (c == sceneSizeMode || c == sceneSizeManual) updateSceneSize();
}

void SpatialModule::updateZones()
{
	TrackingData::Snapshot s = trackingData.getSnapshot();

	if (s.sceneSize != zonesSceneSize)
	{
		zonesSceneSize = s.sceneSize;
		for (auto& z : zoneManager.items) z->setSceneSize(zonesSceneSize);
	}

	for (auto& z : zoneManager.items) z->updateFromPersons(s.persons);

	playerSlots.update(s.persons); //before the people, which show their slot
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

		Array<SpatialZone*> zones;
		sp->firstZoneIndex = -1;
		for (int i = 0; i < zoneManager.items.size(); i++)
		{
			SpatialZone* z = zoneManager.items[i];
			if (!z->idsInside.contains(p.id)) continue; //same membership as the zone (with its anti-flicker)
			if (zones.isEmpty()) sp->firstZoneIndex = i;
			zones.add(z);
		}
		sp->update(p.position, zones);

		int slotIndex = playerSlots.getSlotIndexForPerson(p.id);
		sp->updateTracking(p, slotIndex >= 0 ? playerSlots.slots[slotIndex]->niceName : String());
	}
}

SpatialPerson::SpatialPerson(int id) :
	ControllableContainer("Person " + String(id))
{
	personId = addIntParameter("Id", "Id of the person in DrawZone : the id given by its source, or a free id from 10000 when another source already uses it", id);
	personId->setControllableFeedbackOnly(true);
	sourceName = addStringParameter("Source", "Tracking system (source) that sees the person", "");
	sourceName->setControllableFeedbackOnly(true);
	sourceId = addIntParameter("Source Id", "Id given to the person by its source", id);
	sourceId->setControllableFeedbackOnly(true);
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

	velocity = addPoint2DParameter("Velocity", "Velocity in the scene axes (as sent by the tracking system, turned with its source)");
	velocity->setControllableFeedbackOnly(true);
	orientation = addFloatParameter("Orientation", "Orientation in degrees, as sent by the tracking system plus the rotation of its source", 0);
	orientation->setControllableFeedbackOnly(true);
	height = addFloatParameter("Height", "Height of the person as sent by the tracking system, in meters", 0);
	height->setControllableFeedbackOnly(true);
	age = addFloatParameter("Age", "Seconds since the person entered the scene, as sent by the tracking system", 0);
	age->setControllableFeedbackOnly(true);
}

void SpatialPerson::updateTracking(const TrackedPerson& p, const String& slot)
{
	sourceName->setValue(p.sourceName);
	sourceId->setValue(p.sourceId);
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
	Point<float> rel = SpatialZone::toBox(area, positionMeters);
	relativePosition->setPoint(jlimit(0.0f, 1.0f, rel.x), jlimit(0.0f, 1.0f, rel.y));
}

void SpatialModule::timerCallback()
{
	//people not updated (missed leave, or their source went silent), disabled sources
	trackingData.removeStalePersons(personTimeoutMs);
	{
		bool updateStatus = ++statusTick >= zoneUpdateRateHz / statusUpdateRateHz;
		if (updateStatus) statusTick = 0;

		const ScopedLock sl(sourceManager.items.getLock());
		AugmentaSource* used = sourceManager.getAugmentaSource();
		for (auto& s : sourceManager.items)
		{
			if (AugmentaSource* a = dynamic_cast<AugmentaSource*>(s)) a->isUsed = a == used;
			if (!s->enabled->boolValue()) trackingData.removeSource(s);
			if (updateStatus) s->update(this);
		}
	}
	personCount->setValue(trackingData.getNumPersons());

	updateZones();
}

void SpatialModule::setupAddedZone(SpatialZone* zone)
{
	zonesValuesCC.addChildControllableContainer(&zone->zoneValues);
	zone->setSceneSize(zonesSceneSize);
}

void SpatialModule::itemAdded(SpatialZone* zone)
{
	setupAddedZone(zone);

	//a brand new zone starts at the center of the scene (not a duplicated or loaded one)
	bool isLoading = isCurrentlyLoadingData || zoneManager.isCurrentlyLoadingData;
	bool isNew = !zone->center->isOverriden;
	if (!isLoading && isNew && zonesSceneSize.x > 0 && zonesSceneSize.y > 0) zone->center->setPoint(zonesSceneSize / 2);
}

void SpatialModule::itemsAdded(Array<SpatialZone*> zones)
{
	for (auto& z : zones) setupAddedZone(z);
}

void SpatialModule::itemRemoved(SpatialZone* zone)
{
	zonesValuesCC.removeChildControllableContainer(&zone->zoneValues);
}

void SpatialModule::itemsRemoved(Array<SpatialZone*> zones)
{
	for (auto& z : zones) zonesValuesCC.removeChildControllableContainer(&z->zoneValues);
}

void SpatialModule::itemRemoved(SpatialSource* source)
{
	trackingData.removeSource(source);
}

void SpatialModule::itemsRemoved(Array<SpatialSource*> sources)
{
	for (auto& s : sources) trackingData.removeSource(s);
}

void SpatialModule::setupInspectorSections()
{
	using namespace SpatialSection;
	auto chip = [](const String& text, bool on) { Chip c; c.text = text; c.on = on; return c; };

	sourceManager.sectionSummary = [this, chip]()
		{
			int n = 0;
			for (auto& s : sourceManager.items) if (s->enabled->boolValue()) n++;
			int p = trackingData.getNumPersons();
			return Array<Chip>({ chip(String(n) + (n == 1 ? " source" : " sources"), false), chip(String(p) + " people", p > 0) });
		};

	zoneManager.sectionSummary = [this, chip]()
		{
			int occupied = 0;
			for (auto& z : zoneManager.items) if (!z->idsInside.isEmpty()) occupied++;
			int n = zoneManager.items.size();
			return Array<Chip>({ chip(String(n) + (n == 1 ? " zone" : " zones"), false), chip(String(occupied) + " occupied", occupied > 0) });
		};

	sceneCC.customGetEditorFunc = [this, chip](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
		{
			return new SpatialSectionEditor<GenericControllableContainerEditor>(SCENE, [this, chip]()
				{
					Point<float> s = trackingData.getSceneSize();
					String size = s.x > 0 && s.y > 0 ? String(s.x, 1) + " x " + String(s.y, 1) + " m" : String("no size yet");
					return Array<Chip>({ chip(sceneAuto ? "Auto" : "Manual", false), chip(size, false) });
				}, cc, isRoot);
		};

	playerSlots.customGetEditorFunc = [this, chip](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
		{
			return new SpatialSectionEditor<EnablingControllableContainerEditor>(SLOTS, [this, chip]()
				{
					if (!playerSlots.enabled->boolValue()) return Array<Chip>({ chip("Disabled", false) });
					int n = playerSlots.slotCount->intValue();
					int taken = n - playerSlots.freeSlots->intValue();
					return Array<Chip>({ chip(String(n) + (n == 1 ? " slot" : " slots"), false), chip(String(taken) + " taken", taken > 0) });
				}, Inspectable::getArrayAs<ControllableContainer, EnablingControllableContainer>(cc), isRoot);
		};

	videoOutput.customGetEditorFunc = [this, chip](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
		{
			return new SpatialSectionEditor<GenericControllableContainerEditor>(OUTPUT, [this, chip]()
				{
					bool s = videoOutput.spout->boolValue(), n = videoOutput.ndi->boolValue();
					return Array<Chip>({ chip(s ? "Spout on" : "Spout off", s), chip(n ? "NDI on" : "NDI off", n) });
				}, cc, isRoot);
		};

	//the OSC input keeps its network editor (local IPs in the header), with the badge only
	if (receiveCC != nullptr)
	{
		receiveCC->customGetEditorFunc = [](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
			{
				return new SpatialSectionEditor<EnablingNetworkControllableContainerEditor>(NETWORK, nullptr, Inspectable::getArrayAs<ControllableContainer, EnablingControllableContainer>(cc), isRoot);
			};
	}

	//inspector order (Vincent's rule, keep it for the next blocks) :
	//network inputs, network outputs (OSC, then PSN / TUIO), Sources, Video Output, Scene, Zones, Player Slots
	//all folded at creation and at load (Chataigne only saves the folded state)
	Array<ControllableContainer*> order { receiveCC.get(), outputManager.get(), thruManager.get(), &sourceManager, &videoOutput, &sceneCC, &zoneManager, &playerSlots };
	auto& list = moduleParams.controllableContainers;
	int pos = 0;
	for (auto& cc : order)
	{
		if (cc == nullptr) continue;
		cc->editorIsCollapsed = true;
		int i = list.indexOf(cc);
		if (i < 0) continue;
		list.move(i, pos++);
	}
}
