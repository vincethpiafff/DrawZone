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
#include "ui/SpatialSectionEditor.h"
#include "SpatialSource.h"
#include "SpatialZone.h"
#include "PlayerSlots.h"
#include "ui/TopViewRenderer.h"
#include "SpatialVideoOutput.h"

//Values of one tracked person, shown in Values > People while the person is tracked
class SpatialPerson :
	public ControllableContainer
{
public:
	SpatialPerson(int id);

	IntParameter* personId;
	StringParameter* sourceName;			// source of the person
	IntParameter* sourceId;				// id given by its source
	Point2DParameter* position;			// meters, origin top left of the scene
	StringParameter* zoneName;			// zone(s) the person is in, empty outside
	Point2DParameter* relativePosition;	// 0-1 in the zone or zones overlap, -1 outside
	StringParameter* slotName;			// player slot of the person, empty without a slot

	//as sent by the tracking system
	Point2DParameter* velocity;
	FloatParameter* orientation;
	FloatParameter* height;		// meters
	FloatParameter* age;		// seconds since entering

	void update(Point<float> positionMeters, const Array<SpatialZone*>& zones);
	void updateTracking(const TrackedPerson& p, const String& slot);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPerson)
};

class SpatialModule :
	public OSCModule,
	public Timer,
	public BaseManager<SpatialZone>::ManagerListener,
	public BaseManager<SpatialSource>::ManagerListener
{
public:
	SpatialModule();
	~SpatialModule();

	//a person not updated for this long is removed (missed /object/leave)
	static const uint32 personTimeoutMs = 1000;
	//no message at all for this long : the source is silent, the list is emptied
	static const uint32 sourceTimeoutMs = 1000;
	//zones are evaluated on the message thread at this rate
	static const int zoneUpdateRateHz = 50;

	TrackingData trackingData;

	//tracking systems (Augmenta, PSN, TUIO...), each with its transform to the scene
	SpatialSourceManager sourceManager;

	//scene size : from Augmenta /scene (Auto) or set by hand (Manual)
	enum SceneSizeMode { SCENE_AUTO, SCENE_MANUAL };
	ControllableContainer sceneCC;
	EnumParameter* sceneSizeMode;
	Point2DParameter* sceneSizeManual;
	std::atomic<bool> sceneAuto { true };	// read by the network threads
	bool isSceneSizeAuto() const { return sceneAuto; }
	void updateSceneSize();

	//Inspector : section look (icon badge, state chips), blocks of the session first, network last, all folded
	void setupInspectorSections();

	IntParameter* personCount;

	SpatialSectionManager<SpatialZone> zoneManager;
	ControllableContainer zonesValuesCC;
	Point<float> zonesSceneSize;	//scene size last applied to the zone bounds

	ControllableContainer peopleValuesCC;
	std::map<int, SpatialPerson*> people;	// by person id, owned by peopleValuesCC

	PlayerSlots playerSlots;

	SpatialVideoOutput videoOutput;	// top view sent as video (Spout, NDI)

	void processMessageInternal(const OSCMessage& msg) override;

	void updateZones();
	void updatePeople(const TrackingData::Snapshot& s);
	void timerCallback() override;

	void itemAdded(SpatialZone* zone) override;
	void itemsAdded(Array<SpatialZone*> zones) override;
	void itemRemoved(SpatialZone* zone) override;
	void itemsRemoved(Array<SpatialZone*> zones) override;

	void itemRemoved(SpatialSource* source) override;
	void itemsRemoved(Array<SpatialSource*> sources) override;

	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

	static SpatialModule* create() { return new SpatialModule(); }
	virtual String getDefaultTypeString() const override { return "Cockpit"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialModule)
};

#include "ui/TrackingMonitorPanel.h"
