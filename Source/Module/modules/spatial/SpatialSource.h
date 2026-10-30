/*
  ==============================================================================

	SpatialSource.h
	Created: 28 Oct 2026
	Author:  DrawZone

	A tracking system feeding the Spatial module (Augmenta OSC, PSN, and
	TUIO next). Each source places its people in the scene with its own
	transform :
	  scene = rotate(scale x flip(source meters)) + offset
	Flip mirrors within the source area when it has one (Augmenta scene),
	around its origin otherwise.

	Sources live in the module's Sources manager. The OSC thread uses them
	under the manager lock (items.getLock()), so a source is never deleted
	while it parses a message. A PSN source owns its thread and stops it
	before being removed.

  ==============================================================================
*/

#pragma once

#include "psn_lib.hpp"

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
	BoolParameter* useAsScene;	// the scene size comes from this source, only one at a time

	void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChangedInternal(Parameter* p) override;

	//source meters -> scene meters ; area = size of the source area, 0 when it has none
	Point<float> toScene(Point<float> sourceMeters, Point<float> area) const;
	Point<float> vectorToScene(Point<float> v) const;	// velocity, flipped by sign
	float angleToScene(float degrees) const;			// orientation

	//message thread, at the status rate (SpatialModule::statusUpdateRateHz) : Status text
	virtual void update(SpatialModule* module);

	//size of the scene this source knows, in meters, 0 when none (call under the manager lock)
	virtual Point<float> getSceneArea() { return Point<float>(); }

	//"No data yet", "Silent" or "Receiving, 50 msg/s"
	String getReceptionText(SpatialModule* module) const;

protected:
	//Inspector order for every source : scene, placement in the scene, reception, status
	void moveParametersToEnd(const Array<Controllable*>& parameters);

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
	Point<float> getSceneArea() override { return augmentaSceneSize; }

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Augmenta"; }

private:
	void processPerson(const OSCMessage& msg, SpatialModule* module);
};

//PosiStageNet, received on its own multicast group by its own thread
class PSNSource :
	public SpatialSource,
	public Thread
{
public:
	PSNSource(var params = var());
	~PSNSource();

	enum UpAxis { UP_Y, UP_Z };

	StringParameter* multicastAddress;
	IntParameter* multicastPort;
	EnumParameter* upAxis;
	BoolParameter* ignoreOrigin;

	//PSN sends no scene size : entered by hand, from the tracking system (Kratos : Area > Position and Scale)
	Point2DParameter* areaPosition;
	Point2DParameter* areaSize;

	void clearItem() override;
	void onContainerParameterChangedInternal(Parameter* p) override;
	void update(SpatialModule* module) override;	// also starts and stops the thread
	void run() override;
	Point<float> getSceneArea() override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "PSN"; }

private:
	enum Connection { CONNECTING, BIND_FAILED, JOIN_FAILED, CONNECTED };
	std::atomic<Connection> connection { CONNECTING };
	SpatialModule* threadModule = nullptr;	// set before the thread starts, never changes

	CriticalSection systemNameLock;
	String systemName;	// from the PSN info packets

	std::map<int, uint32> firstSeenMs;	// by tracker id, thread only, gives the age

	void processFrame(const psn::tracker_map& trackers);
	Point<float> toGround(const psn::float3& v, bool upY) const;	// top view, not mirrored, in source meters
	Point<float> getAreaOrigin(bool upY);	// top left of the area in toGround() meters
	bool joinOnAllInterfaces(DatagramSocket& socket, const String& address);
};

class SpatialSourceManager :
	public SpatialSectionManager<SpatialSource>
{
public:
	SpatialSourceManager();
	~SpatialSourceManager();

	Factory<SpatialSource> factory;

	AugmentaSource* getAugmentaSource();	// first enabled one, call under items.getLock()
	//the enabled source marked Use As Scene, else the only one knowing its scene : with several, it's up to the user. Call under items.getLock()
	Point<float> getSceneArea();
};
