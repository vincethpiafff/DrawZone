/*
  ==============================================================================

	TrackingData.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Protocol-independent list of tracked people, from one or more sources
	(Augmenta OSC, PSN, TUIO...). Each source gives its own ids : a person
	keeps the id of its source when no one else uses it, otherwise gets a
	free id from 10000, so ids stay unique across sources.
	Filled by the sources and read by the zones, the slots and the UI.
	Written from the network threads and read from the message thread,
	so every access is locked.

  ==============================================================================
*/

#pragma once

struct TrackedPerson
{
	int id = -1;			// unique in the module
	int sourceId = -1;		// id given by its source
	String sourceName;		// name of its source
	Point<float> position;	// normalized 0-1 in the scene, origin top left
	Point<float> velocity;	// scene meters, as sent by the source
	float orientation = 0;
	float height = 0;		// meters
	float age = 0;			// seconds since entering
	uint32 lastUpdateMs = 0;
	const void* source = nullptr;	// identifies the source, never dereferenced
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

	static const int firstRemappedId = 10000;

	void countMessage(const void* source);
	void setScene(int frame, Point<float> sceneSize);
	void setSceneSize(Point<float> sceneSize);
	Point<float> getSceneSize();

	//person.source and person.sourceId identify the person, its id is given here
	void updatePerson(const TrackedPerson& person);
	void removePerson(const void* source, int sourceId);
	void removeSource(const void* source);
	void removeStalePersons(uint32 timeoutMs);
	bool removeSilentSources(uint32 timeoutMs);	// true if people were removed
	void clear();

	int getNumPersons();
	int getNumPersons(const void* source);
	Snapshot getSnapshot();

private:
	CriticalSection lock;
	Snapshot data;
	std::map<const void*, uint32> lastSourceMessageMs;
	int nextRemappedId = firstRemappedId;

	int messageCount = 0;
	uint32 rateWindowStartMs = 0;

	bool isIdUsed(int id) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingData)
};
