/*
  ==============================================================================

	SpatialVideoOutput.h
	Created: 28 Oct 2026
	Author:  DrawZone

	Video output of the top view (zones, people, player slots), drawn by
	TopViewRenderer into an image at a fixed rate, and sent with Spout
	and / or NDI.

	Spout : our own Spout library instance, fed with the image pixels
	(SendImage) on the interface OpenGL thread. Chataigne's shared texture
	sender is not used : it opens a 2.006 sender (CreateSender) that stays
	black next to the 2.007 one that receives the image (two sources).

	NDI : the runtime loaded by Chataigne's NDI out module
	(VideoNetworkRuntime). Sending blocks, so it runs on its own thread,
	which takes the latest image ; nothing is sent while nobody receives.

	Scale : pixel = (meters - area origin) x Pixels Per Meter.
	Output Area : Scene (the image is exactly the scene), Custom Area (origin
	and size in meters) or Fixed Resolution (the whole scene, centered).

  ==============================================================================
*/

#pragma once

#include "../video/VideoNetworkRuntime.h"

class SpatialModule;

class SpatialVideoOutput :
	public ControllableContainer,
	private Timer
{
public:
	SpatialVideoOutput(SpatialModule* module);
	~SpatialVideoOutput();

	enum OutputArea { SCENE, CUSTOM_AREA, FIXED_RESOLUTION };

	BoolParameter* spout;
	BoolParameter* ndi;
	StringParameter* streamName;
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

private:
	SpatialModule* module;
	TopViewModel model;

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

	//image size and the scene rectangle in it, from the Output Area ; false when the scene size is unknown
	bool computeGeometry(Point<float> sceneSize, int& w, int& h, Rectangle<float>& scene, float& scale) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialVideoOutput)
};
