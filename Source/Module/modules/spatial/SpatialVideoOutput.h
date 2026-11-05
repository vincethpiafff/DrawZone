/*
  ==============================================================================

	SpatialVideoOutput.h
	Created: 28 Oct 2026
	Author:  DrawZone

	Video output of one surface seen from above (zones, people, player slots),
	one per surface, named like it ; drawn by TopViewRenderer into an image at a fixed rate, and sent with Spout
	and / or NDI.

	Spout : our own Spout library instance, fed with the image pixels
	(SendImage) on the interface OpenGL thread. Chataigne's shared texture
	sender is not used : it opens a 2.006 sender (CreateSender) that stays
	black next to the 2.007 one that receives the image (two sources).

	NDI : the runtime loaded by Chataigne's NDI out module
	(VideoNetworkRuntime). Sending blocks, so it runs on its own thread,
	which takes the latest image ; nothing is sent while nobody receives.

	Scale : pixel = (meters - area origin) x Pixels Per Meter.
	Output Area : Surface (the image is exactly the surface), Custom Area (origin
	and size in meters) or Fixed Resolution (the whole surface, centered).

  ==============================================================================
*/

#pragma once

#include "../video/VideoNetworkRuntime.h"

class SpatialModule;
class SpatialSurface;

class SpatialVideoOutput :
	public ControllableContainer,
	private Timer
{
public:
	SpatialVideoOutput(SpatialModule* module, SpatialSurface* surface);
	~SpatialVideoOutput();

	enum OutputArea { SURFACE, CUSTOM_AREA, FIXED_RESOLUTION };

	BoolParameter* spout;
	BoolParameter* ndi;
	EnumParameter* outputArea;
	FloatParameter* pixelsPerMeter;
	Point2DParameter* areaOrigin;	// meters, Custom Area
	Point2DParameter* areaSize;		// meters, Custom Area
	IntParameter* width;			// Fixed Resolution
	IntParameter* height;
	FloatParameter* fps;
	StringParameter* resolution;	// read only : "1000 x 600 px, 1 px = 1 cm"

	//what is drawn in the image (the Tracking Monitor is not affected)
	ControllableContainer display;
	BoolParameter* showGrid;
	BoolParameter* showZoneNames;
	BoolParameter* showPersonIds;
	BoolParameter* showTrails;
	BoolParameter* showSlotLabels;
	BoolParameter* showHeldSpots;
	ColorParameter* background;
	BoolParameter* transparentBackground;	// alpha sent, not premultiplied (Spout and NDI expect straight alpha)

	static const int maxSize = 8192;

	void onContainerParameterChanged(Parameter* p) override;
	void onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c) override;

	String getStreamName() const;	// the name of its surface, for Spout and NDI

private:
	SpatialModule* module;
	SpatialSurface* surface;	// owns this output
	TopViewModel model;
	uint32 lastResolutionTextMs = 0;

	//two frames reused in turn instead of a new image (about 8 MB in 1080p) at each frame
	Image framePool[2];
	Image getFrameImage(int w, int h);

	//Spout sender, used on the GL thread only ; shared with the queued GL jobs so it outlives this object
	struct SpoutState
	{
#if JUCE_WINDOWS
		SPOUTLIBRARY* lib = nullptr;
#endif
		String name;	// name given to the library, empty = not set yet
		std::atomic<bool> frameQueued { false };	// one frame at a time on the GL thread
	};
	std::shared_ptr<SpoutState> spoutState;

	//NDI sender, on its own thread : sending a frame blocks
	class NDISender :
		public Thread
	{
	public:
		NDISender();
		~NDISender();

		void pushFrame(const Image& image, bool alpha, const String& name, float fps);	// message thread, keeps the latest image only
		String getError();

	private:
		std::mutex lock;
		std::condition_variable signal;
		Image pending;
		bool pendingAlpha = false;
		String wantedName;
		float frameRate = 30;
		String error;

		NDIlib_send_instance_t sender = nullptr;	// worker thread only
		String senderName;

		void run() override;
		void destroySender();
	};
	std::unique_ptr<NDISender> ndiSender;

	OpenGLContext* getGLContext() const;	// the interface context, nullptr without Use OpenGL Renderer
	void updateSender();
	void releaseSpout();
	void updateParametersState();
	void timerCallback() override;
	Image renderFrame();
	void sendSpout(const Image& image);

	//image size and the surface rectangle in it, from the Output Area ; false when the surface size is unknown
	bool computeGeometry(Point<float> surfaceSize, int& w, int& h, Rectangle<float>& surfaceRect, float& scale) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialVideoOutput)
};
