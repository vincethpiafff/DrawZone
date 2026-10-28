/*
  ==============================================================================

	SpatialVideoOutput.cpp
	Created: 28 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialVideoOutput::SpatialVideoOutput(SpatialModule* module) :
	ControllableContainer("Video Output"),
	module(module)
{
	spout = addBoolParameter("Spout", "Send the top view (zones, people, player slots) as a Spout texture, for video software on this computer (Smode, Resolume, TouchDesigner...). Needs Use OpenGL Renderer in the preferences", false);
	streamName = addStringParameter("Stream Name", "Name of the Spout sender", "DrawZone");

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

	updateParametersState();
}

SpatialVideoOutput::~SpatialVideoOutput()
{
	stopTimer();
	releaseSpout();
}

void SpatialVideoOutput::onContainerParameterChanged(Parameter* p)
{
	if (p == spout || p == streamName || p == fps) updateSender();
	if (p == outputArea) updateParametersState();
}

void SpatialVideoOutput::updateParametersState()
{
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
		startTimerHz(jmax(1, roundToInt(fps->floatValue())));
	}
	else
	{
		stopTimer();
		releaseSpout();
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
	if (image.isValid() && spout->boolValue()) sendSpout(image);
}

Image SpatialVideoOutput::renderFrame()
{
	model.update(module);

	int w = 0, h = 0;
	Rectangle<float> scene;
	float scale = 0;
	if (!computeGeometry(model.snapshot.sceneSize, w, h, scene, scale))
	{
		resolution->setValue("Waiting for scene size (/scene)");
		return Image();
	}

	Image image(Image::ARGB, w, h, true, SoftwareImageType());
	{
		Graphics g(image);
		g.fillAll(Colours::black);
		TopViewRenderer::Options o;
		TopViewRenderer::paint(g, model, scene, scale, image.getBounds().toFloat(), o);
	}

	String cm = String(100.0f / scale, 2).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
	resolution->setValue(String(w) + " x " + String(h) + " px, 1 px = " + cm + " cm"
		+ (spout->boolValue() && getGLContext() == nullptr ? " (Spout needs Use OpenGL Renderer)" : ""));

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
