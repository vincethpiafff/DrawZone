/*
  ==============================================================================

	SpatialModule.h
	Created: 24 Oct 2026
	Author:  DrawZone

	Spatial module, shown as "Cockpit" (see D14): receives tracked people
	from its sources (Augmenta OSC), exposes zones, people and player
	slots as Chataigne parameters.
	Inherits OSCModule to reuse its OSC receiver and senders (see D12).

  ==============================================================================
*/

#pragma once

#include "TrackingData.h"
#include "ui/SpatialStyle.h"
#include "ui/SpatialSectionEditor.h"
#include "PlayerSlots.h"
#include "ui/TopViewRenderer.h"
#include "ui/Viewport3DRenderer.h"
#include "SpatialVideoOutput.h"
#include "SpatialSurface.h"
#include "SpatialSource.h"
#include "SpatialZone.h"
#include "SpatialPSNOutput.h"

//Values of one tracked person, shown in Values > People while the person is tracked
class SpatialPerson :
	public ControllableContainer
{
public:
	SpatialPerson(int id);

	IntParameter* personId;
	StringParameter* sourceName;			// source of the person
	IntParameter* sourceId;				// id given by its source
	StringParameter* surfaceName;		// surface the person is on
	Point2DParameter* position;			// meters, origin top left of the surface
	StringParameter* zoneName;			// zone(s) the person is in, empty outside
	Point2DParameter* relativePosition;	// 0-1 in the zone or zones overlap, -1 outside
	StringParameter* slotName;			// player slot of the person, empty without a slot

	//as sent by the tracking system
	Point2DParameter* velocity;
	FloatParameter* orientation;
	FloatParameter* height;		// meters
	FloatParameter* age;		// seconds since entering

	int firstZoneIndex = -1;	// index of the first zone the person is in, -1 = none (for the top view)

	void update(Point<float> positionMeters, const Array<SpatialZone*>& zones);
	void updateTracking(const TrackedPerson& p, const String& slot, const String& surface);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPerson)
};

class SpatialModule :
	public OSCModule,
	public Timer,
	public BaseManager<SpatialZone>::ManagerListener,
	public BaseManager<SpatialSource>::ManagerListener,
	public BaseManager<SpatialSurface>::ManagerListener
{
public:
	SpatialModule();
	~SpatialModule();

	//a person not updated for this long is removed (missed /object/leave, or its source went silent)
	static const uint32 personTimeoutMs = 1000;
	//no message at all for this long : the source is shown as silent
	static const uint32 sourceTimeoutMs = 1000;
	//shown by the Monitor and the video output while the surface size is unknown
	static String getNoSurfaceSizeText() { return "Waiting for the surface size (a source of this surface knowing its area, Use As Surface on one if there are several, or Surfaces > Size Mode : Manual)"; }
	//zones are evaluated on the message thread at this rate
	static const int zoneUpdateRateHz = 50;
	//status texts (sources) are rebuilt at this rate only
	static const int statusUpdateRateHz = 4;
	int statusTick = 0;

	TrackingData trackingData;

	//tracking systems (Augmenta, PSN...), each with its transform to the surface
	SpatialSourceManager sourceManager;

	//flat tracked areas (floor, wall...), each placed in 3D ; sources feed a surface, zones belong to one
	SpatialSurfaceManager surfaceManager;
	void updateSurfaces(bool updateStatus);	// sizes (Auto : from the sources of each surface), links of sources and zones

	//Scene block of the sessions before v0.6 (one scene) : still loaded, hidden, moved into the first surface
	enum LegacySceneSizeMode { LEGACY_SCENE_AUTO, LEGACY_SCENE_MANUAL };
	ControllableContainer legacySceneCC;
	EnumParameter* legacySceneSizeMode;
	Point2DParameter* legacySceneSizeManual;
	void afterLoadJSONDataInternal() override;

	//Inspector : section look (icon badge, state chips), blocks of the session first, network last, all folded
	void setupInspectorSections();

	IntParameter* personCount;

	SpatialZoneManager zoneManager;
	ControllableContainer zonesValuesCC;

	ControllableContainer peopleValuesCC;
	std::map<int, SpatialPerson*> people;	// by person id, owned by peopleValuesCC

	ControllableContainer slotsValuesCC;	// Values > Slots : the slots of each surface
	//the player slot of a person, nullptr when none ; surface : where that slot is
	PlayerSlot* getSlotForPerson(int personId, SpatialSurface** surface = nullptr) const;
	SpatialSurface* getSurface(int index) const;	// clamped, nullptr without surface

	SpatialPSNOutput psnOutput;		// player slots sent as PSN trackers

	void processMessageInternal(const OSCMessage& msg) override;

	void updateZones();
	void updatePeople(const TrackingData::Snapshot& s);
	void timerCallback() override;

	void setupAddedZone(SpatialZone* zone);	// its values in Values > Zones, its bounds
	void itemAdded(SpatialZone* zone) override;
	void itemsAdded(Array<SpatialZone*> zones) override;
	void itemRemoved(SpatialZone* zone) override;
	void itemsRemoved(Array<SpatialZone*> zones) override;

	void itemRemoved(SpatialSource* source) override;
	void itemsRemoved(Array<SpatialSource*> sources) override;

	void itemAdded(SpatialSurface* surface) override;
	void itemsAdded(Array<SpatialSurface*> surfaces) override;
	void itemRemoved(SpatialSurface* surface) override;
	void itemsRemoved(Array<SpatialSurface*> surfaces) override;

	//sessions saved before surfaces (Scenes block of the first v0.6 builds, module-wide Player Slots and Video Output) : moved into the surfaces
	void loadJSONDataItemInternal(var data) override;
	static var migrateParams(var params);


	static SpatialModule* create() { return new SpatialModule(); }
	virtual String getDefaultTypeString() const override { return "Cockpit"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialModule)
};

#include "ui/TrackingMonitorPanel.h"
