/*
  ==============================================================================

	SpatialPSNOutput.h
	Created: 30 Oct 2026
	Author:  DrawZone

	Sends the tracked people in PosiStageNet, the user chooses who :
	  Player Slots : slot 1 -> tracker 1... Only occupied slots are sent, a
	    held one stays at its last position ; people without a slot are not.
	  Source IDs : everyone, with the unique id of the module (the id of its
	    source when free, from 10000 otherwise).
	Every mode and every source use the same frame, so changing mode or source
	moves nothing : scene meters, PSN axes : x = scene x, y = up (height given
	by the source), z = scene y.

	Sent on one network card only, chosen by the user (Auto : the default one).

  ==============================================================================
*/

#pragma once

class SpatialModule;

class SpatialPSNOutput :
	public EnablingControllableContainer
{
public:
	SpatialPSNOutput(SpatialModule* module);
	~SpatialPSNOutput();

	enum SendMode { PLAYER_SLOTS, SOURCE_IDS };

	EnumParameter* sendMode;
	NetworkInterfaceParameter* networkInterface;
	StringParameter* multicastAddress;
	IntParameter* multicastPort;
	StringParameter* status;	// read only

	int numTrackersSent = 0;

	void update(const Array<TrackedPerson>& persons);	// message thread, at the zone rate, after the slots
	void updateStatus();								// message thread, at the status rate

	void onContainerParameterChanged(Parameter* p) override;

private:
	SpatialModule* module;

	std::unique_ptr<DatagramSocket> socket;
	psn::psn_encoder encoder { "DrawZone" };
	uint32 lastInfoMs = 0;
	std::map<int, float> lastHeights;	// by slot index, for a held slot

	void fillFromSlots(psn::tracker_map& trackers, const Array<TrackedPerson>& persons);
	void fillFromPeople(psn::tracker_map& trackers, const Array<TrackedPerson>& persons);
	void openSocket();
	void send(const std::list<std::string>& packets);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPSNOutput)
};
