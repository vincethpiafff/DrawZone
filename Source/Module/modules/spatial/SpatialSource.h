/*
  ==============================================================================

	SpatialSource.h
	Created: 28 Oct 2026
	Author:  DrawZone

	A tracking system feeding the Spatial module (Augmenta OSC for now,
	PSN and TUIO next). Each source places its people in the scene with
	its own transform :
	  scene = rotate(scale x flip(source meters)) + offset
	Flip mirrors within the source area when it has one (Augmenta scene),
	around its origin otherwise.

	Sources live in the module's Sources manager. Network threads use them
	under the manager lock (items.getLock()), so a source is never deleted
	while it parses a message.

  ==============================================================================
*/

#pragma once

class SpatialModule;

class SpatialSource :
	public BaseItem
{
public:
	SpatialSource(const String& name = "Source", var params = var());
	virtual ~SpatialSource();

	Point2DParameter* offset;	// meters
	FloatParameter* rotation;	// degrees, clockwise
	FloatParameter* scale;
	BoolParameter* flipX;
	BoolParameter* flipY;
	StringParameter* status;	// read only
	Trigger* resetParameters;	// transform back to identity

	void onContainerTriggerTriggered(Trigger* t) override;

	//source meters -> scene meters ; area = size of the source area, 0 when it has none
	Point<float> toScene(Point<float> sourceMeters, Point<float> area) const;
	Point<float> vectorToScene(Point<float> v) const;	// velocity, flipped by sign
	float angleToScene(float degrees) const;			// orientation

	//message thread, at the zone rate
	virtual void update(SpatialModule* module);

	//"No data yet", "Silent" or "Receiving, 50 msg/s"
	String getReceptionText(SpatialModule* module) const;

private:
	Point<float> scaleAndRotate(Point<float> v) const;	// no flip, no offset
};

//Augmenta OSC protocol v2, received on the module's OSC input
class AugmentaSource :
	public SpatialSource
{
public:
	AugmentaSource(var params = var());
	~AugmentaSource();

	Point<float> augmentaSceneSize;	// meters, from /scene ; written and read under the manager lock
	std::atomic<bool> isUsed { false };	// the first enabled Augmenta source gets the messages

	//network thread, under the manager lock
	void processMessage(const OSCMessage& msg, SpatialModule* module);
	void update(SpatialModule* module) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Augmenta"; }

private:
	void processPerson(const OSCMessage& msg, SpatialModule* module);
};

class SpatialSourceManager :
	public SpatialSectionManager<SpatialSource>
{
public:
	SpatialSourceManager();
	~SpatialSourceManager();

	Factory<SpatialSource> factory;

	AugmentaSource* getAugmentaSource();	// first enabled one, call under items.getLock()
};
