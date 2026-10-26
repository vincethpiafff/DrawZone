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

	ControllableContainer zoneValues;
	BoolParameter* presence;
	IntParameter* personCount;

	bool contains(Point<float> positionMeters) const;
	void updateFromPersons(const Array<TrackedPerson>& persons, Point<float> sceneSize);

	void onContainerNiceNameChanged() override;

	String getTypeString() const override { return "Zone"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialZone)
};
