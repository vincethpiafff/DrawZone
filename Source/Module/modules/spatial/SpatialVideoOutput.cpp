/*
  ==============================================================================

	SpatialVideoOutput.cpp
	Created: 28 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialVideoOutput::SpatialVideoOutput(SpatialModule* module) :
	ControllableContainer("Video Output"),
	display("Display"),
	module(module)
{
	spout = addBoolParameter("Spout", "Send the top view (zones, people, player slots) as a Spout texture, for video software on this computer (Smode, Resolume, TouchDesigner...). Needs Use OpenGL Renderer in the preferences", false);
	ndi = addBoolParameter("NDI", "Send the top view as an NDI source on the network (Smode, Resolume, TouchDesigner, OBS...). Needs the NDI runtime", false);
	streamName = addStringParameter("Stream Name", "Name of the Spout sender and of the NDI source", "DrawZone");

	outputArea = addEnumParameter("Output Area", "Part of the floor in the image. Scene : exactly the scene, its size is Scene x Pixels Per Meter. Custom Area : Area Origin and Area Size, in meters. Fixed Resolution : the whole scene, centered in Width x Height");
	outputArea->addOption("Scene", SCENE)->addOption("Custom Area", CUSTOM_AREA)->addOption("Fixed Resolution", FIXED_RESOLUTION);

	pixelsPerMeter = addFloatParameter("Pixels Per Meter", "Scale of the image : number of pixels for one meter (100 = 1 px per cm)", 100, 1, 1000);
	areaOrigin = addPoint2DParameter("Area Origin", "Custom Area : top left corner of the area, in meters from the top left corner of the scene (negative = margin around the scene)");
	areaSize = addPoint2DParameter("Area Size", "Custom Area : width and height of the area, in meters");
	areaSize->setDefaultPoint(10, 6);
	width = addIntParameter("Width", "Fixed Resolution : width of the image, in pixels", 1920, 16, maxSize);
	height = addIntParameter("Height", "Fixed Resolution : height of the image, in pixels", 1080, 16, maxSize);
	fps = addFloatParameter("FPS", "Images sent per second", 30, 1, 120);

	resolution = addStringParameter("Resolution", "Size of the image sent and its scale", "");
	resolution->setControllableFeedbackOnly(true);
	resolution->isSavable = false;

	showGrid = display.addBoolParameter("Grid", "Draw the 1 m grid and the scene border", true);
	showZoneNames = display.addBoolParameter("Zone Names", "Draw the name of each zone", true);
	showPersonIds = display.addBoolParameter("Person Ids", "Draw the id of each person", true);
	showTrails = display.addBoolParameter("Trails", "Draw the recent path of each person", true);
	showSlotLabels = display.addBoolParameter("Slot Labels", "Draw the player slot name next to each person who has one", true);
	showHeldSpots = display.addBoolParameter("Held Spots", "Draw the spots held for a player slot (Hold spot mode)", true);
	background = display.addColorParameter("Background", "Color behind the zones and people", Colours::black);
	transparentBackground = display.addBoolParameter("Transparent Background", "No background : the image keeps its transparency, to put the view over another image", false);
	addChildControllableContainer(&display);

	updateParametersState();
}

SpatialVideoOutput::~SpatialVideoOutput()
{
	stopTimer();
	releaseSpout();
	ndiSender = nullptr;
}

void SpatialVideoOutput::onContainerParameterChanged(Parameter* p)
{
	if (p == spout || p == ndi || p == streamName || p == fps) updateSender();
	if (p == outputArea) updateParametersState();
}

void SpatialVideoOutput::onControllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
	ControllableContainer::onControllableFeedbackUpdate(cc, c);
	if (c == transparentBackground) updateParametersState();
}

void SpatialVideoOutput::updateParametersState()
{
	background->setEnabled(!transparentBackground->boolValue());

	OutputArea area = outputArea->getValueDataAsEnum<OutputArea>();
	pixelsPerMeter->setEnabled(area != FIXED_RESOLUTION);
	areaOrigin->setEnabled(area == CUSTOM_AREA);
	areaSize->setEnabled(area == CUSTOM_AREA);
	width->setEnabled(area == FIXED_RESOLUTION);
	height->setEnabled(area == FIXED_RESOLUTION);
}

OpenGLContext* SpatialVideoOutput::getGLContext() const
{
	auto* mc = getApp().mainComponent.get();
	return mc != nullptr ? mc->openGLContext.get() : nullptr;
}

void SpatialVideoOutput::updateSender()
{
	if (spout->boolValue())
	{
#if JUCE_WINDOWS
		if (spoutState == nullptr)
		{
			spoutState = std::make_shared<SpoutState>();
			spoutState->lib = GetSpout();
		}
#endif
	}
	else releaseSpout();

	if (ndi->boolValue())
	{
		if (ndiSender == nullptr) ndiSender.reset(new NDISender());
	}
	else ndiSender = nullptr;

	if (spout->boolValue() || ndi->boolValue()) startTimerHz(jmax(1, roundToInt(fps->floatValue())));
	else
	{
		stopTimer();
		resolution->setValue("");
	}
}

void SpatialVideoOutput::releaseSpout()
{
	if (spoutState == nullptr) return;

#if JUCE_WINDOWS
	//the sender holds GL resources : released on the GL thread, after the frames already queued
	std::shared_ptr<SpoutState> state = spoutState;
	if (OpenGLContext* gl = getGLContext())
	{
		gl->executeOnGLThread([state](OpenGLContext&)
			{
				if (state->lib == nullptr) return;
				state->lib->ReleaseSender();
				state->lib->Release();
				state->lib = nullptr;
			}, false);
	}
	else if (state->lib != nullptr)
	{
		state->lib->Release();
		state->lib = nullptr;
	}
#endif

	spoutState = nullptr;
}

bool SpatialVideoOutput::computeGeometry(Point<float> sceneSize, int& w, int& h, Rectangle<float>& scene, float& scale) const
{
	if (sceneSize.x <= 0 || sceneSize.y <= 0) return false;

	switch (outputArea->getValueDataAsEnum<OutputArea>())
	{
	case CUSTOM_AREA:
	{
		scale = pixelsPerMeter->floatValue();
		Point<float> size = areaSize->getPoint();
		if (size.x <= 0 || size.y <= 0) return false;
		w = roundToInt(size.x * scale);
		h = roundToInt(size.y * scale);
		Point<float> origin = areaOrigin->getPoint();
		scene = Rectangle<float>(-origin.x * scale, -origin.y * scale, sceneSize.x * scale, sceneSize.y * scale);
		break;
	}

	case FIXED_RESOLUTION:
		w = width->intValue();
		h = height->intValue();
		scale = jmin(w / sceneSize.x, h / sceneSize.y);
		scene = Rectangle<float>(sceneSize.x * scale, sceneSize.y * scale).withCentre(Point<float>(w / 2.0f, h / 2.0f));
		break;

	default: //Scene
		scale = pixelsPerMeter->floatValue();
		w = roundToInt(sceneSize.x * scale);
		h = roundToInt(sceneSize.y * scale);
		scene = Rectangle<float>(sceneSize.x * scale, sceneSize.y * scale);
		break;
	}

	//too big : the same area with fewer pixels
	if (w > maxSize || h > maxSize)
	{
		float ratio = jmin((float)maxSize / w, (float)maxSize / h);
		w = roundToInt(w * ratio);
		h = roundToInt(h * ratio);
		scale *= ratio;
		scene = Rectangle<float>(scene.getX() * ratio, scene.getY() * ratio, scene.getWidth() * ratio, scene.getHeight() * ratio);
	}

	w = jmax(16, w);
	h = jmax(16, h);
	return true;
}

void SpatialVideoOutput::timerCallback()
{
	Image image = renderFrame();
	if (!image.isValid()) return;
	if (spout->boolValue()) sendSpout(image);
	if (ndiSender != nullptr) ndiSender->pushFrame(image, transparentBackground->boolValue(), streamName->stringValue().trim(), fps->floatValue());
}

Image SpatialVideoOutput::renderFrame()
{
	model.update(module);

	int w = 0, h = 0;
	Rectangle<float> scene;
	float scale = 0;
	if (!computeGeometry(model.snapshot.sceneSize, w, h, scene, scale))
	{
		resolution->setValue(SpatialModule::getNoSceneSizeText());
		return Image();
	}

	bool transparent = transparentBackground->boolValue();
	Colour bg = transparent ? Colours::transparentBlack : background->getColor().withAlpha(1.0f);

	Image image(Image::ARGB, w, h, true, SoftwareImageType());
	{
		Graphics g(image);
		g.fillAll(bg);
		TopViewRenderer::Options o;
		o.background = bg;
		o.grid = showGrid->boolValue();
		o.zoneNames = showZoneNames->boolValue();
		o.personIds = showPersonIds->boolValue();
		o.trails = showTrails->boolValue();
		o.slots = showSlotLabels->boolValue();
		o.heldSpots = showHeldSpots->boolValue();
		TopViewRenderer::paint(g, model, scene, scale, image.getBounds().toFloat(), o);
	}

	//JUCE images are premultiplied, Spout and NDI receivers expect straight alpha
	if (transparent)
	{
		Image::BitmapData data(image, Image::BitmapData::readWrite);
		for (int y = 0; y < data.height; y++)
		{
			PixelARGB* line = (PixelARGB*)data.getLinePointer(y);
			for (int x = 0; x < data.width; x++) line[x].unpremultiply();
		}
	}

	String cm = String(100.0f / scale, 2).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
	resolution->setValue(String(w) + " x " + String(h) + " px, 1 px = " + cm + " cm"
		+ (spout->boolValue() && getGLContext() == nullptr ? " (Spout needs Use OpenGL Renderer)" : "")
		+ (ndiSender != nullptr && ndiSender->getError().isNotEmpty() ? " (" + ndiSender->getError() + ")" : ""));

	return image;
}

void SpatialVideoOutput::sendSpout(const Image& image)
{
#if JUCE_WINDOWS
	OpenGLContext* gl = getGLContext();
	if (gl == nullptr || spoutState == nullptr || spoutState->lib == nullptr) return;

	//the GL thread is late : drop this frame rather than queueing them
	if (spoutState->frameQueued.exchange(true)) return;

	std::shared_ptr<SpoutState> state = spoutState;
	String name = streamName->stringValue();
	gl->executeOnGLThread([state, image, name](OpenGLContext&)
		{
			state->frameQueued = false;
			if (state->lib == nullptr) return;

			//one sender, under our name (renamed : the old one is closed first)
			if (state->name != name)
			{
				if (state->name.isNotEmpty()) state->lib->ReleaseSender();
				state->lib->SetSenderName(name.toRawUTF8());
				state->name = name;
			}

			//ARGB image = B, G, R, A bytes, rows from the top
			Image::BitmapData data(image, Image::BitmapData::readOnly);
			if (data.lineStride != data.width * 4) return;
			state->lib->SendImage(data.data, (unsigned int)data.width, (unsigned int)data.height, 0x80E1 /* GL_BGRA_EXT */, false);
		}, false);

	gl->triggerRepaint(); //the interface context only runs its jobs when it renders
#else
	ignoreUnused(image);
#endif
}

// NDISender

SpatialVideoOutput::NDISender::NDISender() :
	Thread("DrawZone NDI sender")
{
	startThread(Priority::low);
}

SpatialVideoOutput::NDISender::~NDISender()
{
	//run() destroys the sender before returning
	signalThreadShouldExit();
	signal.notify_all();
	stopThread(-1);
}

void SpatialVideoOutput::NDISender::pushFrame(const Image& image, bool alpha, const String& name, float fps)
{
	{
		const std::lock_guard<std::mutex> l(lock);
		pending = image;
		pendingAlpha = alpha;
		wantedName = name;
		frameRate = fps;
	}
	signal.notify_one();
}

String SpatialVideoOutput::NDISender::getError()
{
	const std::lock_guard<std::mutex> l(lock);
	return error;
}

void SpatialVideoOutput::NDISender::run()
{
	while (!threadShouldExit())
	{
		Image image;
		bool alpha = false;
		String name;
		float rate = 30;
		{
			std::unique_lock<std::mutex> l(lock);
			signal.wait(l, [this] { return pending.isValid() || threadShouldExit(); });
			if (threadShouldExit()) break;
			image = pending;
			alpha = pendingAlpha;
			pending = Image();
			name = wantedName;
			rate = frameRate;
		}

		//the source name is fixed at creation : a new name = a new sender
		if (sender == nullptr || senderName != name)
		{
			destroySender();
			if (name.isEmpty()) continue;

			String e;
			if (!VideoNetworkRuntime::NDI::get().load()) e = "NDI runtime not found";
			else
			{
				const CharPointer_UTF8 nameUtf8 = name.toUTF8();
				NDIlib_send_create_t settings;
				settings.p_ndi_name = nameUtf8.getAddress();
				settings.p_groups = nullptr;
				settings.clock_video = false; //paced by our timer
				settings.clock_audio = false;
				sender = VideoNetworkRuntime::NDI::get().send_create(&settings);
				if (sender == nullptr) e = "could not create the NDI source";
				else senderName = name;
			}

			{
				const std::lock_guard<std::mutex> l(lock);
				error = e;
			}
			if (sender == nullptr) continue;
		}

		//nobody receives : nothing to send
		if (VideoNetworkRuntime::NDI::get().send_get_no_connections(sender, 0) == 0) continue;

		//ARGB image = B, G, R, A bytes, rows from the top ; opaque (BGRX) unless the background is transparent
		const Image::BitmapData data(image, Image::BitmapData::readOnly);
		const int frameRateN = jmax(1000, roundToInt(rate * 1000.0f));
		NDIlib_video_frame_v2_t frame(data.width, data.height,
			alpha ? NDIlib_FourCC_video_type_BGRA : NDIlib_FourCC_video_type_BGRX,
			frameRateN, 1000,
			(float)data.width / (float)data.height,
			NDIlib_frame_format_type_progressive,
			NDIlib_send_timecode_synthesize,
			(uint8*)data.getLinePointer(0),
			data.lineStride);

		//synchronous : the image must stay alive until the call returns
		VideoNetworkRuntime::NDI::get().send_send_video_v2(sender, &frame);
	}

	destroySender();
}

void SpatialVideoOutput::NDISender::destroySender()
{
	if (sender == nullptr) return;
	VideoNetworkRuntime::NDI::get().send_destroy(sender);
	sender = nullptr;
	senderName.clear();
}
