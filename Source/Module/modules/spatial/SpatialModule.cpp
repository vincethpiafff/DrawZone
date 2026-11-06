/*
  ==============================================================================

	SpatialModule.cpp
	Created: 24 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#include "TrackingData.cpp"
#include "SpatialSurface.cpp"
#include "SpatialSource.cpp"
#include "SpatialZone.cpp"
#include "PlayerSlots.cpp"
#include "ui/TopViewRenderer.cpp"
#include "ui/Viewport3DRenderer.cpp"
#include "SpatialVideoOutput.cpp"
#include "SpatialPSNOutput.cpp"
#include "ui/TrackingMonitorPanel.cpp"
#if ORGANICUI_USE_MCP
#include "MCP/ChataigneMcp.h"	// Live Mode : MCP server paused
#endif

SpatialModule::SpatialModule() :
	OSCModule(getDefaultTypeString(), 12000, 9000),
	legacySceneCC("Scene"),
	zonesValuesCC("Zones"),
	peopleValuesCC("People"),
	slotsValuesCC("Slots"),
	psnOutput(this)
{
	personCount = valuesCC.addIntParameter("Person Count", "Number of people currently tracked, all surfaces together", 0, 0);
	personCount->setControllableFeedbackOnly(true);

	legacySceneSizeMode = legacySceneCC.addEnumParameter("Size Mode", "Sessions before v0.6 only : moved into the first surface");
	legacySceneSizeMode->addOption("Auto (From Sources)", LEGACY_SCENE_AUTO)->addOption("Manual", LEGACY_SCENE_MANUAL);
	legacySceneSizeManual = legacySceneCC.addPoint2DParameter("Size", "Sessions before v0.6 only : moved into the first surface");
	legacySceneSizeManual->setBounds(.1f, .1f, 1000, 1000);
	legacySceneSizeManual->setDefaultPoint(10, 6);

	//a new module starts with one surface (sessions before v0.6 too : nothing to load)
	surfaceManager.module = this;
	surfaceManager.addBaseManagerListener(this);
	moduleParams.addChildControllableContainer(&surfaceManager);
	surfaceManager.addItem(new SpatialSurface(this), var(), false);
	sourceManager.surfaceManager = &surfaceManager;
	zoneManager.surfaceManager = &surfaceManager;

	//and an Augmenta source on its OSC input (sessions without sources too : nothing to load)
	moduleParams.addChildControllableContainer(&sourceManager);
	sourceManager.addBaseManagerListener(this);
	sourceManager.addItem(new AugmentaSource(), var(), false);

	legacySceneCC.hideInEditor = true;
	moduleParams.addChildControllableContainer(&legacySceneCC);

	//zones are saved with the module parameters (recursive save)
	moduleParams.addChildControllableContainer(&zoneManager);
	zoneManager.selectItemWhenCreated = false;
	zoneManager.addBaseManagerListener(this);

	valuesCC.addChildControllableContainer(&zonesValuesCC);
	peopleValuesCC.saveAndLoadRecursiveData = false; //live data, nothing to save
	valuesCC.addChildControllableContainer(&peopleValuesCC);
	valuesCC.addChildControllableContainer(&slotsValuesCC);

	moduleParams.addChildControllableContainer(&psnOutput);
	moduleParams.addChildControllableContainer(&liveMode);

	setupInspectorSections();

	startTimerHz(zoneUpdateRateHz);
}

SpatialModule::~SpatialModule()
{
	stopTimer();
	for (auto& s : surfaceManager.items) slotsValuesCC.removeChildControllableContainer(&s->playerSlots.slotsValues);
	zoneManager.removeBaseManagerListener(this);
	sourceManager.removeBaseManagerListener(this);
	surfaceManager.removeBaseManagerListener(this);
}

void SpatialModule::processMessageInternal(const OSCMessage& msg)
{
	//the OSC input feeds the first enabled Augmenta source on its port ; locked : a source is not deleted while it parses
	const ScopedLock sl(sourceManager.items.getLock());
	if (AugmentaSource* a = sourceManager.getAugmentaSource(localPort != nullptr ? localPort->intValue() : -1)) a->processMessage(msg, this);
}

void SpatialModule::afterLoadJSONDataInternal()
{
	OSCModule::afterLoadJSONDataInternal();

	//session before v0.6 : its Scene block goes into the first surface, then is put back to default (not saved again)
	if (!legacySceneSizeMode->isOverriden && !legacySceneSizeManual->isOverriden) return;
	if (SpatialSurface* s = surfaceManager.items.isEmpty() ? nullptr : surfaceManager.items[0])
	{
		if (!s->sizeMode->isOverriden && !s->manualSize->isOverriden)
		{
			s->sizeMode->setValueWithData(legacySceneSizeMode->getValueData());
			s->manualSize->setPoint(legacySceneSizeManual->getPoint());
		}
	}
	legacySceneSizeMode->resetValue();
	legacySceneSizeManual->resetValue();
}

void SpatialModule::updateSurfaces(bool updateStatus)
{
	//call under sourceManager.items.getLock()
	for (auto& s : sourceManager.items) s->surfaceId = surfaceManager.getSurface(s->surfaceTarget);
	for (auto& z : zoneManager.items) z->surfaceId = surfaceManager.getSurface(z->surfaceTarget);

	for (auto& surface : surfaceManager.items)
	{
		String from;
		bool manual = surface->sizeMode->getValueDataAsEnum<SpatialSurface::SizeMode>() == SpatialSurface::SIZE_MANUAL;
		surface->size = manual ? surface->manualSize->getPoint() : sourceManager.getSurfaceArea(surface, &from);
		if (!updateStatus) continue;

		String s = surface->size.x <= 0 || surface->size.y <= 0 ? String("No size yet") : String(surface->size.x, 1) + " x " + String(surface->size.y, 1) + " m, " + (manual ? String("manual") : "from " + from);
		//Spout / NDI streams and Values > Slots are named after the surface
		for (auto& other : surfaceManager.items)
			if (other != surface && other->niceName == surface->niceName) { s += ". Warning : another surface has the same name, give each one its own (video streams, Values > Slots)"; break; }
		surface->status->setValue(s);
	}
}

void SpatialModule::updateZones()
{
	TrackingData::Snapshot s = trackingData.getSnapshot();

	//each zone only sees the people of its surface
	for (auto& z : zoneManager.items)
	{
		Array<TrackedPerson> inSurface;
		for (auto& p : s.persons) if (p.surface == z->surfaceId) inSurface.add(p);
		z->updateFromPersons(inSurface);
	}

	//each surface gives its slots to its people, before the people, which show their slot
	for (auto& surface : surfaceManager.items)
	{
		Array<TrackedPerson> onSurface;
		for (auto& p : s.persons) if (p.surface == surface) onSurface.add(p);
		surface->playerSlots.update(onSurface);
	}
	psnOutput.update(s.persons);
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

		PlayerSlot* slot = getSlotForPerson(p.id);
		String surfaceName;
		for (auto& surface : surfaceManager.items) if (surface == p.surface) surfaceName = surface->niceName;
		sp->updateTracking(p, slot != nullptr ? slot->niceName : String(), surfaceName);
	}
}

SpatialPerson::SpatialPerson(int id) :
	ControllableContainer("Person " + String(id))
{
	personId = addIntParameter("Id", "Id of the person in DrawZone : the id given by its source plus its Id Offset, or a free id from 10000 when another source already uses it", id);
	personId->setControllableFeedbackOnly(true);
	sourceName = addStringParameter("Source", "Tracking system (source) that sees the person", "");
	sourceName->setControllableFeedbackOnly(true);
	sourceId = addIntParameter("Source Id", "Id given to the person by its source", id);
	sourceId->setControllableFeedbackOnly(true);
	surfaceName = addStringParameter("Surface", "Surface the person is on (the surface of its source)", "");
	surfaceName->setControllableFeedbackOnly(true);
	position = addPoint2DParameter("Position", "Position in meters, from the top left corner of the surface");
	position->setControllableFeedbackOnly(true);
	zoneName = addStringParameter("Zone", "Zone the person is in, \"A + B\" where zones overlap, empty outside any zone", "");
	zoneName->setControllableFeedbackOnly(true);
	relativePosition = addPoint2DParameter("Relative Position", "Position in the zone, from 0,0 (top left) to 1,1 (bottom right). Where zones overlap, position in the overlapping area. -1,-1 outside any zone");
	relativePosition->setBounds(-1, -1, 1, 1);
	relativePosition->setPoint(-1, -1);
	relativePosition->setControllableFeedbackOnly(true);
	slotName = addStringParameter("Slot", "Player slot of the person on its surface, empty without a slot", "");
	slotName->setControllableFeedbackOnly(true);

	velocity = addPoint2DParameter("Velocity", "Velocity in the surface axes (as sent by the tracking system, turned with its source)");
	velocity->setControllableFeedbackOnly(true);
	orientation = addFloatParameter("Orientation", "Orientation in degrees, as sent by the tracking system plus the rotation of its source", 0);
	orientation->setControllableFeedbackOnly(true);
	height = addFloatParameter("Height", "Height of the person as sent by the tracking system, in meters", 0);
	height->setControllableFeedbackOnly(true);
	age = addFloatParameter("Age", "Seconds since the person entered the surface, as sent by the tracking system", 0);
	age->setControllableFeedbackOnly(true);
}

void SpatialPerson::updateTracking(const TrackedPerson& p, const String& slot, const String& surface)
{
	surfaceName->setValue(surface);
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
		bool updateStatus = ++statusTick >= zoneUpdateRateHz / (liveMode.isLive() ? 1 : statusUpdateRateHz);
		if (updateStatus) statusTick = 0;

		const ScopedLock sl(sourceManager.items.getLock());
		for (auto& s : sourceManager.items)
		{
			if (!s->enabled->boolValue()) trackingData.removeSource(s);
			if (updateStatus) s->update(this);
		}
		if (updateStatus) psnOutput.updateStatus();
		updateSurfaces(updateStatus);
	}
	personCount->setValue(trackingData.getNumPersons());

	updateZones();
}

void SpatialModule::setupAddedZone(SpatialZone* zone)
{
	zonesValuesCC.addChildControllableContainer(&zone->zoneValues);
}

void SpatialModule::itemAdded(SpatialZone* zone)
{
	setupAddedZone(zone);

	//a brand new zone starts at the center of its surface (not a duplicated or loaded one)
	bool isLoading = isCurrentlyLoadingData || zoneManager.isCurrentlyLoadingData;
	bool isNew = !zone->center->isOverriden;
	SpatialSurface* surface = surfaceManager.getSurface(zone->surfaceTarget);
	Point<float> size = surface != nullptr ? surface->size : Point<float>();
	if (!isLoading && isNew && size.x > 0 && size.y > 0) zone->center->setPoint(size / 2);
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

void SpatialModule::itemAdded(SpatialSurface* surface)
{
	slotsValuesCC.addChildControllableContainer(&surface->playerSlots.slotsValues);
}

void SpatialModule::itemsAdded(Array<SpatialSurface*> surfaces)
{
	for (auto& s : surfaces) itemAdded(s);
}

void SpatialModule::itemRemoved(SpatialSurface* surface)
{
	slotsValuesCC.removeChildControllableContainer(&surface->playerSlots.slotsValues);
}

void SpatialModule::itemsRemoved(Array<SpatialSurface*> surfaces)
{
	for (auto& s : surfaces) itemRemoved(s);
}

PlayerSlot* SpatialModule::getSlotForPerson(int personId, SpatialSurface** surface) const
{
	for (auto& s : surfaceManager.items)
	{
		int i = s->playerSlots.getSlotIndexForPerson(personId);
		if (i < 0) continue;
		if (surface != nullptr) *surface = s;
		return s->playerSlots.slots[i];
	}
	return nullptr;
}

SpatialSurface* SpatialModule::getSurface(int index) const
{
	if (surfaceManager.items.isEmpty()) return nullptr;
	return surfaceManager.items[jlimit(0, surfaceManager.items.size() - 1, index)];
}

var SpatialModule::getJSONData(bool includeNonOverriden)
{
	var data = OSCModule::getJSONData(includeNonOverriden);
	data.getDynamicObject()->setProperty("drawzoneFormat", formatVersion);
	return data;
}

void SpatialModule::loadJSONDataItemInternal(var data)
{
	if (data.getDynamicObject() != nullptr && data.hasProperty("params"))
	{
		data = JSON::parse(JSON::toString(data)); //a copy : the session data is not changed
		int format = data.getProperty("drawzoneFormat", 1);
		data.getDynamicObject()->setProperty("params", migrateParams(data.getProperty("params", var()), format));
	}
	OSCModule::loadJSONDataItemInternal(data);
}

var SpatialModule::migrateParams(var params, int format)
{
	DynamicObject* containers = params.getProperty("containers", var()).getDynamicObject();
	if (containers == nullptr) return params;

	//Augmenta sources before format 2 : Use Module Input (on by default) -> Port = the port of the module's OSC Input
	int modulePort = 12000;
	var inputParameters = containers->getProperty("oscInput").getProperty("parameters", var());
	for (int j = 0; j < inputParameters.size(); j++)
		if (inputParameters[j].getProperty("controlAddress", "").toString() == "/localPort") modulePort = inputParameters[j].getProperty("value", modulePort);
	var sourceItems = containers->getProperty("sources").getProperty("items", var());
	for (int i = 0; i < sourceItems.size(); i++)
	{
		var item = sourceItems[i];
		if (item.getProperty("type", "").toString() != "Augmenta") continue;
		if (!item.getProperty("parameters", var()).isArray()) item.getDynamicObject()->setProperty("parameters", Array<var>());
		var parameters = item.getProperty("parameters", var());
		bool useModuleInput = true;
		int portIndex = -1;
		for (int j = parameters.size() - 1; j >= 0; j--)
		{
			String address = parameters[j].getProperty("controlAddress", "").toString();
			if (address == "/port") portIndex = j;
			if (address != "/useModuleInput") continue;
			useModuleInput = parameters[j].getProperty("value", true);
			parameters.remove(j);
			if (portIndex > j) portIndex--;
		}
		if (format >= 2 || !useModuleInput) continue;
		if (portIndex >= 0) parameters[portIndex].getDynamicObject()->setProperty("value", modulePort);
		else
		{
			var p = new DynamicObject();
			p.getDynamicObject()->setProperty("value", modulePort);
			p.getDynamicObject()->setProperty("controlAddress", "/port");
			parameters.append(p);
		}
	}

	//first v0.6 builds : Scenes block, Scene and Use As Scene parameters
	if (containers->hasProperty("scenes") && !containers->hasProperty("surfaces"))
	{
		containers->setProperty("surfaces", containers->getProperty("scenes"));
		containers->removeProperty("scenes");
	}
	auto renameParameters = [](var manager)
		{
			var items = manager.getProperty("items", var());
			for (int i = 0; i < items.size(); i++)
			{
				var item = items[i];
				if (item.getProperty("type", "").toString() == "Scene") item.getDynamicObject()->setProperty("type", "Surface");
				var parameters = item.getProperty("parameters", var());
				for (int j = 0; j < parameters.size(); j++)
				{
					DynamicObject* p = parameters[j].getDynamicObject();
					if (p == nullptr) continue;
					String address = p->getProperty("controlAddress").toString();
					if (address == "/scene") p->setProperty("controlAddress", "/surface");
					else if (address == "/useAsScene") p->setProperty("controlAddress", "/useAsSurface");
				}
			}
		};
	renameParameters(containers->getProperty("surfaces"));
	renameParameters(containers->getProperty("sources"));
	renameParameters(containers->getProperty("zones"));

	//before surfaces : Player Slots and Video Output of the module go into the first surface
	var slots = containers->getProperty("playerSlots");
	var video = containers->getProperty("videoOutput");
	if (slots.isVoid() && video.isVoid()) return params;

	if (!containers->hasProperty("surfaces"))
	{
		var surfaces = new DynamicObject();
		surfaces.getDynamicObject()->setProperty("items", Array<var>());
		containers->setProperty("surfaces", surfaces);
	}
	var items = containers->getProperty("surfaces").getProperty("items", var());
	if (!items.isArray()) return params;
	if (items.size() == 0)
	{
		var first = new DynamicObject();
		first.getDynamicObject()->setProperty("niceName", "Surface");
		first.getDynamicObject()->setProperty("type", "Surface");
		items.append(first);
	}

	var first = items[0];
	if (!first.getProperty("containers", var()).isObject()) first.getDynamicObject()->setProperty("containers", new DynamicObject());
	DynamicObject* own = first.getProperty("containers", var()).getDynamicObject();
	if (!slots.isVoid() && !own->hasProperty("playerSlots")) own->setProperty("playerSlots", slots);
	if (!video.isVoid() && !own->hasProperty("videoOutput"))
	{
		//its stream now takes the name of the surface
		var parameters = video.getProperty("parameters", var());
		if (parameters.isArray()) for (int j = parameters.size() - 1; j >= 0; j--) if (parameters[j].getProperty("controlAddress", "").toString() == "/streamName") parameters.remove(j);
		own->setProperty("videoOutput", video);
	}
	containers->removeProperty("playerSlots");
	containers->removeProperty("videoOutput");
	return params;
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

	surfaceManager.sectionSummary = [this, chip]()
		{
			int n = surfaceManager.items.size();
			return Array<Chip>({ chip(String(n) + (n == 1 ? " surface" : " surfaces"), false) });
		};

	psnOutput.customGetEditorFunc = [this, chip](bool isRoot, Array<ControllableContainer*> cc) -> InspectableEditor*
		{
			return new SpatialSectionEditor<EnablingControllableContainerEditor>(OUTPUT, [this, chip]()
				{
					if (!psnOutput.enabled->boolValue()) return Array<Chip>({ chip("Disabled", false) });
					int n = psnOutput.numTrackersSent;
					return Array<Chip>({ chip(String(n) + (n == 1 ? " tracker" : " trackers"), n > 0) });
				}, Inspectable::getArrayAs<ControllableContainer, EnablingControllableContainer>(cc), isRoot);
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
	//Live Mode, network inputs, network outputs (OSC, then PSN), Sources, Surfaces (each with its Player Slots and Video Output), Zones
	//all folded at creation and at load (Chataigne only saves the folded state)
	Array<ControllableContainer*> order { &liveMode, receiveCC.get(), outputManager.get(), thruManager.get(), &psnOutput, &sourceManager, &surfaceManager, &zoneManager };
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

// SpatialLiveMode

SpatialLiveMode::SpatialLiveMode() :
	EnablingControllableContainer("Live Mode")
{
	enabled->setDefaultValue(false);
	enabled->description = ("Live Mode : only the display and the editing stop, to save the computer for the show. Tracking, zones, slots, logic, variables and every output (OSC, PSN, Spout, NDI) keep running. Stopped : the Tracking Monitor view, editing in it, the Inspector (nothing can be selected : leave Live Mode with the LIVE button of the Tracking Monitor, the State Machine, OSC or the Dashboard), status texts at 1 Hz");
	slowPreview = addBoolParameter("Slow Preview", "Live Mode : the Tracking Monitor view is redrawn once a second instead of being frozen", false);
	pauseMcp = addBoolParameter("Pause MCP", "Live Mode : the MCP server (link with Claude) is turned off while live, and back on when Live Mode stops", false);
}

SpatialLiveMode::~SpatialLiveMode()
{
	if (!isLive()) return;
	enabled->setValue(false, true); //what Live Mode turned off comes back if the module goes away while live
	apply();
}

void SpatialLiveMode::onContainerParameterChanged(Parameter* p)
{
	EnablingControllableContainer::onContainerParameterChanged(p);
	if (p == enabled || p == pauseMcp) apply();
}

void SpatialLiveMode::apply()
{
	bool live = isLive();
	if (InspectableSelectionManager::mainSelectionManager != nullptr) InspectableSelectionManager::mainSelectionManager->setEnabled(!live);

#if ORGANICUI_USE_MCP
	ChataigneEngine* e = dynamic_cast<ChataigneEngine*>(Engine::mainEngine);
	if (e == nullptr || e->mcp == nullptr) return;
	if (live && pauseMcp->boolValue())
	{
		if (!e->mcp->enabled->boolValue()) return;
		mcpPausedByUs = true;
		e->mcp->enabled->setValue(false);
	}
	else if (mcpPausedByUs)
	{
		mcpPausedByUs = false;
		e->mcp->enabled->setValue(true);
	}
#endif
}
