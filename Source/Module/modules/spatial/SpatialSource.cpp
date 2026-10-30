/*
  ==============================================================================

	SpatialSource.cpp
	Created: 28 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

#if JUCE_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

SpatialSource::SpatialSource(const String& name, var params) :
	BaseItem(name)
{
	//color of its people in the Tracking Monitor, the default comes from the manager
	setHasCustomColor(true);

	useAsScene = addBoolParameter("Use As Scene", "The scene size comes from this source (Scene > Size Mode : Auto). Needed when several sources know their scene. Only one source at a time", false);
	offset = addPoint2DParameter("Offset", "Where the origin of this source lands in the scene, in meters from the top left corner of the scene (applied last)");
	rotation = addFloatParameter("Rotation", "Rotation of this source around its origin, in degrees, clockwise", 0, -360, 360);
	scale = addFloatParameter("Scale", "Scale of this source (1 = its meters are scene meters)", 1, .01f, 100);
	flipX = addBoolParameter("Flip X", "Mirror this source left / right (within its area when it has one, around its origin otherwise)", false);
	flipY = addBoolParameter("Flip Y", "Mirror this source top / bottom (within its area when it has one, around its origin otherwise)", false);
	resetParameters = addTrigger("Reset All Parameters", "Put Offset, Rotation, Scale and Flip back to their default values (the source as it is sent)");
	status = addStringParameter("Status", "What this source receives", "");
	status->setControllableFeedbackOnly(true);
	status->isSavable = false;
}

SpatialSource::~SpatialSource()
{
}

void SpatialSource::moveParametersToEnd(const Array<Controllable*>& parameters)
{
	for (auto& p : parameters) controllables.move(controllables.indexOf(p), -1);
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

void SpatialSource::onContainerParameterChangedInternal(Parameter* p)
{
	BaseItem::onContainerParameterChangedInternal(p);
	if (p != useAsScene || !useAsScene->boolValue()) return;
	if (SpatialSourceManager* m = dynamic_cast<SpatialSourceManager*>(parentContainer.get()))
		for (auto& s : m->items) if (s != this) s->useAsScene->setValue(false);
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

// PSNSource

PSNSource::PSNSource(var params) :
	SpatialSource(getTypeStringStatic(), params),
	Thread("Cockpit PSN")
{
	multicastAddress = addStringParameter("Multicast Address", "Multicast group to join. PosiStageNet default is 236.10.10.10", psn::DEFAULT_UDP_MULTICAST_ADDR);
	multicastPort = addIntParameter("Multicast Port", "Port to listen to. PosiStageNet default is 56565", psn::DEFAULT_UDP_PORT, 1, 65535);
	upAxis = addEnumParameter("Up Axis", "Vertical axis of the positions received. Y : PosiStageNet standard. Z : systems with a vertical Z axis, like Kratos without remap");
	upAxis->addOption("Y", UP_Y)->addOption("Z", UP_Z);
	ignoreOrigin = addBoolParameter("Ignore Trackers At Origin", "Don't create a person for a tracker at 0, 0, 0 (within 1 mm) : many PSN systems send their unused trackers there", true);
	areaPosition = addPoint2DParameter("Area Position", "PSN sends no scene size. Corner of the area to use as the scene, in the coordinates of the tracking system. Kratos : Area > Position X, Y");
	areaSize = addPoint2DParameter("Area Size", "PSN sends no scene size. Size of the area to use as the scene, in meters (0 = no area). Kratos : Area > Scale X, Y");
	areaSize->setBounds(0, 0, 1000, 1000);

	moveParametersToEnd({ areaPosition, areaSize, upAxis, offset, rotation, scale, flipX, flipY, resetParameters, multicastAddress, multicastPort, ignoreOrigin, status });
}

PSNSource::~PSNSource()
{
	stopThread(1000);
}

void PSNSource::clearItem()
{
	//before the manager forgets this source : the thread could add a person back
	stopThread(1000);
	SpatialSource::clearItem();
}

void PSNSource::onContainerParameterChangedInternal(Parameter* p)
{
	SpatialSource::onContainerParameterChangedInternal(p);
	if (p == multicastAddress || p == multicastPort) stopThread(1000); //update() starts it again
}

void PSNSource::update(SpatialModule* module)
{
	if (!enabled->boolValue() || !module->enabled->boolValue())
	{
		stopThread(1000);
		status->setValue("Disabled");
		return;
	}

	if (!isThreadRunning())
	{
		connection = CONNECTING;
		threadModule = module;
		startThread();
	}

	String group = multicastAddress->stringValue() + ":" + multicastPort->stringValue();
	switch (connection.load())
	{
	case CONNECTING: status->setValue("Connecting to " + group); break;
	case BIND_FAILED: status->setValue("Could not bind port " + multicastPort->stringValue()); break;
	case JOIN_FAILED: status->setValue("Could not join multicast " + multicastAddress->stringValue()); break;
	case CONNECTED:
	{
		String s = getReceptionText(module) + ", " + String(module->trackingData.getNumPersons(this)) + " people, " + group;
		const ScopedLock sl(systemNameLock);
		if (systemName.isNotEmpty()) s += ", from " + systemName;
		if (systemName.containsIgnoreCase("kratos") && (upAxis->getValueDataAsEnum<UpAxis>() != UP_Z || getSceneArea().x <= 0))
			s += ". Kratos : set Up Axis to Z, and Area Position / Size from Kratos > Area";
		status->setValue(s);
		break;
	}
	}
}

Point<float> PSNSource::getSceneArea()
{
	Point<float> a = areaSize->getPoint();
	return a.x > 0 && a.y > 0 ? a : Point<float>();
}

Point<float> PSNSource::getAreaOrigin(bool upY)
{
	Point<float> area = getSceneArea();
	if (area.x <= 0) return Point<float>();
	Point<float> pos = areaPosition->getPoint();
	return upY ? pos : Point<float>(pos.x, -(pos.y + area.y)); //Z up : y is turned over, so its far end is the top
}

bool PSNSource::joinOnAllInterfaces(DatagramSocket& socket, const String& address)
{
	//DatagramSocket::joinMulticast only joins on the default network card : a sender on another one (Kratos on Loopback) is not heard
	IPAddress group(address);
	if (group.isNull() || group.isIPv6) return false;

	Array<IPAddress> interfaces = IPAddress::getAllAddresses();
	interfaces.addIfNotAlreadyThere(IPAddress::local());

	int joined = 0;
	for (auto& ip : interfaces)
	{
		ip_mreq request;
		memcpy(&request.imr_multiaddr, group.address, 4);
		memcpy(&request.imr_interface, ip.address, 4);
#if JUCE_WINDOWS
		if (setsockopt((SOCKET)socket.getRawSocketHandle(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&request, sizeof(request)) == 0) joined++;
#else
		if (setsockopt(socket.getRawSocketHandle(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &request, sizeof(request)) == 0) joined++;
#endif
	}
	return joined > 0;
}

void PSNSource::run()
{
	String address = multicastAddress->stringValue();
	int port = multicastPort->intValue();

	//retried every second : the network may come up after the session
	while (!threadShouldExit())
	{
		DatagramSocket socket(true);
		socket.setEnablePortReuse(true);
		if (!socket.bindToPort(port)) connection = BIND_FAILED;
		else if (!joinOnAllInterfaces(socket, address)) connection = JOIN_FAILED;
		else
		{
			connection = CONNECTED;
			psn::psn_decoder decoder;
			char buffer[psn::MAX_UDP_PACKET_SIZE];
			int lastFrameId = -1;

			while (!threadShouldExit())
			{
				if (socket.waitUntilReady(true, 100) <= 0) continue;
				int numRead = socket.read(buffer, psn::MAX_UDP_PACKET_SIZE, false);
				if (numRead <= 0) continue;
				decoder.decode(buffer, numRead);

				String name(decoder.get_info().system_name);
				{
					const ScopedLock sl(systemNameLock);
					systemName = name;
				}

				//a frame can take several packets : the decoder gives it once complete
				const psn::psn_decoder::data_t& frame = decoder.get_data();
				if (frame.header.frame_packet_count == 0 || frame.header.frame_id == lastFrameId) continue;
				lastFrameId = frame.header.frame_id;
				threadModule->trackingData.countMessage(this);
				processFrame(frame.trackers);
			}
			return;
		}
		wait(1000);
	}
}

Point<float> PSNSource::toGround(const psn::float3& v, bool upY) const
{
	//Y up : x right, z toward the audience. Z up : x right, y away from the audience, so turned over for a view from above
	return upY ? Point<float>(v.x, v.z) : Point<float>(v.x, -v.y);
}

void PSNSource::processFrame(const psn::tracker_map& trackers)
{
	const float originTolerance = .001f; // meters
	bool upY = upAxis->getValueDataAsEnum<UpAxis>() == UP_Y;
	Point<float> area = getSceneArea();
	Point<float> areaOrigin = getAreaOrigin(upY);
	uint32 now = Time::getMillisecondCounter();
	std::map<int, uint32> seen;

	for (auto& it : trackers)
	{
		const psn::tracker& t = it.second;
		if (!t.is_pos_set()) continue;

		psn::float3 pos = t.get_pos();
		//not exactly 0 : Kratos sends values like 4e-18
		if (ignoreOrigin->boolValue() && std::abs(pos.x) < originTolerance && std::abs(pos.y) < originTolerance && std::abs(pos.z) < originTolerance) continue;

		TrackedPerson p;
		p.sourceId = t.get_id();
		p.sourceName = niceName;
		p.source = this;

		auto first = firstSeenMs.find(p.sourceId);
		uint32 firstMs = first != firstSeenMs.end() ? first->second : now;
		seen[p.sourceId] = firstMs;
		p.age = (now - firstMs) / 1000.0f;

		//without area, PSN has no scene and its meters are absolute
		p.position = toScene(toGround(pos, upY) - areaOrigin, area);
		p.height = upY ? pos.y : pos.z;
		p.trackerName = t.get_name();
		if (t.is_speed_set()) p.velocity = vectorToScene(toGround(t.get_speed(), upY));
		if (t.is_ori_set())
		{
			//radians around the up axis, counterclockwise seen from above
			psn::float3 ori = t.get_ori();
			p.orientation = angleToScene(-radiansToDegrees(upY ? ori.y : ori.z));
		}
		p.lastUpdateMs = now;

		threadModule->trackingData.updatePerson(p);
	}

	firstSeenMs = std::move(seen);
}

// SpatialSourceManager

SpatialSourceManager::SpatialSourceManager() :
	SpatialSectionManager("Sources", SpatialSection::SOURCES)
{
	itemDataType = "SpatialSource";
	selectItemWhenCreated = false;
	managerFactory = &factory;
	factory.defs.add(Factory<SpatialSource>::Definition::createDef<AugmentaSource>(""));
	factory.defs.add(Factory<SpatialSource>::Definition::createDef<PSNSource>(""));
}

SpatialSourceManager::~SpatialSourceManager()
{
}

void SpatialSourceManager::addItemInternal(SpatialSource* item, var)
{
	//distinct hues, spread by the golden ratio after the first 4 ; a color chosen by the user is kept
	const float hues[] = { .08f, .55f, .33f, .8f };
	int index = items.indexOf(item);
	float hue = index < 4 ? hues[index] : std::fmod(index * .618034f, 1.0f);
	if (!item->itemColor->isOverriden) item->itemColor->setDefaultValue(Colour::fromHSV(hue, .7f, 1, 1));
}

AugmentaSource* SpatialSourceManager::getAugmentaSource()
{
	for (auto& s : items)
		if (s->enabled->boolValue())
			if (AugmentaSource* a = dynamic_cast<AugmentaSource*>(s)) return a;
	return nullptr;
}

Point<float> SpatialSourceManager::getSceneArea()
{
	Point<float> known;
	int numKnown = 0;
	for (auto& s : items)
	{
		if (!s->enabled->boolValue()) continue;
		Point<float> a = s->getSceneArea();
		if (s->useAsScene->boolValue()) return a;
		if (a.x > 0 && a.y > 0)
		{
			known = a;
			numKnown++;
		}
	}
	return numKnown == 1 ? known : Point<float>();
}
