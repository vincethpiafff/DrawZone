/*
  ==============================================================================

	SpatialZone.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Rectangular interaction zone, defined in scene meters (origin top left).
	Its values (presence, person count) live in a container that the
	Spatial module shows in its Values, so they can be used anywhere
	in Chataigne (State Machine, Mappings...).

  ==============================================================================
*/

#pragma once

class SpatialZone :
	public BaseItem
{
public:
	SpatialZone(var params = var());
	~SpatialZone();

	//bounds used until the tracking system sends its scene size
	static constexpr float maxMeters = 1000;

	void setSceneSize(Point<float> sceneSize);

	Point2DParameter* center;	// meters
	Point2DParameter* size;		// meters
	BoolParameter* hiddenInMonitor;
	IntParameter* capacity;
	FloatParameter* exitMargin;	// meters
	FloatParameter* exitDelay;	// seconds

	ControllableContainer zoneValues;
	BoolParameter* presence;
	IntParameter* personCount;
	Trigger* entered;
	Trigger* exited;
	IntParameter* lastEnteredId;
	FloatParameter* presenceTime;
	FloatParameter* occupancy;

	//whole group in the zone (barycenter) ; values kept when the zone gets empty
	Point2DParameter* averagePosition = nullptr;		// 0-1 in the zone, origin top left
	FloatParameter* averageDistanceToCenter = nullptr;	// meters

	uint32 presenceStartMs = 0;	// when the zone became occupied

	Array<int> idsInside;	// people in the zone at the last update, to detect each entry and exit
	HashMap<int, uint32> lastInsideMs;	// last time each person of idsInside was really inside, for the exit delay

	bool contains(Point<float> positionMeters, float margin = 0) const;
	Rectangle<float> getBounds() const;	// meters
	void updateFromPersons(const Array<TrackedPerson>& persons, Point<float> sceneSize);

	void onContainerNiceNameChanged() override;

	String getTypeString() const override { return "Zone"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialZone)
};
