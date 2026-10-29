/*
  ==============================================================================

	TopViewRenderer.h
	Created: 28 Oct 2026
	Author:  DrawZone

	Top view of a Spatial module (zones, people, player slots), drawn the same
	way in the Tracking Monitor and in the video output (Spout / NDI).
	TopViewModel copies what is drawn from the module (message thread),
	TopViewRenderer paints it at any scale.

  ==============================================================================
*/

#pragma once

class SpatialModule;

//what is drawn, copied from the module on each refresh so painting never touches the network data
class TopViewModel
{
public:
	bool hasModule = false;
	String moduleName;
	int localPort = 0;
	String sourcesText;	// each enabled source and its reception
	TrackingData::Snapshot snapshot;

	struct ZoneView
	{
		WeakReference<ControllableContainer> zone;
		String name;
		Rectangle<float> bounds;	// meters
		bool ellipse = false;
		bool freeShape = false;	// neither : rectangle
		Path outline;				// meters, free shape only
		Array<Point<float>> points;	// meters, free shape only
		Array<int> interpolations;	// ZonePoint::Interpolation of each point
		Array<Point<float>> handlesIn, handlesOut;	// curve control points, meters
		Colour color;
		bool enabled = true;
		bool presence = false;
		bool hidden = false;
		bool selected = false;
		int personCount = 0;

		//live values shown on the zone cards
		int capacity = 1;
		float occupancy = 0;		// 0-1
		float presenceTime = 0;		// seconds
		int lastEnteredId = -1;
		Point<float> averagePosition;	// 0-1 in the box
		float averageDistance = 0;	// meters
	};
	Array<ZoneView> zones;

	struct SlotView
	{
		WeakReference<ControllableContainer> slot;
		String name;
		Colour color;
		bool occupied = false;
		bool held = false;
		int personId = -1;
		Point<float> position;		// meters
		float holdRemaining = 0;	// seconds, -1 = held until reclaimed
	};
	Array<SlotView> slots;
	bool slotsEnabled = false;
	int slotReleaseMode = 0;	// PlayerSlots::ReleaseMode
	int freeSlots = 0;
	float reclaimRadius = 0;	// meters
	float holdTime = 0;			// seconds, 0 = until reclaimed
	int getSlotIndexForPerson(int id) const;	// -1 = no slot

	Array<int> personZones;	// zone index for each person of the snapshot, -1 = in no zone
	Array<Point<float>> personRelativePositions;	// for each person of the snapshot, from Values > People, -1 = in no zone
	int selectedPersonId = -1;

	//motion measured here from the successive positions (independent of the source velocity units)
	struct Motion
	{
		Point<float> lastPosition;	// meters
		uint32 lastMs = 0;
		Point<float> velocity;		// meters per second, smoothed
		Array<Point<float>> trail;	// recent positions in meters, oldest first
		uint32 lastTrailMs = 0;
	};
	std::map<int, Motion> motions;	// by person id

	static constexpr float minArrowSpeed = .15f;	// m/s, below that the person is considered still
	static const int trailStepMs = 100;			// one trail point every 100 ms...
	static const int trailLength = 20;			// ...kept for 2 s

	void update(SpatialModule* sm);	// nullptr : no module, everything empty
	static Colour getPersonColor(int id);

private:
	void updateMotions();
};

class TopViewRenderer
{
public:
	struct Options
	{
		Colour background = TrackingMonitorStyle::sceneBg;
		bool grid = true;
		bool zoneNames = true;
		bool personIds = true;
		bool trails = true;
		bool slots = true;		// slot label on each person
		bool heldSpots = true;
		bool selection = false;	// selected zone and person highlighted (editing only)
	};

	//scene : the scene (0,0 to its size in meters) on screen ; clip : the area that may be painted
	//zoneOverlay : called after each zone (editing handles) ; afterZones : called once all zones are drawn
	static void paint(Graphics& g, const TopViewModel& m, Rectangle<float> scene, float scale, Rectangle<float> clip, const Options& o,
		std::function<void(int zoneIndex, Rectangle<float> zoneRect)> zoneOverlay = nullptr,
		std::function<void()> afterZones = nullptr);
};
