/*
  ==============================================================================

	TrackingData.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Protocol-independent list of tracked people, from one or more sources
	(Augmenta OSC, PSN...). A person's id is the id given by its source plus
	the Id Offset of that source, so it doesn't depend on who arrives first.
	If another source already uses it (offsets badly set), it gets a free id
	from 10000, so ids stay unique across sources.
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
	int idOffset = 0;		// Id Offset of its source
	String sourceName;		// name of its source
	Point<float> position;	// scene meters, origin top left (may be outside the scene)
	Point<float> velocity;	// scene meters, as sent by the source
	float orientation = 0;
	float height = 0;		// meters
	float age = 0;			// seconds since entering
	String trackerName;		// "system tracker" name from its source, empty when it has none
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
		float messagesPerSecond = 0;
		uint32 lastMessageMs = 0;	// 0 = nothing received yet
	};

	//reception of one source
	struct SourceStats
	{
		float messagesPerSecond = 0;
		uint32 lastMessageMs = 0;	// 0 = nothing received yet
	};

	static const int firstRemappedId = 10000;

	void countMessage(const void* source);
	SourceStats getSourceStats(const void* source);
	void setSceneSize(Point<float> sceneSize);
	Point<float> getSceneSize();

	//person.source and person.sourceId identify the person, its id is given here
	void updatePerson(const TrackedPerson& person);
	void removePerson(const void* source, int sourceId);
	void removeSource(const void* source);
	void removeStalePersons(uint32 timeoutMs);	// also empties a source gone silent

	int getNumPersons();
	int getNumPersons(const void* source);
	int getNumIdConflicts(const void* source);	// people who could not get source id + Id Offset
	Snapshot getSnapshot();

private:
	CriticalSection lock;
	Snapshot data;
	int nextRemappedId = firstRemappedId;

	//messages counted over about a second, for the whole module and per source
	struct RateCounter
	{
		float messagesPerSecond = 0;
		uint32 lastMessageMs = 0;
		int count = 0;
		uint32 windowStartMs = 0;

		void add(uint32 now);
		float getRate(uint32 now) const;	// 0 when silent for more than a second, not an old rate
	};

	RateCounter moduleRate;
	std::map<const void*, RateCounter> sourceRates;

	bool isIdUsed(int id) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingData)
};
