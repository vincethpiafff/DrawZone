/*
  ==============================================================================

	TrackingData.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Protocol-independent list of tracked people.
	Filled by an input parser (Augmenta OSC for now, PSN / TUIO later)
	and read by the UI (Tracking Monitor).
	Written from the network thread and read from the message thread,
	so every access is locked.

  ==============================================================================
*/

#pragma once

struct TrackedPerson
{
	int id = -1;
	Point<float> position;	// normalized 0-1, origin top left
	Point<float> velocity;	// as sent by the source
	float orientation = 0;
	float height = 0;		// meters
	float age = 0;			// seconds since entering
	uint32 lastUpdateMs = 0;
};

class TrackingData
{
public:
	TrackingData();
	~TrackingData();

	struct Snapshot
	{
		Array<TrackedPerson> persons;
		Point<float> sceneSize;	// meters
		int frame = 0;
		float messagesPerSecond = 0;
		uint32 lastMessageMs = 0;	// 0 = nothing received yet
	};

	void countMessage();
	void setScene(int frame, Point<float> sceneSize);
	void updatePerson(const TrackedPerson& person);
	void removePerson(int id);
	void removeStalePersons(uint32 timeoutMs);
	bool removeAllIfSilent(uint32 timeoutMs);
	void clear();

	int getNumPersons();
	Snapshot getSnapshot();

private:
	CriticalSection lock;
	Snapshot data;

	int messageCount = 0;
	uint32 rateWindowStartMs = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingData)
};
