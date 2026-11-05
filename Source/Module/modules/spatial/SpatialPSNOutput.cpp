/*
  ==============================================================================

	SpatialPSNOutput.cpp
	Created: 30 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

SpatialPSNOutput::SpatialPSNOutput(SpatialModule* module) :
	EnablingControllableContainer("PSN Output"),
	module(module)
{
	enabled->setDefaultValue(false);

	sendMode = addEnumParameter("Send", "Who this output sends, always in surface meters, whatever the source.\nPlayer Slots : the occupied slots of each surface, 1st surface : trackers 60001, 60002..., 2nd surface : 60101... A surface whose Player Slots are off : its people, with the id given by the tracking\nSource IDs : everyone, with the id of its source.\nNames : \"JOUEUR 1 (SOL)\", the slot or tracker then its surface");
	sendMode->addOption("Player Slots", PLAYER_SLOTS)->addOption("Source IDs", SOURCE_IDS);

	networkInterface = new NetworkInterfaceParameter();
	addParameter(networkInterface);

	//not the PSN default : a PSN source on its default group would hear this output
	multicastAddress = addStringParameter("Multicast Address", "Multicast group to send to. PosiStageNet default is 236.10.10.10", "236.10.10.11");
	multicastPort = addIntParameter("Multicast Port", "Port to send to. PosiStageNet default is 56565", psn::DEFAULT_UDP_PORT, 1, 65535);
	status = addStringParameter("Status", "What this output sends", "");
	status->setControllableFeedbackOnly(true);
	status->isSavable = false;
}

SpatialPSNOutput::~SpatialPSNOutput()
{
}

void SpatialPSNOutput::onContainerParameterChanged(Parameter* p)
{
	EnablingControllableContainer::onContainerParameterChanged(p);
	if (p == networkInterface) socket.reset(); //update() opens it again
}

void SpatialPSNOutput::openSocket()
{
	String ip = networkInterface->getIP();
	std::unique_ptr<DatagramSocket> s(new DatagramSocket(true));
	if (!s->bindToPort(0, ip)) return;

	if (ip.isNotEmpty())
	{
		//multicast leaves by the default card unless told otherwise
		IPAddress local(ip);
		in_addr address;
		memcpy(&address, local.address, 4);
#if JUCE_WINDOWS
		if (setsockopt((SOCKET)s->getRawSocketHandle(), IPPROTO_IP, IP_MULTICAST_IF, (const char*)&address, sizeof(address)) != 0) return;
#else
		if (setsockopt(s->getRawSocketHandle(), IPPROTO_IP, IP_MULTICAST_IF, &address, sizeof(address)) != 0) return;
#endif
	}
	socket = std::move(s);
}

void SpatialPSNOutput::send(const std::list<std::string>& packets)
{
	String address = multicastAddress->stringValue();
	int port = multicastPort->intValue();
	for (auto& p : packets) socket->write(address, port, p.data(), (int)p.size());
}

void SpatialPSNOutput::fillFromSlots(psn::tracker_map& trackers, const Array<TrackedPerson>& persons)
{
	//surface n (from 0) : its slots are trackers 60000 + 100 x n + 1, + 2... (64 slots at most)
	auto& surfaces = module->surfaceManager.items;
	for (int si = 0; si < surfaces.size(); si++)
	{
		SpatialSurface* surface = surfaces[si];
		if (!surface->playerSlots.enabled->boolValue()) continue; //its people are sent below, with their own ids
		auto& slots = surface->playerSlots.slots;
		for (int i = 0; i < slots.size(); i++)
		{
			PlayerSlot* s = slots[i];
			if (s->isFree()) continue;

			int id = getSlotTrackerId(si, i);
			psn::tracker t((uint16_t)id, getTrackerName(s->niceName, surface->niceName).toStdString());
			Point<float> speed; //none while held
			if (!s->isHeld())
			{
				for (auto& p : persons)
				{
					if (p.id != s->trackedId) continue;
					lastHeights[id] = p.height;
					speed = p.velocity;
					break;
				}
			}

			Point<float> pos = s->position->getPoint();
			t.set_pos(psn::float3(pos.x, lastHeights[id], pos.y));
			t.set_speed(psn::float3(speed.x, 0, speed.y));
			trackers[t.get_id()] = t;
		}
	}

	//surfaces without slots : their people, with the id given by the tracking (after the slots, which keep their ids)
	for (auto& surface : surfaces)
	{
		if (surface->playerSlots.enabled->boolValue()) continue;
		for (auto& p : persons) if (p.surface == surface) addPerson(trackers, p);
	}
}

void SpatialPSNOutput::addPerson(psn::tracker_map& trackers, const TrackedPerson& p)
{
	if (p.id < 0 || p.id > 65535) return; //PSN ids are on 16 bits
	if (trackers.count((uint16_t)p.id) > 0) { numIdConflicts++; return; } //a slot tracker already has this id

	String name = p.trackerName.isNotEmpty() ? p.trackerName : p.sourceName + " " + String(p.sourceId);
	String surfaceName;
	for (auto& s : module->surfaceManager.items) if (s == p.surface) surfaceName = s->niceName;
	psn::tracker t((uint16_t)p.id, getTrackerName(name, surfaceName).toStdString());
	t.set_pos(psn::float3(p.position.x, p.height, p.position.y));
	t.set_speed(psn::float3(p.velocity.x, 0, p.velocity.y));
	trackers[t.get_id()] = t;
}

void SpatialPSNOutput::fillFromPeople(psn::tracker_map& trackers, const Array<TrackedPerson>& persons)
{
	for (auto& p : persons) addPerson(trackers, p);
}

void SpatialPSNOutput::update(const Array<TrackedPerson>& persons)
{
	numTrackersSent = 0;

	if (!enabled->boolValue() || !module->enabled->boolValue())
	{
		socket.reset();
		return;
	}

	if (socket == nullptr) openSocket();
	if (socket == nullptr) return;

	numIdConflicts = 0;
	psn::tracker_map trackers;
	SendMode mode = sendMode->getValueDataAsEnum<SendMode>();
	if (mode == PLAYER_SLOTS) fillFromSlots(trackers, persons);
	else fillFromPeople(trackers, persons);

	numTrackersSent = (int)trackers.size();

	uint64_t usec = (uint64_t)(Time::getMillisecondCounterHiRes() * 1000);
	send(encoder.encode_data(trackers, usec));

	uint32 now = Time::getMillisecondCounter();
	if (now - lastInfoMs >= 1000)
	{
		lastInfoMs = now;
		send(encoder.encode_info(trackers, usec));
	}
}

void SpatialPSNOutput::updateStatus()
{
	if (!enabled->boolValue() || !module->enabled->boolValue())
	{
		status->setValue("Disabled");
		return;
	}
	if (socket == nullptr)
	{
		status->setValue("Could not open the network interface");
		return;
	}

	String group = multicastAddress->stringValue() + ":" + multicastPort->stringValue();
	String s = "Sending " + String(numTrackersSent) + (numTrackersSent == 1 ? " tracker (" : " trackers (") + sendMode->getValueKey() + ") to " + group;
	if (numIdConflicts > 0) s += ". " + String(numIdConflicts) + (numIdConflicts == 1 ? " person not sent" : " people not sent") + " : a player slot of another surface uses the same id (change the Id Offset of its source)";

	//would come back as new people
	const ScopedLock sl(module->sourceManager.items.getLock());
	for (auto& source : module->sourceManager.items)
	{
		PSNSource* psnSource = dynamic_cast<PSNSource*>(source);
		if (psnSource == nullptr || !psnSource->enabled->boolValue()) continue;
		if (psnSource->multicastAddress->stringValue() == multicastAddress->stringValue() && psnSource->multicastPort->intValue() == multicastPort->intValue())
			s += ". Warning : source " + psnSource->niceName + " listens to this group and will hear this output";
	}
	status->setValue(s);
}

String SpatialPSNOutput::getTrackerName(const String& name, const String& surface)
{
	return surface.isEmpty() ? name : name + " (" + surface + ")";
}
