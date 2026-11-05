/*
  ==============================================================================

	SpatialSource.h
	Created: 28 Oct 2026
	Author:  DrawZone

	A tracking system feeding the Spatial module (Augmenta OSC, PSN). Each
	source places its people in the surface with its own
	transform :
	  surface = rotate(scale x flip(source meters)) + offset
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
	SpatialSource(const String& name = "Source", var params = var(), int defaultIdOffset = 0);
	virtual ~SpatialSource();

	TargetParameter* surfaceTarget;	// surface fed by this source, empty = the first one
	std::atomic<const void*> surfaceId { nullptr };	// that surface, set by the module (message thread), read by the network threads

	Point2DParameter* offset;	// meters
	FloatParameter* rotation;	// degrees, clockwise
	FloatParameter* scale;
	BoolParameter* flipX;
	BoolParameter* flipY;
	StringParameter* status;	// read only
	Trigger* resetParameters;	// transform back to identity
	BoolParameter* useAsSurface;	// the surface size comes from this source, only one at a time
	IntParameter* idOffset;		// added to the ids of this source

	void onContainerTriggerTriggered(Trigger* t) override;
	void onContainerParameterChangedInternal(Parameter* p) override;

	//source meters -> surface meters ; area = size of the source area, 0 when it has none
	Point<float> toSurface(Point<float> sourceMeters, Point<float> area) const;
	Point<float> vectorToSurface(Point<float> v) const;	// velocity, flipped by sign
	float angleToSurface(float degrees) const;			// orientation

	//message thread, at the status rate (SpatialModule::statusUpdateRateHz) : Status text
	virtual void update(SpatialModule* module);

	//size of the surface this source knows, in meters, 0 when none (call under the manager lock)
	virtual Point<float> getSurfaceArea() { return Point<float>(); }

	//"No data yet", "Silent" or "Receiving, 50 msg/s"
	String getReceptionText(SpatialModule* module) const;

	//warning for the Status text, empty when every person has its id
	String getIdConflictText(SpatialModule* module) const;

	//name for the network threads : niceName itself may change while they copy it
	String getNameForThreads() const;
	void onContainerNiceNameChanged() override;

protected:
	//Inspector order for every source : surface, placement in the surface, reception, status
	void moveParametersToEnd(const Array<Controllable*>& parameters);

private:
	Point<float> scaleAndRotate(Point<float> v) const;	// no flip, no offset
	mutable CriticalSection nameLock;
	String nameForThreads;
};

//Augmenta OSC protocol v2, one Fusion scene per port. Use Module Input : the first enabled such source gets the
//module's OSC input (sessions before v0.6) ; otherwise the source listens to its own Port
class AugmentaSource :
	public SpatialSource,
	private OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
{
public:
	AugmentaSource(var params = var());
	~AugmentaSource();

	BoolParameter* useModuleInput;	// fed by the module's OSC Input, Port unused
	IntParameter* port;				// own port, one Fusion scene per port

	std::atomic<bool> isUsed { false };	// gets the module's OSC input

	//network thread : module input (under the manager lock) or own receiver
	void processMessage(const OSCMessage& msg, SpatialModule* module);
	void clearItem() override;
	void update(SpatialModule* module) override;	// also opens and closes the own receiver
	void onContainerParameterChangedInternal(Parameter* p) override;
	Point<float> getSurfaceArea() override;
	String getPortText(int modulePort) const { return " :" + String(useModuleInput->boolValue() ? modulePort : port->intValue()); }

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Augmenta"; }

private:
	CriticalSection surfaceLock;
	Point<float> augmentaSceneSize;	// meters, from /scene

	//own receiver, when Use Module Input is off
	OSCReceiver receiver;
	int ownPort = 0;	// 0 = closed
	bool bindFailed = false;
	int triedPort = 0;	// last port tried by the own receiver
	SpatialModule* receiverModule = nullptr;	// set before the receiver opens, never changes after
	void openReceiver(SpatialModule* module, int p);
	void closeReceiver();
	void oscMessageReceived(const OSCMessage& msg) override;
	void oscBundleReceived(const OSCBundle& bundle) override;

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

	//PSN sends no surface size : entered by hand, from the tracking system (Kratos : Area > Position and Scale)
	Point2DParameter* areaPosition;
	Point2DParameter* areaSize;

	void clearItem() override;
	void onContainerParameterChangedInternal(Parameter* p) override;
	void update(SpatialModule* module) override;	// also starts and stops the thread
	void run() override;
	Point<float> getSurfaceArea() override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "PSN"; }

private:
	enum Connection { CONNECTING, BIND_FAILED, JOIN_FAILED, CONNECTED };
	std::atomic<Connection> connection { CONNECTING };
	SpatialModule* threadModule = nullptr;	// set before the thread starts, never changes

	CriticalSection systemNameLock;
	String systemName;	// from the PSN info packets

	std::map<int, uint32> firstSeenMs;	// by tracker id, thread only, gives the age

	void processFrame(const psn::tracker_map& trackers, const String& system);
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

	SpatialSurfaceManager* surfaceManager = nullptr;	// set by the module before anything is loaded
	SpatialSource* createItemFromData(var data) override;	// its Surface parameter picks among the surfaces before it loads
	void addItemInternal(SpatialSource* item, var data) override;

	//first enabled Augmenta source with Use Module Input, call under items.getLock()
	AugmentaSource* getAugmentaSource();
	//size of a surface from its sources : the enabled one marked Use As Surface, else the only one knowing its surface
	//(with several, it's up to the user). from : name of that source. Call under items.getLock()
	Point<float> getSurfaceArea(const void* surface, String* from = nullptr);
};
