/*
  ==============================================================================

	SpatialZone.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Interaction zone (rectangle, ellipse or free shape), defined in scene meters (origin top left).
	Its values (presence, person count) live in a container that the
	Spatial module shows in its Values, so they can be used anywhere
	in Chataigne (State Machine, Mappings...).

  ==============================================================================
*/

#pragma once

//one point of a free shape outline
class ZonePoint :
	public ControllableContainer
{
public:
	ZonePoint(int index);

	Point2DParameter* position;	// 0-1 in the Center + Size box of the zone, origin top left

	//curves : Linear = straight on both sides, Quadratic = one handle mirrored on both sides, Cubic = two independent handles
	enum Interpolation { LINEAR, QUADRATIC, CUBIC };
	EnumParameter* interpolation = nullptr;
	Point2DParameter* handleIn = nullptr;	// meters from the point, toward the previous point (Cubic only)
	Point2DParameter* handleOut = nullptr;	// meters from the point, toward the next point (Quadratic and Cubic)

	Interpolation getInterpolation() const;
	Point<float> getInOffset() const;	// effective, meters (0,0 when Linear)
	Point<float> getOutOffset() const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZonePoint)
};

class SpatialZone :
	public BaseItem
{
public:
	SpatialZone(var params = var());
	~SpatialZone();

	//bounds used until the tracking system sends its scene size
	static constexpr float maxMeters = 1000;

	void setSceneSize(Point<float> sceneSize);

	enum Shape { RECTANGLE, ELLIPSE, FREE_SHAPE };
	EnumParameter* shape;
	Point2DParameter* center;	// meters
	Point2DParameter* size;		// meters
	BoolParameter* hiddenInMonitor;
	IntParameter* capacity;
	FloatParameter* exitMargin;	// meters
	FloatParameter* exitDelay;	// seconds

	//free shape outline : saved by getJSONData / loadJSONDataInternal, the box (Center + Size) always holds it
	ControllableContainer pointsCC;
	OwnedArray<ZonePoint> freePoints;
	Array<Point<float>> getFreePointsMeters() const;
	void setFreePointsMeters(const Array<Point<float>>& meters);	// also fits Center and Size to the points
	void setFreePoints(const Array<Point<float>>& relative);		// 0-1 in the box
	const Path& getOutline() const;	// meters, free shape only ; kept until the box or a point changes
	Point<float> getHandleMeters(int index, bool out) const;	// curve control point of a free shape point, meters

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
	bool isEllipse() const;
	bool isFreeShape() const;
	Rectangle<float> getBounds() const;	// meters

	//box (meters) <-> 0-1 in the box, origin top left ; a flat box gives .5 on that axis
	static Point<float> toBox(Rectangle<float> box, Point<float> meters);
	static Point<float> fromBox(Rectangle<float> box, Point<float> relative);

	void updateFromPersons(const Array<TrackedPerson>& persons);

	void onContainerNiceNameChanged() override;
	void onContainerParameterChangedInternal(Parameter* p) override;
	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;	// a free shape point changed

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternal(var data) override;

	InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables = Array<Inspectable*>()) override;

	String getTypeString() const override { return "Zone"; }

private:
	//message thread only (zone updates and Monitor)
	mutable Path outlineCache;
	mutable bool outlineDirty = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialZone)
};

//Inspector of a zone : its parameters, then its live values (which live in the module Values), then its points (folded)
class SpatialZoneEditor :
	public BaseItemEditor
{
public:
	SpatialZoneEditor(SpatialZone* zone, bool isRoot);

	SpatialZone* zone;

	void resetAndBuild() override;
	void addValuesEditor();
};
