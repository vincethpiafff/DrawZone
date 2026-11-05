/*
  ==============================================================================

	SpatialScene.h
	Created: 5 Nov 2026
	Author:  DrawZone

	A scene is a flat tracked area (a floor, a wall...) : its own 2D meters
	(origin top left, like the top view), its size, and where it stands in
	the 3D view. Sources feed a scene, zones belong to a scene.
	3D : world = Position + Rotation (X, then Y, then Z) x (x, y, height),
	world z up. A wall standing at the back of a floor : Rotation X -90,
	Position Z = its height.

  ==============================================================================
*/

#pragma once

class SpatialScene :
	public BaseItem
{
public:
	SpatialScene(var params = var());
	~SpatialScene();

	enum SizeMode { SIZE_AUTO, SIZE_MANUAL };
	EnumParameter* sizeMode;
	Point2DParameter* manualSize;	// meters
	Point3DParameter* position;		// meters
	Point3DParameter* rotation;		// degrees
	StringParameter* status;		// read only

	Point<float> size;	// meters, 0 when unknown ; message thread, set by the module

	void onContainerParameterChangedInternal(Parameter* p) override;

	String getTypeString() const override { return "Scene"; }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialScene)
};

class SpatialSceneManager :
	public SpatialSectionManager<SpatialScene>
{
public:
	SpatialSceneManager();
	~SpatialSceneManager();

	void addItemInternal(SpatialScene* item, var data) override;

	//the scene a source or a zone points to ; an empty or broken link : the first scene (nullptr when none)
	SpatialScene* getScene(TargetParameter* target);

	//"Scene" parameter of a source or a zone : picks a scene of this manager
	void setupTarget(TargetParameter* target);
};
