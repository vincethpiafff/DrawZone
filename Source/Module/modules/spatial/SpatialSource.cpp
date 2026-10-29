/*
  ==============================================================================

	SpatialSource.cpp
	Created: 28 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialSource::SpatialSource(const String& name, var params) :
	BaseItem(name)
{
	offset = addPoint2DParameter("Offset", "Where the origin of this source lands in the scene, in meters from the top left corner of the scene (applied last)");
	rotation = addFloatParameter("Rotation", "Rotation of this source around its origin, in degrees, clockwise", 0, -360, 360);
	scale = addFloatParameter("Scale", "Scale of this source (1 = its meters are scene meters)", 1, .01f, 100);
	flipX = addBoolParameter("Flip X", "Mirror this source left / right (within its area when it has one, around its origin otherwise)", false);
	flipY = addBoolParameter("Flip Y", "Mirror this source top / bottom (within its area when it has one, around its origin otherwise)", false);
	status = addStringParameter("Status", "What this source receives", "");
	status->setControllableFeedbackOnly(true);
	status->isSavable = false;
	resetParameters = addTrigger("Reset All Parameters", "Put Offset, Rotation, Scale and Flip back to their default values (the source as it is sent)");
}

SpatialSource::~SpatialSource()
{
}

void SpatialSource::onContainerTriggerTriggered(Trigger* t)
{
	BaseItem::onContainerTriggerTriggered(t);
	if (t != resetParameters) return;
	offset->resetValue();
	rotation->resetValue();
	scale->resetValue();
	flipX->resetValue();
	flipY->resetValue();
}

Point<float> SpatialSource::toScene(Point<float> p, Point<float> area) const
{
	if (flipX->boolValue()) p.x = area.x > 0 ? area.x - p.x : -p.x;
	if (flipY->boolValue()) p.y = area.y > 0 ? area.y - p.y : -p.y;
	return scaleAndRotate(p) + offset->getPoint();
}

Point<float> SpatialSource::vectorToScene(Point<float> v) const
{
	if (flipX->boolValue()) v.x = -v.x;
	if (flipY->boolValue()) v.y = -v.y;
	return scaleAndRotate(v);
}

Point<float> SpatialSource::scaleAndRotate(Point<float> v) const
{
	v *= scale->floatValue();
	float a = degreesToRadians(rotation->floatValue());
	float c = std::cos(a), s = std::sin(a);
	return Point<float>(v.x * c - v.y * s, v.x * s + v.y * c);
}

float SpatialSource::angleToScene(float degrees) const
{
	if (flipX->boolValue()) degrees = 180 - degrees;
	if (flipY->boolValue()) degrees = -degrees;
	return degrees + rotation->floatValue();
}

void SpatialSource::update(SpatialModule*)
{
}

String SpatialSource::getReceptionText(SpatialModule* module) const
{
	TrackingData::SourceStats s = module->trackingData.getSourceStats(this);
	if (s.lastMessageMs == 0) return "No data yet";
	if (Time::getMillisecondCounter() - s.lastMessageMs > SpatialModule::sourceTimeoutMs) return "Silent";
	return "Receiving, " + String(roundToInt(s.messagesPerSecond)) + " msg/s";
}

// AugmentaSource

AugmentaSource::AugmentaSource(var params) :
	SpatialSource(getTypeStringStatic(), params)
{
}

AugmentaSource::~AugmentaSource()
{
}

void AugmentaSource::processMessage(const OSCMessage& msg, SpatialModule* module)
{
	TrackingData& data = module->trackingData;
	data.countMessage(this);

	//Augmenta OSC protocol v2, see docs/drawzone/notes/augmenta-protocol.md
	String address = msg.getAddressPattern().toString();

	if (address == "/scene")
	{
		if (msg.size() < 4) return;
		augmentaSceneSize = Point<float>(OSCHelpers::getFloatArg(msg[2]), OSCHelpers::getFloatArg(msg[3]));
		if (module->isSceneSizeAuto()) data.setScene(OSCHelpers::getIntArg(msg[0]), augmentaSceneSize);
	}
	else if (address == "/object/enter" || address == "/object/update")
	{
		processPerson(msg, module);
	}
	else if (address == "/object/leave")
	{
		if (msg.size() < 2) return;
		data.removePerson(this, OSCHelpers::getIntArg(msg[1]));
	}
}

void AugmentaSource::processPerson(const OSCMessage& msg, SpatialModule* module)
{
	if (msg.size() < 15) return;

	Point<float> area = augmentaSceneSize;
	if (area.x <= 0 || area.y <= 0) return; //no /scene yet : Augmenta positions (0-1 in its scene) can't be placed

	TrackedPerson p;
	p.sourceId = OSCHelpers::getIntArg(msg[1]);
	p.sourceName = niceName;
	p.source = this;
	p.age = OSCHelpers::getFloatArg(msg[3]);

	//normalized in the Augmenta scene -> Augmenta meters -> scene meters
	Point<float> meters(OSCHelpers::getFloatArg(msg[4]) * area.x, OSCHelpers::getFloatArg(msg[5]) * area.y);
	p.position = toScene(meters, area);

	Point<float> v(OSCHelpers::getFloatArg(msg[6]) * area.x, OSCHelpers::getFloatArg(msg[7]) * area.y);
	p.velocity = vectorToScene(v);
	p.orientation = angleToScene(OSCHelpers::getFloatArg(msg[8]));
	p.height = OSCHelpers::getFloatArg(msg[14]);
	p.lastUpdateMs = Time::getMillisecondCounter();

	module->trackingData.updatePerson(p);
}

void AugmentaSource::update(SpatialModule* module)
{
	if (!enabled->boolValue()) status->setValue("Disabled");
	else if (!isUsed) status->setValue("Not used : only the first Augmenta source receives the OSC input");
	else
	{
		String s = getReceptionText(module) + ", " + String(module->trackingData.getNumPersons(this)) + " people";
		if (augmentaSceneSize.x > 0) s += ", Augmenta scene " + String(augmentaSceneSize.x, 1) + " x " + String(augmentaSceneSize.y, 1) + " m";
		else s += ", waiting for /scene";
		status->setValue(s);
	}
}

// SpatialSourceManager

SpatialSourceManager::SpatialSourceManager() :
	SpatialSectionManager("Sources", SpatialSection::SOURCES)
{
	itemDataType = "SpatialSource";
	selectItemWhenCreated = false;
	managerFactory = &factory;
	factory.defs.add(Factory<SpatialSource>::Definition::createDef<AugmentaSource>(""));
}

SpatialSourceManager::~SpatialSourceManager()
{
}

AugmentaSource* SpatialSourceManager::getAugmentaSource()
{
	for (auto& s : items)
		if (s->enabled->boolValue())
			if (AugmentaSource* a = dynamic_cast<AugmentaSource*>(s)) return a;
	return nullptr;
}
