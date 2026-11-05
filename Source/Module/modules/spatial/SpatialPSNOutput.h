/*
  ==============================================================================

	SpatialPSNOutput.h
	Created: 30 Oct 2026
	Author:  DrawZone

	Sends the tracked people in PosiStageNet, the user chooses who :
	  Player Slots : each surface has its slots ; surface n (from 0), slot k
	    (from 1) -> tracker 60000 + 100 x n + k : SOL 60001, 60002... MUR 60101... Only
	    occupied slots are sent, a held one stays at its last position ;
	    people without a slot are not. A surface whose Player Slots are off
	    sends its people with the id given by the tracking (Source IDs).
	  Source IDs : everyone, with the unique id of the module (the id of its
	    source when free, from 10000 otherwise).
	Each tracker is named "<slot or tracker> (<surface>)", e.g. "JOUEUR 1 (SOL)",
	its values are in the meters of its surface. Every mode and every source
	use the same frame, so changing mode or source moves nothing : PSN axes
	x = surface x, y = up (height given by the source), z = surface y.

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
	int numIdConflicts = 0;	// people not sent : a slot tracker of another surface has their id

	void update(const Array<TrackedPerson>& persons);	// message thread, at the zone rate, after the slots
	void updateStatus();								// message thread, at the status rate

	void onContainerParameterChanged(Parameter* p) override;

	//slots from 60001 : above the ids of the tracking (Augmenta 101.., PSN 1..), which a surface without slots sends as they are
	static const int firstSlotTrackerId = 60001;
	static int getSlotTrackerId(int surfaceIndex, int slotIndex) { return firstSlotTrackerId + 100 * surfaceIndex + slotIndex; }
	static String getTrackerName(const String& name, const String& surface);	// "JOUEUR 1 (SOL)"

private:
	SpatialModule* module;

	std::unique_ptr<DatagramSocket> socket;
	psn::psn_encoder encoder { "DrawZone" };
	uint32 lastInfoMs = 0;
	std::map<int, float> lastHeights;	// by tracker id, for a held slot

	void fillFromSlots(psn::tracker_map& trackers, const Array<TrackedPerson>& persons);
	void fillFromPeople(psn::tracker_map& trackers, const Array<TrackedPerson>& persons);
	void addPerson(psn::tracker_map& trackers, const TrackedPerson& p);	// with its own id
	void openSocket();
	void send(const std::list<std::string>& packets);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPSNOutput)
};
