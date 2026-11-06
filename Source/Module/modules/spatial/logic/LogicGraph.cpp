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
	case METERS: return Colour(0xff7f9cff);
	default: return Colour(0xffa7adb7);
	}
}

bool LogicPin::canConnect(Kind out, Kind in)
{
	if (out == EXEC || in == EXEC) return out == in;
	auto isNumber = [](Kind k) { return k == NUMBER || k == TIME || k == PERCENT || k == METERS; };
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
	case COMPARE: return Colour(0xfff0a35e);
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
	case COMPARE: return "Compare";
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

String LogicTargets::getSlotSummary(TargetParameter* surface, const SlotSetting& slot)
{
	ControllableContainer* s = surface->targetContainer.get();
	String name = s != nullptr ? s->niceName : String("First surface");
	return name + ", " + slot.getName();
}

// SlotSetting

void LogicTargets::SlotSetting::create(ControllableContainer* node, bool any, const String& help)
{
	withAny = any;
	choice = node->addEnumParameter("Slot", help + ". The names are the slots of the surface, as named in the Tracking Monitor");
	choice->setCustomShortName("slotName");	// /slot is the number
	choice->isSavable = false;	// the number is saved : a renamed slot stays selected
	number = node->addIntParameter("Slot Number", "Slot of the list above, from 1 (0 = any slot)", any ? 0 : 1, any ? 0 : 1);
	number->setCustomShortName("slot");	// address /slot : same as the number setting of the first versions
	number->hideInEditor = true;
	refresh(nullptr);
}

String LogicTargets::SlotSetting::getName() const
{
	int n = getNumber();
	if (n <= 0) return "any slot";
	String key = choice->getValueKey();
	return key.isNotEmpty() ? key : "slot " + String(n);
}

void LogicTargets::SlotSetting::refresh(SpatialSurface* surface)
{
	//keys : the slot names, made unique ; a number missing from the surface keeps its own entry so it stays selected
	Array<EnumParameter::EnumValue> options;
	StringArray keys;
	auto add = [&](String key, int n)
	{
		String k = key;
		for (int i = 2; keys.contains(k); i++) k = key + " (" + String(i) + ")";
		keys.add(k);
		options.add(EnumParameter::EnumValue(k, n));
	};
	if (withAny) add("Any slot", 0);
	int count = surface != nullptr ? surface->playerSlots.slots.size() : 0;
	for (int i = 0; i < count; i++) add(surface->playerSlots.slots[i]->niceName, i + 1);
	int n = getNumber();
	if (n > count) add("Slot " + String(n) + (surface != nullptr ? " (not on this surface)" : ""), n);

	if (keys == lastKeys && choice->getValueData() == var(n)) return;
	lastKeys = keys;

	refreshing = true;
	choice->setOptions(options);
	choice->setValueWithData(n);	// parameterChanged ignores it : the number does not change
	refreshing = false;
}

bool LogicTargets::SlotSetting::parameterChanged(Parameter* p)
{
	if (p != choice) return false;
	if (!refreshing && !choice->getValueData().isVoid()) number->setValue((int)choice->getValueData());
	return true;
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
	slot.create(this, true, joined ? "Player slot watched : fires when a person takes it. Any slot : every slot of the surface" : "Player slot watched : fires when it is released. Any slot : every slot of the surface");
	addPin("Then", LogicPin::EXEC, true);
	addPin("Slot", LogicPin::NUMBER, true);
	addPin("Person Id", LogicPin::NUMBER, true);
}

void SlotTriggerNode::setup(SpatialModule* m)
{
	LogicNode::setup(m);
	if (m != nullptr) m->surfaceManager.setupTarget(surfaceTarget);
	slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
}

void SlotTriggerNode::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == surfaceTarget) slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
	else slot.parameterChanged(p);
}

void SlotTriggerNode::process(LogicGraph& g)
{
	SpatialSurface* surface = LogicTargets::getSurface(surfaceTarget, module);
	slot.refresh(surface);
	if (!enabled->boolValue() || surface == nullptr) return;

	int only = slot.getNumber();
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
	slot.create(this, false, held ? "Player slot watched : On when it waits for a lost person (Hold Spot To Reclaim), Off when it stops waiting" : "Player slot watched : On when a person takes it, Off when it is free again (a held slot stays occupied)");
}

void SlotStateNode::setup(SpatialModule* m)
{
	StateNode::setup(m);
	if (m != nullptr) m->surfaceManager.setupTarget(surfaceTarget);
	slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
}

void SlotStateNode::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == surfaceTarget) slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
	else slot.parameterChanged(p);
}

void SlotStateNode::process(LogicGraph& g)
{
	slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
	StateNode::process(g);
}

bool SlotStateNode::getState(bool& valid) const
{
	SpatialSurface* surface = LogicTargets::getSurface(surfaceTarget, module);
	int i = slot.getNumber() - 1;
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

// ValueNode

ValueNode::ValueNode(const String& name, var params) :
	LogicNode(name, params)
{
	addPin("Changed", LogicPin::EXEC, true);
}

void ValueNode::addValuePin(const String& name, LogicPin::Kind kind)
{
	addPin(name, kind, true);
	valueNames.add(name);
}

bool ValueNode::readSafe(LogicGraph& g, Array<var>& values)
{
	if (reading) return false;
	reading = true;
	bool ok = read(g, values);
	reading = false;
	return ok && values.size() == valueNames.size();
}

void ValueNode::process(LogicGraph& g)
{
	if (!enabled->boolValue()) { reset(); return; }

	//nothing to read (person not tracked, slot free...) : no value ; a value that appears later fires Changed
	Array<var> values;
	bool has = readSafe(g, values);
	bool changed = primed && has && (!hadValues || values != lastValues);
	lastValues = values;
	hadValues = has;
	primed = true;
	if (changed) g.trigger(this, "Changed");
}

var ValueNode::getOutputValue(LogicGraph& g, const String& outPin)
{
	int i = valueNames.indexOf(outPin);
	Array<var> values;
	if (i < 0 || !readSafe(g, values)) return var();
	return values[i];
}

ZoneValueNode::ZoneValueNode(const String& name, var params, const String& help) :
	ValueNode(name, params)
{
	zoneTarget = addTargetParameter("Zone", help);
}

void ZoneValueNode::setup(SpatialModule* m)
{
	ValueNode::setup(m);
	LogicTargets::setupZone(zoneTarget, m);
}

SlotValueNode::SlotValueNode(const String& name, var params) :
	ValueNode(name, params)
{
	surfaceTarget = addTargetParameter("Surface", "Surface of the slot watched. Empty : the first surface");
	slot.create(this, false, "Player slot watched : Changed fires each time its value changes");
}

void SlotValueNode::setup(SpatialModule* m)
{
	ValueNode::setup(m);
	if (m != nullptr) m->surfaceManager.setupTarget(surfaceTarget);
	slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
}

void SlotValueNode::onContainerParameterChangedInternal(Parameter* p)
{
	if (p == surfaceTarget) slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
	else slot.parameterChanged(p);
}

void SlotValueNode::process(LogicGraph& g)
{
	slot.refresh(LogicTargets::getSurface(surfaceTarget, module));
	ValueNode::process(g);
}

PlayerSlot* SlotValueNode::getSlot() const
{
	SpatialSurface* surface = LogicTargets::getSurface(surfaceTarget, module);
	int i = slot.getNumber() - 1;
	if (surface == nullptr || i < 0 || i >= surface->playerSlots.slots.size()) return nullptr;
	return surface->playerSlots.slots[i];
}

PersonValueNode::PersonValueNode(const String& name, var params) :
	ValueNode(name, params)
{
	personId = addIntParameter("Person Id", "Person watched, while tracked. The Person Id input replaces it when it is linked (Zone Entered, Slot Joined...)", 1, 0);
	addPin("Person Id", LogicPin::NUMBER, false);
}

SpatialPerson* PersonValueNode::getPerson(LogicGraph& g) const
{
	if (module == nullptr) return nullptr;
	var in = g.getInput(const_cast<PersonValueNode*>(this), "Person Id");
	int id = in.isVoid() ? personId->intValue() : (int)in;
	auto it = module->people.find(id);
	return it != module->people.end() ? it->second : nullptr;
}

ZonePresenceTimeNode::ZonePresenceTimeNode(var params) :
	ZoneValueNode(getTypeStringStatic(), params, "Zone watched : time since it became occupied without interruption (0 when empty)")
{
	addValuePin("Time", LogicPin::TIME);
}

bool ZonePresenceTimeNode::read(LogicGraph&, Array<var>& values)
{
	SpatialZone* z = LogicTargets::getZone(zoneTarget);
	if (z == nullptr) return false;
	values.add(z->presenceTime->floatValue());
	return true;
}

ZoneOccupancyNode::ZoneOccupancyNode(var params) :
	ZoneValueNode(getTypeStringStatic(), params, "Zone watched : Person Count / Capacity, 100 % = full")
{
	addValuePin("Percentage", LogicPin::PERCENT);
}

bool ZoneOccupancyNode::read(LogicGraph&, Array<var>& values)
{
	SpatialZone* z = LogicTargets::getZone(zoneTarget);
	if (z == nullptr) return false;
	values.add(z->occupancy->floatValue());
	return true;
}

ZoneDistanceNode::ZoneDistanceNode(var params) :
	ZoneValueNode(getTypeStringStatic(), params, "Zone watched : distance between the barycenter of the people in it and its center, in meters")
{
	addValuePin("Distance", LogicPin::METERS);
}

bool ZoneDistanceNode::read(LogicGraph&, Array<var>& values)
{
	SpatialZone* z = LogicTargets::getZone(zoneTarget);
	if (z == nullptr || z->averageDistanceToCenter == nullptr) return false;
	values.add(z->averageDistanceToCenter->floatValue());
	return true;
}

SlotHoldRemainingNode::SlotHoldRemainingNode(var params) :
	SlotValueNode(getTypeStringStatic(), params)
{
	addValuePin("Time", LogicPin::TIME);
}

bool SlotHoldRemainingNode::read(LogicGraph&, Array<var>& values)
{
	PlayerSlot* s = getSlot();
	if (s == nullptr) return false;
	values.add(s->holdRemaining->floatValue());
	return true;
}

SlotPositionNode::SlotPositionNode(var params) :
	SlotValueNode(getTypeStringStatic(), params)
{
	addValuePin("X", LogicPin::METERS);
	addValuePin("Y", LogicPin::METERS);
}

bool SlotPositionNode::read(LogicGraph&, Array<var>& values)
{
	PlayerSlot* s = getSlot();
	if (s == nullptr) return false;
	values.add(s->position->x);
	values.add(s->position->y);
	return true;
}

PersonAgeNode::PersonAgeNode(var params) :
	PersonValueNode(getTypeStringStatic(), params)
{
	addValuePin("Time", LogicPin::TIME);
}

bool PersonAgeNode::read(LogicGraph& g, Array<var>& values)
{
	SpatialPerson* p = getPerson(g);
	if (p == nullptr) return false;
	values.add(p->age->floatValue());
	return true;
}

PersonPositionNode::PersonPositionNode(var params) :
	PersonValueNode(getTypeStringStatic(), params)
{
	addValuePin("X", LogicPin::METERS);
	addValuePin("Y", LogicPin::METERS);
}

bool PersonPositionNode::read(LogicGraph& g, Array<var>& values)
{
	SpatialPerson* p = getPerson(g);
	if (p == nullptr) return false;
	values.add(p->position->x);
	values.add(p->position->y);
	return true;
}

PersonHeightNode::PersonHeightNode(var params) :
	PersonValueNode(getTypeStringStatic(), params)
{
	addValuePin("Height", LogicPin::METERS);
}

bool PersonHeightNode::read(LogicGraph& g, Array<var>& values)
{
	SpatialPerson* p = getPerson(g);
	if (p == nullptr) return false;
	values.add(p->height->floatValue());
	return true;
}

// relative positions

static void addRelativePins(std::function<void(const String&, LogicPin::Kind)> add)
{
	add("Zone X", LogicPin::PERCENT);
	add("Zone Y", LogicPin::PERCENT);
	add("Surface X", LogicPin::PERCENT);
	add("Surface Y", LogicPin::PERCENT);
}

//position in meters on surface -> 0-1 of the surface (-1 when its size is unknown)
static void addSurfaceRelative(Array<var>& values, Point<float> meters, SpatialSurface* s)
{
	bool known = s != nullptr && s->size.x > 0 && s->size.y > 0;
	values.add(known ? meters.x / s->size.x : -1.0f);
	values.add(known ? meters.y / s->size.y : -1.0f);
}

static SpatialSurface* getSurfaceNamed(SpatialModule* m, const String& name)
{
	if (m == nullptr) return nullptr;
	for (auto& s : m->surfaceManager.items) if (s->niceName == name) return s;
	return nullptr;
}

PersonRelativePositionNode::PersonRelativePositionNode(var params) :
	PersonValueNode(getTypeStringStatic(), params)
{
	addRelativePins([this](const String& name, LogicPin::Kind k) { addValuePin(name, k); });
}

bool PersonRelativePositionNode::read(LogicGraph& g, Array<var>& values)
{
	SpatialPerson* p = getPerson(g);
	if (p == nullptr) return false;
	values.add(p->relativePosition->x);
	values.add(p->relativePosition->y);
	addSurfaceRelative(values, p->position->getPoint(), getSurfaceNamed(module, p->surfaceName->stringValue()));
	return true;
}

SlotRelativePositionNode::SlotRelativePositionNode(var params) :
	SlotValueNode(getTypeStringStatic(), params)
{
	addRelativePins([this](const String& name, LogicPin::Kind k) { addValuePin(name, k); });
}

bool SlotRelativePositionNode::read(LogicGraph&, Array<var>& values)
{
	PlayerSlot* s = getSlot();
	if (s == nullptr || s->isFree()) return false;
	//in the zone : the person of the slot (-1 while held, the person is lost) ; on the surface : the slot position (frozen while held)
	SpatialPerson* p = nullptr;
	if (module != nullptr && !s->isHeld())
	{
		auto it = module->people.find(s->trackedId);
		if (it != module->people.end()) p = it->second;
	}
	values.add(p != nullptr ? p->relativePosition->x : -1.0f);
	values.add(p != nullptr ? p->relativePosition->y : -1.0f);
	addSurfaceRelative(values, s->position->getPoint(), LogicTargets::getSurface(surfaceTarget, module));
	return true;
}

SlotPersonNode::SlotPersonNode(var params) :
	SlotValueNode(getTypeStringStatic(), params)
{
	addValuePin("Person Id", LogicPin::NUMBER);
}

bool SlotPersonNode::read(LogicGraph&, Array<var>& values)
{
	PlayerSlot* s = getSlot();
	if (s == nullptr) return false;
	values.add(s->isFree() || s->isHeld() ? -1 : s->trackedId);
	return true;
}

// BranchNode

BranchNode::BranchNode(var params) :
	LogicNode(getTypeStringStatic(), params)
{
	condition = addBoolParameter("Condition", "Used when the Condition input is not linked", false);
	addPin("In", LogicPin::EXEC, false);
	addPin("Condition", LogicPin::BOOL, false);
	addPin("True", LogicPin::EXEC, true);
	addPin("False", LogicPin::EXEC, true);
}

void BranchNode::execute(LogicGraph& g, const String&)
{
	if (!enabled->boolValue()) return;
	var v = g.getInput(this, "Condition");
	bool c = v.isVoid() ? condition->boolValue() : (bool)v;
	g.trigger(this, c ? "True" : "False");
}

// LogicThreshold

LogicThreshold::LogicThreshold(ControllableContainer* cc, const String& name, const String& help, double numberValue, double seconds, double percentValue, double metersValue)
{
	number = cc->addFloatParameter(name, help, numberValue);
	time = (FloatParameter*)cc->addParameter(new TimecodeParameter(name + " Time", help + " (HH:MM:SS:FF)", seconds));
	percent = (FloatParameter*)cc->addParameter(new PercentParameter(name + " Percentage", help + " (%)", percentValue));
	meters = cc->addFloatParameter(name + " Meters", help + " (meters)", metersValue);
	time->hideInEditor = true;
	percent->hideInEditor = true;
	meters->hideInEditor = true;
}

bool LogicThreshold::setUnit(LogicPin::Kind k)
{
	if (k != LogicPin::TIME && k != LogicPin::PERCENT && k != LogicPin::METERS) k = LogicPin::NUMBER;
	if (k == unit) return false;
	unit = k;
	number->hideInEditor = unit != LogicPin::NUMBER;
	time->hideInEditor = unit != LogicPin::TIME;
	percent->hideInEditor = unit != LogicPin::PERCENT;
	meters->hideInEditor = unit != LogicPin::METERS;
	return true;
}

FloatParameter* LogicThreshold::current() const
{
	switch (unit)
	{
	case LogicPin::TIME: return time;
	case LogicPin::PERCENT: return percent;
	case LogicPin::METERS: return meters;
	default: return number;
	}
}

String LogicThreshold::format(double v, LogicPin::Kind k)
{
	switch (k)
	{
	case LogicPin::TIME: return TimecodeParameter::toTimecode(v);
	case LogicPin::PERCENT: return PercentParameter::toPercent(v);
	case LogicPin::METERS: return String(v, 2) + " m";
	default: return v == std::floor(v) ? String((int64)v) : String(v, 3);
	}
}

//the unit of the thresholds follows the value linked to inPin ; the Inspector is rebuilt when it changes
static void updateThresholdUnits(LogicGraph& g, LogicNode* n, const String& inPin, Array<LogicThreshold*> thresholds)
{
	if (!g.isLinked(n, inPin)) return;
	LogicPin::Kind k = g.getInputKind(n, inPin, LogicPin::NUMBER);
	bool changed = false;
	for (auto& t : thresholds) changed |= t->setUnit(k);
	if (changed) n->queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, n));
}

static bool readBool(LogicGraph& g, LogicNode* n, const String& pin)
{
	var v = g.getInput(n, pin);
	return !v.isVoid() && (bool)v;
}

// CompareNode

CompareNode::CompareNode(var params) :
	LogicNode(getTypeStringStatic(), params),
	b(this, "B", "Value compared to A, when the B input is not linked", 0, 10, .5, 1)
{
	op = addEnumParameter("Operator", "Result is on when A (operator) B is true");
	op->addOption("=", EQUAL)->addOption("!=", NOT_EQUAL)->addOption(">", GREATER)->addOption(">=", GREATER_EQUAL)->addOption("<", LESS)->addOption("<=", LESS_EQUAL);
	op->setValueWithKey(">=");
	addPin("A", LogicPin::NUMBER, false);
	addPin("B", LogicPin::NUMBER, false);
	addPin("Result", LogicPin::BOOL, true);
}

String CompareNode::getSymbol() const
{
	switch ((Op)(int)op->getValueData())
	{
	case EQUAL: return "=";
	case NOT_EQUAL: return String(CharPointer_UTF8("\xe2\x89\xa0"));
	case GREATER: return ">";
	case GREATER_EQUAL: return String(CharPointer_UTF8("\xe2\x89\xa5"));
	case LESS: return "<";
	case LESS_EQUAL: return String(CharPointer_UTF8("\xe2\x89\xa4"));
	}
	return "?";
}

void CompareNode::linksChanged(LogicGraph& g)
{
	bLinked = g.isLinked(this, "B");
	updateThresholdUnits(g, this, "A", { &b });
}

var CompareNode::getOutputValue(LogicGraph& g, const String& outPin)
{
	if (outPin != "Result" || !enabled->boolValue()) return false;
	var av = g.getInput(this, "A");
	if (av.isVoid()) return false;
	var bv = g.getInput(this, "B");
	double x = (double)av;
	double y = bv.isVoid() ? b.get() : (double)bv;
	const double eps = 1e-6;
	switch ((Op)(int)op->getValueData())
	{
	case EQUAL: return std::abs(x - y) < eps;
	case NOT_EQUAL: return std::abs(x - y) >= eps;
	case GREATER: return x > y;
	case GREATER_EQUAL: return x >= y - eps;
	case LESS: return x < y;
	case LESS_EQUAL: return x <= y + eps;
	}
	return false;
}

// InRangeNode

InRangeNode::InRangeNode(var params) :
	LogicNode(getTypeStringStatic(), params),
	minValue(this, "Min", "Lowest value of the range, when the Min input is not linked", 0, 0, 0, 0),
	maxValue(this, "Max", "Highest value of the range, when the Max input is not linked", 1, 10, 1, 1)
{
	addPin("Value", LogicPin::NUMBER, false);
	addPin("Min", LogicPin::NUMBER, false);
	addPin("Max", LogicPin::NUMBER, false);
	addPin("Result", LogicPin::BOOL, true);
}

String InRangeNode::getSummary() const
{
	if (minLinked && maxLinked) return String();
	return (minLinked ? String("Min") : minValue.toText()) + " to " + (maxLinked ? String("Max") : maxValue.toText());
}

void InRangeNode::linksChanged(LogicGraph& g)
{
	minLinked = g.isLinked(this, "Min");
	maxLinked = g.isLinked(this, "Max");
	updateThresholdUnits(g, this, "Value", { &minValue, &maxValue });
}

var InRangeNode::getOutputValue(LogicGraph& g, const String& outPin)
{
	if (outPin != "Result" || !enabled->boolValue()) return false;
	var v = g.getInput(this, "Value");
	if (v.isVoid()) return false;
	var mn = g.getInput(this, "Min");
	var mx = g.getInput(this, "Max");
	double x = (double)v;
	double lo = mn.isVoid() ? minValue.get() : (double)mn;
	double hi = mx.isVoid() ? maxValue.get() : (double)mx;
	return x >= lo - 1e-6 && x <= hi + 1e-6;
}

// GateNode

GateNode::GateNode(const String& name, var params, Gate gate) :
	LogicNode(name, params),
	gate(gate)
{
	addPin("A", LogicPin::BOOL, false);
	if (gate != NOT) addPin("B", LogicPin::BOOL, false);
	addPin("Result", LogicPin::BOOL, true);
}

String GateNode::getSymbol() const
{
	switch (gate)
	{
	case AND: return "AND";
	case OR: return "OR";
	case XOR: return "XOR";
	default: return "NOT";
	}
}

var GateNode::getOutputValue(LogicGraph& g, const String& outPin)
{
	if (outPin != "Result" || !enabled->boolValue()) return false;
	bool a = readBool(g, this, "A");
	if (gate == NOT) return !a;
	bool bv = readBool(g, this, "B");
	switch (gate)
	{
	case AND: return a && bv;
	case OR: return a || bv;
	default: return a != bv;
	}
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
	//menu by type of value
	auto add = [this](Factory<LogicNode>::Definition* d) { factory.defs.add(d); };
	add(Factory<LogicNode>::Definition::createDef<OnStartNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<TimerNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<ZoneEnteredNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<ZoneExitedNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<SlotJoinedNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<SlotLeftNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<PersonAppearedNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<PersonLostNode>("Trigger"));
	add(Factory<LogicNode>::Definition::createDef<ZonePresenceNode>("On-Off"));
	add(Factory<LogicNode>::Definition::createDef<ZoneFullNode>("On-Off"));
	add(Factory<LogicNode>::Definition::createDef<SlotOccupiedNode>("On-Off"));
	add(Factory<LogicNode>::Definition::createDef<SlotHeldNode>("On-Off"));
	add(Factory<LogicNode>::Definition::createDef<PersonCountNode>("Count"));
	add(Factory<LogicNode>::Definition::createDef<ZonePersonCountNode>("Count"));
	add(Factory<LogicNode>::Definition::createDef<FreeSlotsNode>("Count"));
	add(Factory<LogicNode>::Definition::createDef<SlotPersonNode>("Count"));
	add(Factory<LogicNode>::Definition::createDef<ZonePresenceTimeNode>("Time"));
	add(Factory<LogicNode>::Definition::createDef<SlotHoldRemainingNode>("Time"));
	add(Factory<LogicNode>::Definition::createDef<PersonAgeNode>("Time"));
	add(Factory<LogicNode>::Definition::createDef<ZoneOccupancyNode>("Percentage"));
	add(Factory<LogicNode>::Definition::createDef<PersonRelativePositionNode>("Percentage"));
	add(Factory<LogicNode>::Definition::createDef<SlotRelativePositionNode>("Percentage"));
	add(Factory<LogicNode>::Definition::createDef<PersonPositionNode>("Meters"));
	add(Factory<LogicNode>::Definition::createDef<PersonHeightNode>("Meters"));
	add(Factory<LogicNode>::Definition::createDef<SlotPositionNode>("Meters"));
	add(Factory<LogicNode>::Definition::createDef<ZoneDistanceNode>("Meters"));
	add(Factory<LogicNode>::Definition::createDef<CompareNode>("Compare"));
	add(Factory<LogicNode>::Definition::createDef<InRangeNode>("Compare"));
	add(Factory<LogicNode>::Definition::createDef<AndNode>("Compare"));
	add(Factory<LogicNode>::Definition::createDef<OrNode>("Compare"));
	add(Factory<LogicNode>::Definition::createDef<XorNode>("Compare"));
	add(Factory<LogicNode>::Definition::createDef<NotNode>("Compare"));
	add(Factory<LogicNode>::Definition::createDef<BranchNode>("Flow"));
	add(Factory<LogicNode>::Definition::createDef<LogNode>("Outputs"));
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

// LogicGroup

LogicGroup::LogicGroup(var params) :
	BaseItem("Group", false)
{
	nameCanBeChangedByUser = true;
	color = addColorParameter("Color", "Color of the group : its frame and a tint on the nodes inside", Colour(0xff4f7fe0));
	collapsed = addBoolParameter("Collapsed", "Collapsed : the nodes inside are hidden behind the group (they keep running), its links leave from its edges", false);
	members = addStringParameter("Members", "Nodes kept inside while collapsed", "");
	members->hideInEditor = true;
	viewUISize->setPoint(400, 240);
}

Point<float> LogicGroup::getNodeAnchor(LogicNode* n)
{
	return n->viewUIPosition->getPoint() + Point<float>(30, 17);
}

bool LogicGroup::containsNode(LogicNode* n) const
{
	if (collapsed->boolValue()) return StringArray::fromTokens(members->stringValue(), ",", "").contains(n->uid->stringValue());
	return Rectangle<float>(viewUIPosition->getPoint().x, viewUIPosition->getPoint().y, viewUISize->getPoint().x, viewUISize->getPoint().y).contains(getNodeAnchor(n));
}

void LogicGroup::onContainerParameterChangedInternal(Parameter* p)
{
	//it collapses (panel, Inspector or redo) : the nodes in the frame now are the ones it keeps
	if (p != collapsed || !collapsed->boolValue() || isCurrentlyLoadingData) return;
	LogicGraph* graph = parentContainer != nullptr ? dynamic_cast<LogicGraph*>(parentContainer->parentContainer.get()) : nullptr;
	if (graph == nullptr) return;
	Rectangle<float> r(viewUIPosition->getPoint().x, viewUIPosition->getPoint().y, viewUISize->getPoint().x, viewUISize->getPoint().y);
	StringArray ids;
	for (auto& n : graph->nodes.items) if (r.contains(getNodeAnchor(n))) ids.add(n->uid->stringValue());
	members->setValue(ids.joinIntoString(","));
}

LogicGroupManager::LogicGroupManager() :
	BaseManager("Groups")
{
	itemDataType = "LogicGroup";
	selectItemWhenCreated = false;
}

LogicGroupManager::~LogicGroupManager()
{
}

// LogicGraph

LogicGraph::LogicGraph(SpatialModule* module) :
	BaseItem("Graph"),
	module(module)
{
	nodes.module = module;
	nodes.addBaseManagerListener(this);
	addChildControllableContainer(&nodes);
	addChildControllableContainer(&groups);
	saveAndLoadRecursiveData = true;
	nodes.editorIsCollapsed = true;
	groups.editorIsCollapsed = true;
}

Array<LogicNode*> LogicGraph::getGroupNodes(LogicGroup* g) const
{
	Array<LogicNode*> result;
	for (auto& n : nodes.items) if (g->containsNode(n)) result.add(n);
	return result;
}

LogicGroup* LogicGraph::getGroupOf(LogicNode* n) const
{
	for (int i = groups.items.size() - 1; i >= 0; i--) if (groups.items[i]->containsNode(n)) return groups.items[i];
	return nullptr;
}

LogicGroup* LogicGraph::getCollapsedGroupOf(LogicNode* n) const
{
	for (auto& g : groups.items) if (g->collapsed->boolValue() && g->containsNode(n)) return g;
	return nullptr;
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
	notifyLinksChanged(l.toNode);
}

void LogicGraph::removeLinkInternal(const Link& l)
{
	links.removeFirstMatchingValue(l);
	notifyLinksChanged(l.toNode);
}

void LogicGraph::notifyLinksChanged(const String& nodeUid)
{
	for (auto& n : nodes.items) if (nodeUid.isEmpty() || n->uid->stringValue() == nodeUid) n->linksChanged(*this);
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

bool LogicGraph::isLinked(LogicNode* node, const String& inPin) const
{
	String id = node->uid->stringValue();
	for (auto& l : links) if (l.toNode == id && l.toPin == inPin) return true;
	return false;
}

LogicPin::Kind LogicGraph::getInputKind(LogicNode* node, const String& inPin, LogicPin::Kind fallback) const
{
	String id = node->uid->stringValue();
	for (auto& l : links)
	{
		if (l.toNode != id || l.toPin != inPin) continue;
		if (LogicNode* n = getNode(l.fromNode))
			if (const LogicPinDef* p = n->getPin(l.fromPin, true)) return p->kind;
	}
	return fallback;
}

void LogicGraph::itemRemoved(LogicNode* n)
{
	String id = n->uid->stringValue();
	links.removeIf([id](const Link& l) { return l.fromNode == id || l.toNode == id; });
	notifyLinksChanged(String());
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
	notifyLinksChanged(String());
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

// one line of help per node, shown in the create menu of the Logic Graph panel
String LogicNodeManager::getDescription(const String& type)
{
	static const std::map<String, String> help = {
		{ "On Start", "Once when the graph starts" }, { "Timer", "Every N seconds" },
		{ "Zone Entered", "A person enters a zone" }, { "Zone Exited", "A person leaves a zone" },
		{ "Slot Joined", "A person takes a player slot" }, { "Slot Left", "A player slot is released" },
		{ "Person Appeared", "A new person is tracked" }, { "Person Lost", "A person is no longer tracked" },
		{ "Zone Presence", "Someone is in the zone" }, { "Zone Full", "The zone reached its capacity" },
		{ "Slot Occupied", "The slot has a player" }, { "Slot Held", "The slot waits for a lost player" },
		{ "Person Count", "People tracked" }, { "Zone Person Count", "People in a zone" },
		{ "Free Slots", "Player slots still free" }, { "Slot Person", "Id of the person in a slot" },
		{ "Zone Presence Time", "Time since the zone is occupied" }, { "Slot Hold Remaining", "Time before a held slot is released" },
		{ "Person Age", "Time since the person appeared" }, { "Zone Occupancy", "People / capacity of a zone" },
		{ "Person Relative Position", "0-1 in its zone and on its surface" }, { "Slot Relative Position", "0-1 of the player of a slot" },
		{ "Person Position", "X / Y in meters" }, { "Person Height", "Height in meters" },
		{ "Slot Position", "X / Y of a player slot" }, { "Zone Average Distance To Center", "Mean distance of the people to the centre" },
		{ "Compare", "= != > >= < <= a threshold" }, { "In Range", "Between two values" },
		{ "And", "On when both are On" }, { "Or", "On when one is On" }, { "Xor", "On when only one is On" }, { "Not", "Inverts On / Off" },
		{ "Branch", "Goes on by True or False" }, { "Log", "Writes a line in the Logger" },
	};
	auto it = help.find(type);
	return it != help.end() ? it->second : String();
}
