/*
  ==============================================================================

	SpatialModule.h
	Created: 24 Oct 2026
	Author:  DrawZone

	Spatial module: receives tracked people (Augmenta OSC for now)
	and will expose interaction zones as Chataigne parameters.
	Inherits OSCModule to reuse its OSC receiver and senders (see D12).

  ==============================================================================
*/

#pragma once

#include "TrackingData.h"

class SpatialModule :
	public OSCModule,
	public Timer
{
public:
	SpatialModule();
	~SpatialModule();

	//a person not updated for this long is removed (missed /object/leave)
	static const uint32 personTimeoutMs = 1000;
	//no message at all for this long : the source is silent, the list is emptied
	static const uint32 sourceTimeoutMs = 1000;

	TrackingData trackingData;

	IntParameter* personCount;

	void processMessageInternal(const OSCMessage& msg) override;
	void processAugmentaPerson(const OSCMessage& msg);

	void timerCallback() override;

	static SpatialModule* create() { return new SpatialModule(); }
	virtual String getDefaultTypeString() const override { return "Spatial"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialModule)
};

#include "ui/TrackingMonitorPanel.h"
