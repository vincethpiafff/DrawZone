/*
  ==============================================================================

	SpatialSurface.h
	Created: 5 Nov 2026
	Author:  DrawZone

	A surface is a flat tracked area (a floor, a wall...) : its own 2D meters
	(origin top left, like the top view), its size, and where it stands in
	the 3D view. Sources feed a surface, zones belong to a surface. Each
	surface has its own player slots and its own video output, named like it.
	3D : world = Position + Rotation (X, then Y, then Z) x (x, y, height),
	world z up. A wall standing at the back of a floor : Rotation X -90,
	Position Z = its height.

  ==============================================================================
*/

#pragma once

class SpatialModule;

class SpatialSurface :
	public BaseItem
{
public:
	SpatialSurface(SpatialModule* module = nullptr);	// nullptr : only for the generic manager code, the manager always gives the module
	~SpatialSurface();

	enum SizeMode { SIZE_AUTO, SIZE_MANUAL };
	EnumParameter* sizeMode;
	Point2DParameter* manualSize;	// meters
	Point3DParameter* position;		// meters
	Point3DParameter* rotation;		// degrees
	StringParameter* status;		// read only

	Point<float> size;	// meters, 0 when unknown ; message thread, set by the module

	PlayerSlots playerSlots;			// its players, slot 1.. ; values in Values > Slots > <surface>
	SpatialVideoOutput videoOutput;	// this surface sent as video, named like it

	void onContainerParameterChangedInternal(Parameter* p) override;
	void onContainerNiceNameChanged() override;	// slot values and video stream follow the name

	String getTypeString() const override { return "Surface"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialSurface)
};

class SpatialSurfaceManager :
	public SpatialSectionManager<SpatialSurface>
{
public:
	SpatialSurfaceManager();
	~SpatialSurfaceManager();

	SpatialModule* module = nullptr;	// set by the module before anything is added
	SpatialSurface* createItem() override { return new SpatialSurface(module); }

	void addItemInternal(SpatialSurface* item, var data) override;

	//the surface a source or a zone points to ; an empty or broken link : the first surface (nullptr when none)
	SpatialSurface* getSurface(TargetParameter* target);

	//"Surface" parameter of a source or a zone : picks a surface of this manager
	void setupTarget(TargetParameter* target);
};
