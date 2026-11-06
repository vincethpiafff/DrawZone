/*
  ==============================================================================

	LogicGraph.cpp
	Created: 6 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

Colour LogicPin::getColour(Kind k)
{
	switch (k)
	{
	case EXEC: return Colour(0xffe8eaee);
	case NUMBER: return Colour(0xff5ec8d8);
	case BOOL: return Colour(0xfff0a35e);
	case TEXT: return Colour(0xff6fd08c);
	case TIME: return Colour(0xffc792ea);
	case PERCENT: return Colour(0xffe6d96a);
	default: return Colour(0xffa7adb7);
	}
}

bool LogicPin::canConnect(Kind out, Kind in)
{
	if (out == EXEC || in == EXEC) return out == in;
	auto isNumber = [](Kind k) { return k == NUMBER || k == TIME || k == PERCENT; };
	return in == ANY || out == ANY || in == out || (isNumber(in) && isNumber(out));
}

// LogicNode

LogicNode::LogicNode(const String& name, var params) :
	BaseItem(name)
{
	//empty default, then a value : a parameter equal to its default is not saved, and the links need it after a reload
	uid = addStringParameter("Uid", "Identifier of this node for the links", "");
	uid->setValue(Uuid().toString());
	uid->hideInEditor = true;
}

LogicNode::~LogicNode()
{
}

void LogicNode::addPin(const String& name, LogicPin::Kind kind, bool output)
{
	pins.add({ name, kind, output });
}

int LogicNode::getPinRow(const String& name, bool output) const
{
	int row = 0;
	for (auto& p : pins)
	{
		if (p.output != output) continue;
		if (p.name == name) return row;
		row++;
	}
	return -1;
}

int LogicNode::getNumRows() const
{
	int in = 0, out = 0;
	for (auto& p : pins) (p.output ? out : in)++;
	return jmax(in, out, getSummary().isNotEmpty() ? 1 : 0);
}

const LogicPinDef* LogicNode::getPin(const String& name, bool output) const
{
	for (auto& p : pins) if (p.output == output && p.name == name) return &p;
	return nullptr;
}

Colour LogicNode::getCategoryColour(Category c)
{
	switch (c)
	{
	case EVENT: return Colour(0xffff6b7a);
	case READ: return Colour(0xff5ec8d8);
	case ACTION: return Colour(0xff6aa8ff);
	case VARIABLE: return Colour(0xff6fd08c);
	default: return Colour(0xffa7adb7);
	}
}

String LogicNode::getCategoryName(Category c)
{
	switch (c)
	{
	case EVENT: return "Event";
	case READ: return "Read";
	case ACTION: return "Output";
	case VARIABLE: return "Variable";
	default: return "Flow";
	}
}

// LogicTargets

void LogicTargets::setupZone(TargetParameter* t, SpatialModule* m)
{
	if (m == nullptr) return;
	t->targetType = TargetParameter::CONTAINER;
	t->maxDefaultSearchLevel = 0;
	t->defaultContainerTypeCheckFunc = [](ControllableContainer* cc) { return dynamic_cast<SpatialZone*>(cc) != nullptr; };
	t->setRootContainer(&m->zoneManager, false, true);
}

SpatialZone* LogicTargets::getZone(TargetParameter* t)
{
	return dynamic_cast<SpatialZone*>(t->targetContainer.get());
}

String LogicTargets::getZoneName(TargetParameter* t)
{
	ControllableContainer* z = t->targetContainer.get();
	return z != nullptr ? z->niceName : String("(no zone)");
}

SpatialSurface* LogicTargets::getSurface(TargetParameter* t, SpatialModule* m)
{
	return m != nullptr ? m->surfaceManager.getSurface(t) : nullptr;
}

String LogicTargets::getSlotSummary(TargetParameter* surface, IntParameter* slot)
{
	ControllableContainer* s = surface->targetContainer.get();
	String name = s != nullptr ? s->niceName : String("First surface");
	return name + ", " + (slot->intValue() > 0 ? "slot " + String(slot->intValue()) : String("any slot"));
}

//people in "now" and not in "before" (in = true), or the other way : fires outPin for each one, firedId set first
static void fireIdChanges(LogicGraph& g, LogicNode* node, const Array<int>& before, const Array<int>& now, bool in, int& firedId)
{
	const Array<int>& from = in ? now : before;
	const Array<int>& other = in ? before : now;
	for (int id : from) if (!other.contains(id)) { firedId = id; g.trigger(node, "Then"); }
}

// StateNode

StateNode::StateNode(const String& name, var params) :
	LogicNode(name, params)
{
	addPin("On", LogicPin::EXEC, true);
	addPin("Off", LogicPin::EXEC, true);
	addPin("State", LogicPin::BOOL, true);
}

void StateNode::process(LogicGraph& g)
{
	bool valid = enabled->boolValue();
	bool state = valid ? getState(valid) : false;
	if (!valid) { reset(); return; }

	bool changed = primed && state != lastState;
	lastState = state;
	primed = true;
	if (changed) g.trigger(this, state ? "On" : "Off");
}

var StateNode::getOutputValue(LogicGraph&, const String& outPin)
{
	if (outPin != "State") return var();
	bool valid = true;
	return getState(valid) && valid;
}

// CountNode

CountNode::CountNode(const String& name, var params) :
	LogicNode(name, params)
{
	addPin("Changed", LogicPin::EXEC, true);
	addPin("Count", LogicPin::NUMBER, true);
}

void CountNode::process(LogicGraph& g)
{
	int count = 0;
	if (!enabled->boolValue() || !getCount(count)) { reset(); return; }

	bool changed = primed && count != lastCount;
	lastCount = count;
	primed = true;
	if (changed) g.trigger(this, "Changed");
}

var CountNode::getOutputValue(LogicGraph&, const String& outPin)
{
	if (outPin != "Count") return var();
	int count = 0;
	return getCount(count) ? count : 0;
}

// ZoneTriggerNode

ZoneTriggerNode::ZoneTriggerNode(const String& name, var params, bool entered) :
	LogicNode(name, params),
	entered(entered)
{
	zoneTarget = addTargetParameter("Zone", entered ? "Zone watched : fires once for each person who enters it" : "Zone watched : fires once for each person who leaves it or is lost by the tracking in it");
	addPin("Then", LogicPin::EXEC, true);
	addPin("Person Id", LogicPin::NUMBER, true);
}

void ZoneTriggerNode::setup(SpatialModule* m)
{
	LogicNode::setup(m);
	LogicTargets::setupZone(zoneTarget, m);
}

void ZoneTriggerNode::process(LogicGraph& g)
{
	SpatialZone* z = enabled->boolValue() ? LogicTargets::getZone(zoneTarget) : nullptr;
	if (z == nullptr) { reset(); return; }

	Array<int> ids = z->idsInside;
	if (primed) fireIdChanges(g, this, lastIds, ids, entered, firedId);
	lastIds = ids;
	primed = true;
}

var ZoneTriggerNode::getOutputValue(LogicGraph&, const String& outPin)
{
	return outPin == "Person Id" ? var(firedId) : var();
}

// SlotTriggerNode

SlotTriggerNode::SlotTriggerNode(const String& name, var params, bool joined) :
	LogicNode(name, params),
	joined(joined)
{
	surfaceTarget = addTargetParameter("Surface", "Surface of the slots watched. Empty : the first surface");
	slotNumber = addIntParameter("Slot", joined ? "Player slot watched (1, 2...) : fires when a person takes it. 0 = any slot of the surface" : "Player slot watched (1, 2...) : fires when it is released. 0 = any slot of the surface", 0, 0);
	addPin("Then", LogicPin::EXEC, true);
	addPin("Slot", LogicPin::NUMBER, true);
	addPin("Person Id", LogicPin::NUMBER, true);
}

void SlotTriggerNode::setup(SpatialModule* m)
{
	LogicNode::setup(m);
	if (m != nullptr) m->surfaceManager.setupTarget(surfaceTarget);
}

void SlotTriggerNode::process(LogicGraph& g)
{
	SpatialSurface* surface = enabled->boolValue() ? LogicTargets::getSurface(surfaceTarget, module) : nullptr;
	if (surface == nullptr) return;

	int only = slotNumber->intValue();
	Array<PlayerSlots::SlotEvent> events = surface->playerSlots.events;	// a copy : a node may change the slots
	for (auto& e : events)
	{
		if (e.joined != joined || (only > 0 && e.slot != only)) continue;
		firedSlot = e.slot;
		firedId = e.personId;
		g.trigger(this, "Then");
	}
}

var SlotTriggerNode::getOutputValue(LogicGraph&, const String& outPin)
{
	if (outPin == "Slot") return firedSlot;
	if (outPin == "Person Id") return firedId;
	return var();
}

// PersonTriggerNode

PersonTriggerNode::PersonTriggerNode(const String& name, var params, bool appeared) :
	LogicNode(name, params),
	appeared(appeared)
{
	addPin("Then", LogicPin::EXEC, true);
	addPin("Person Id", LogicPin::NUMBER, true);
}

void PersonTriggerNode::process(LogicGraph& g)
{
	if (module == nullptr || !enabled->boolValue()) { reset(); return; }

	Array<int> ids;
	for (auto& p : module->people) ids.add(p.first);
	if (primed) fireIdChanges(g, this, lastIds, ids, appeared, firedId);
	lastIds = ids;
	primed = true;
}

var PersonTriggerNode::getOutputValue(LogicGraph&, const String& outPin)
{
	return outPin == "Person Id" ? var(firedId) : var();
}

// ZonePresenceNode, ZoneFullNode

ZonePresenceNode::ZonePresenceNode(var params) :
	StateNode(getTypeStringStatic(), params)
{
	zoneTarget = addTargetParameter("Zone", "Zone watched : On when someone enters it while it was empty, Off when it is empty again");
}

void ZonePresenceNode::setup(SpatialModule* m)
{
	StateNode::setup(m);
	LogicTargets::setupZone(zoneTarget, m);
}

bool ZonePresenceNode::getState(bool& valid) const
{
	SpatialZone* z = LogicTargets::getZone(zoneTarget);
	valid = z != nullptr;
	return valid && z->idsInside.size() > 0;
}

ZoneFullNode::ZoneFullNode(var params) :
	StateNode(getTypeStringStatic(), params)
{
	zoneTarget = addTargetParameter("Zone", "Zone watched : On when its Person Count reaches its Capacity, Off when it goes under it again");
}

void ZoneFullNode::setup(SpatialModule* m)
{
	StateNode::setup(m);
	LogicTargets::setupZone(zoneTarget, m);
}

bool ZoneFullNode::getState(bool& valid) const
{
	SpatialZone* z = LogicTargets::getZone(zoneTarget);
	valid = z != nullptr;
	return valid && z->idsInside.size() >= z->capacity->intValue();
}

// SlotStateNode

SlotStateNode::SlotStateNode(const String& name, var params, bool held) :
	StateNode(name, params),
	held(held)
{
	surfaceTarget = addTargetParameter("Surface", "Surface of the slot watched. Empty : the first surface");
	slotNumber = addIntParameter("Slot", held ? "Player slot watched (1, 2...) : On when it waits for a lost person (Hold Spot To Reclaim), Off when it stops waiting" : "Player slot watched (1, 2...) : On when a person takes it, Off when it is free again (a held slot stays occupied)", 1, 1);
}

void SlotStateNode::setup(SpatialModule* m)
{
	StateNode::setup(m);
	if (m != nullptr) m->surfaceManager.setupTarget(surfaceTarget);
}

bool SlotStateNode::getState(bool& valid) const
{
	SpatialSurface* surface = LogicTargets::getSurface(surfaceTarget, module);
	int i = slotNumber->intValue() - 1;
	valid = surface != nullptr && i >= 0 && i < surface->playerSlots.slots.size();
	if (!valid) return false;
	PlayerSlot* s = surface->playerSlots.slots[i];
	return held ? s->isHeld() : !s->isFree();
}

// PersonCountNode, ZonePersonCountNode, FreeSlotsNode

bool PersonCountNode::getCount(int& count) const
{
	if (module == nullptr) return false;
	count = (int)module->people.size();
	return true;
}

ZonePersonCountNode::ZonePersonCountNode(var params) :
	CountNode(getTypeStringStatic(), params)
{
	zoneTarget = addTargetParameter("Zone", "Zone watched : Changed fires each time its Person Count changes");
}

void ZonePersonCountNode::setup(SpatialModule* m)
{
	CountNode::setup(m);
	LogicTargets::setupZone(zoneTarget, m);
}

bool ZonePersonCountNode::getCount(int& count) const
{
	SpatialZone* z = LogicTargets::getZone(zoneTarget);
	if (z == nullptr) return false;
	count = z->idsInside.size();
	return true;
}

FreeSlotsNode::FreeSlotsNode(var params) :
	CountNode(getTypeStringStatic(), params)
{
	surfaceTarget = addTargetParameter("Surface", "Surface watched : Changed fires each time its number of free player slots changes. Empty : the first surface");
}

void FreeSlotsNode::setup(SpatialModule* m)
{
	CountNode::setup(m);
	if (m != nullptr) m->surfaceManager.setupTarget(surfaceTarget);
}

bool FreeSlotsNode::getCount(int& count) const
{
	SpatialSurface* surface = LogicTargets::getSurface(surfaceTarget, module);
	if (surface == nullptr) return false;
	count = surface->playerSlots.freeSlots->intValue();
	return true;
}

String FreeSlotsNode::getSummary() const
{
	ControllableContainer* s = surfaceTarget->targetContainer.get();
	return s != nullptr ? s->niceName : String("First surface");
}

// OnStartNode

OnStartNode::OnStartNode(var params) :
	LogicNode(getTypeStringStatic(), params)
{
	addPin("Then", LogicPin::EXEC, true);
}

void OnStartNode::process(LogicGraph& g)
{
	if (!pending || !enabled->boolValue()) return;
	pending = false;
	g.trigger(this, "Then");
}

// TimerNode

TimerNode::TimerNode(var params) :
	LogicNode(getTypeStringStatic(), params)
{
	interval = (FloatParameter*)addParameter(new TimecodeParameter("Interval", "Time between two ticks (HH:MM:SS:FF)", 1, 0.02));
	addPin("Then", LogicPin::EXEC, true);
	addPin("Count", LogicPin::NUMBER, true);
}

void TimerNode::process(LogicGraph& g)
{
	if (!enabled->boolValue()) { reset(); return; }
	double now = Time::getMillisecondCounterHiRes();
	if (lastMs <= 0) { lastMs = now; return; }	// first tick one interval after the start
	double step = interval->floatValue() * 1000.0;
	if (now - lastMs < step) return;
	lastMs = now - jmin(now - lastMs - step, step);	// keeps the rhythm, without a burst after a pause
	count++;
	g.trigger(this, "Then");
}

var TimerNode::getOutputValue(LogicGraph&, const String& outPin)
{
	return outPin == "Count" ? var(count) : var();
}

String TimerNode::getSummary() const
{
	return "every " + TimecodeParameter::toTimecode(interval->floatValue());
}

// LogNode

LogNode::LogNode(var params) :
	LogicNode(getTypeStringStatic(), params)
{
	message = addStringParameter("Message", "Text written in the Logger, followed by the Value input when it is linked", "Hello");
	addPin("In", LogicPin::EXEC, false);
	addPin("Value", LogicPin::ANY, false);
	addPin("Then", LogicPin::EXEC, true);
}

void LogNode::execute(LogicGraph& g, const String&)
{
	if (!enabled->boolValue()) return;
	var v = g.getInput(this, "Value");
	String text = message->stringValue();
	if (!v.isVoid()) text += " " + v.toString();
	NLOG(g.niceName, text);
	g.trigger(this, "Then");
}

// LogicNodeManager

LogicNodeManager::LogicNodeManager() :
	BaseManager("Nodes")
{
	itemDataType = "LogicNode";
	selectItemWhenCreated = false;
	managerFactory = &factory;
	factory.defs.add(Factory<LogicNode>::Definition::createDef<OnStartNode>("Events/Time"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<TimerNode>("Events/Time"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<ZoneEnteredNode>("Events/Zones"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<ZoneExitedNode>("Events/Zones"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<ZonePresenceNode>("Events/Zones"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<ZoneFullNode>("Events/Zones"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<ZonePersonCountNode>("Events/Zones"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<SlotJoinedNode>("Events/Slots"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<SlotLeftNode>("Events/Slots"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<SlotOccupiedNode>("Events/Slots"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<SlotHeldNode>("Events/Slots"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<FreeSlotsNode>("Events/Slots"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<PersonAppearedNode>("Events/People"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<PersonLostNode>("Events/People"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<PersonCountNode>("Events/People"));
	factory.defs.add(Factory<LogicNode>::Definition::createDef<LogNode>("Outputs"));
}

LogicNodeManager::~LogicNodeManager()
{
}

LogicNode* LogicNodeManager::createItemFromData(var data)
{
	LogicNode* n = BaseManager<LogicNode>::createItemFromData(data);
	if (n != nullptr) n->setup(module);
	return n;
}

void LogicNodeManager::addItemInternal(LogicNode* item, var)
{
	item->setup(module);
	//duplicated or pasted : the copy gets its own uid (links stay on the original)
	for (auto& n : items)
		if (n != item && n->uid->stringValue() == item->uid->stringValue()) { item->uid->setValue(Uuid().toString()); break; }
}

// LogicGraph

LogicGraph::LogicGraph(SpatialModule* module) :
	BaseItem("Graph"),
	module(module)
{
	nodes.module = module;
	nodes.addBaseManagerListener(this);
	addChildControllableContainer(&nodes);
	saveAndLoadRecursiveData = true;
	nodes.editorIsCollapsed = true;
}

LogicGraph::~LogicGraph()
{
	nodes.removeBaseManagerListener(this);
}

LogicNode* LogicGraph::getNode(const String& id) const
{
	for (auto& n : nodes.items) if (n->uid->stringValue() == id) return n;
	return nullptr;
}

void LogicGraph::addLinkInternal(const Link& l)
{
	links.addIfNotAlreadyThere(l);
}

void LogicGraph::removeLinkInternal(const Link& l)
{
	links.removeFirstMatchingValue(l);
}

void LogicGraph::connect(const Link& l)
{
	LogicNode* from = getNode(l.fromNode);
	LogicNode* to = getNode(l.toNode);
	if (from == nullptr || to == nullptr || from == to || links.contains(l)) return;
	const LogicPinDef* out = from->getPin(l.fromPin, true);
	const LogicPinDef* in = to->getPin(l.toPin, false);
	if (out == nullptr || in == nullptr || !LogicPin::canConnect(out->kind, in->kind)) return;

	Array<UndoableAction*> actions;
	for (auto& o : links)
	{
		bool sameExecOut = out->kind == LogicPin::EXEC && o.fromNode == l.fromNode && o.fromPin == l.fromPin;
		bool sameDataIn = in->kind != LogicPin::EXEC && o.toNode == l.toNode && o.toPin == l.toPin;
		if (sameExecOut || sameDataIn) actions.add(new LinkAction(this, o, false));
	}
	actions.add(new LinkAction(this, l, true));
	UndoMaster::getInstance()->performActions("Connect", actions);
}

void LogicGraph::disconnect(const Link& l)
{
	if (!links.contains(l)) return;
	UndoMaster::getInstance()->performAction("Disconnect", new LinkAction(this, l, false));
}

Array<UndoableAction*> LogicGraph::getDisconnectNodeActions(LogicNode* n)
{
	Array<UndoableAction*> actions;
	String id = n->uid->stringValue();
	for (auto& l : links) if (l.fromNode == id || l.toNode == id) actions.add(new LinkAction(this, l, false));
	return actions;
}

bool LogicGraph::LinkAction::perform()
{
	LogicGraph* g = dynamic_cast<LogicGraph*>(graphRef.get());
	if (g == nullptr) return false;
	if (add) g->addLinkInternal(link);
	else g->removeLinkInternal(link);
	return true;
}

bool LogicGraph::LinkAction::undo()
{
	LogicGraph* g = dynamic_cast<LogicGraph*>(graphRef.get());
	if (g == nullptr) return false;
	if (add) g->removeLinkInternal(link);
	else g->addLinkInternal(link);
	return true;
}

void LogicGraph::process()
{
	if (!enabled->boolValue()) { running = false; return; }
	if (!running)
	{
		running = true;
		for (auto& n : nodes.items) n->reset();
	}
	for (auto& n : nodes.items)
	{
		depth = 0;
		n->process(*this);
	}
}

void LogicGraph::trigger(LogicNode* from, const String& outPin)
{
	if (depth >= maxDepth)
	{
		NLOGWARNING(niceName, "Stopped a chain longer than " + String(maxDepth) + " nodes (a loop ?) at " + from->niceName);
		return;
	}
	depth++;
	String id = from->uid->stringValue();
	Array<Link> targets;	// a copy : a node may change the links while running
	for (auto& l : links) if (l.fromNode == id && l.fromPin == outPin) targets.add(l);
	for (auto& l : targets)
		if (LogicNode* n = getNode(l.toNode)) n->execute(*this, l.toPin);
	depth--;
}

var LogicGraph::getInput(LogicNode* node, const String& inPin)
{
	String id = node->uid->stringValue();
	for (auto& l : links)
	{
		if (l.toNode != id || l.toPin != inPin) continue;
		if (LogicNode* n = getNode(l.fromNode)) return n->getOutputValue(*this, l.fromPin);
	}
	return var();
}

void LogicGraph::itemRemoved(LogicNode* n)
{
	String id = n->uid->stringValue();
	links.removeIf([id](const Link& l) { return l.fromNode == id || l.toNode == id; });
}

void LogicGraph::itemsRemoved(Array<LogicNode*> ns)
{
	for (auto& n : ns) itemRemoved(n);
}

var LogicGraph::getJSONData(bool includeNonOverriden)
{
	var data = BaseItem::getJSONData(includeNonOverriden);
	var linksData;
	for (auto& l : links)
	{
		var lData;
		lData.append(l.fromNode);
		lData.append(l.fromPin);
		lData.append(l.toNode);
		lData.append(l.toPin);
		linksData.append(lData);
	}
	data.getDynamicObject()->setProperty("links", linksData);
	return data;
}

void LogicGraph::loadJSONDataInternal(var data)
{
	BaseItem::loadJSONDataInternal(data);
	links.clear();
	var linksData = data.getProperty("links", var());
	for (int i = 0; i < linksData.size(); i++)
	{
		var lData = linksData[i];
		if (lData.size() < 4) continue;
		links.add({ lData[0].toString(), lData[1].toString(), lData[2].toString(), lData[3].toString() });
	}
}

// LogicGraphManager

LogicGraphManager::LogicGraphManager() :
	SpatialSectionManager("Logic", SpatialSection::LOGIC)
{
	itemDataType = "LogicGraph";
	selectItemWhenCreated = false;
}

LogicGraphManager::~LogicGraphManager()
{
}

void LogicGraphManager::process()
{
	for (auto& g : items) g->process();
}
