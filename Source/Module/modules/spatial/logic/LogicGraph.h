/*
  ==============================================================================

	LogicGraph.h
	Created: 6 Nov 2026
	Author:  DrawZone

	Logic Graph (v0.7.2, D18) : node graphs of the Cockpit module, Blueprint
	style. Logic block -> named graphs -> nodes (+ links between their pins).
	Two kinds of links : exec (what runs next, white) and data (a value read
	when a node runs).

	The graphs run in the module timer, right after the zones and the slots
	(SpatialModule::updateZones), never in the panel : closing the Logic Graph
	panel or Live Mode never stops them.

	A node is a BaseItem : its settings show in the Inspector, its place in
	the panel is BaseItem::viewUIPosition. Links name their nodes by a hidden
	uid, so renaming or reordering nodes keeps them.

  ==============================================================================
*/

#pragma once

class SpatialModule;
class SpatialPerson;
class LogicGraph;

namespace LogicPin
{
	enum Kind { EXEC, NUMBER, BOOL, TEXT, ANY, TIME, PERCENT, METERS };	// TIME : seconds shown HH:MM:SS:FF, PERCENT : 0-1 shown in %
	Colour getColour(Kind k);
	//an output can feed an input : exec to exec, data to data (ANY takes every data kind) ; NUMBER, TIME, PERCENT and METERS are all numbers
	bool canConnect(Kind out, Kind in);
}

struct LogicPinDef
{
	String name;
	LogicPin::Kind kind = LogicPin::EXEC;
	bool output = false;
};

class LogicNode :
	public BaseItem
{
public:
	LogicNode(const String& name = "Node", var params = var());
	virtual ~LogicNode();

	enum Category { EVENT, FLOW, READ, ACTION, VARIABLE, COMPARE };

	StringParameter* uid;	// hidden, saved : what links point to
	SpatialModule* module = nullptr;
	Array<LogicPinDef> pins;

	virtual Category getCategory() const { return FLOW; }
	virtual int getWidth() const { return 220; }
	virtual void setup(SpatialModule* m) { module = m; }	// the module is known : target parameters can be set up

	//engine, message thread, module timer
	virtual void reset() {}											// the graph starts : forget what was seen before
	virtual void process(LogicGraph&) {}							// each update (events look for what happened)
	virtual void execute(LogicGraph&, const String& inPin) {}		// an exec link reached inPin
	virtual var getOutputValue(LogicGraph&, const String& outPin) { return var(); }
	virtual void linksChanged(LogicGraph&) {}						// a link to this node was added or removed, or the graph loaded

	//not empty : the node is drawn as a pill with this symbol, without header (compare nodes) ; the summary goes in a chip next to it
	virtual String getSymbol() const { return String(); }

	//inputs on the left, outputs on the right, one row each from the top : row of a pin, -1 if unknown
	int getPinRow(const String& name, bool output) const;
	int getNumRows() const;
	const LogicPinDef* getPin(const String& name, bool output) const;

	static Colour getCategoryColour(Category c);
	static String getCategoryName(Category c);

protected:
	void addPin(const String& name, LogicPin::Kind kind, bool output);
	//one line of the node shows this setting (zone name, message...), next to the first row
	virtual String getSummary() const { return String(); }

public:
	String getSummaryText() const { return getSummary(); }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogicNode)
};

// --- event nodes (brick 3) : one node per value of the Cockpit, by type
//   trigger (something happened) : Then + who
//   boolean : On (it becomes true), Off (it becomes false), State
//   integer : Changed, Count
// What is already true or counted when the graph starts does not fire.

namespace LogicTargets
{
	void setupZone(TargetParameter* t, SpatialModule* m);
	SpatialZone* getZone(TargetParameter* t);			// nullptr without zone
	String getZoneName(TargetParameter* t);
	SpatialSurface* getSurface(TargetParameter* t, SpatialModule* m);	// empty target : the first surface
	String getSlotSummary(TargetParameter* surface, IntParameter* slot);
}

//base of the boolean nodes
class StateNode :
	public LogicNode
{
public:
	StateNode(const String& name, var params);

	Category getCategory() const override { return EVENT; }
	void reset() override { primed = false; }
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;

protected:
	virtual bool getState(bool& valid) const = 0;	// valid = false : nothing to watch (no zone...)

private:
	bool lastState = false;
	bool primed = false;
};

//base of the integer nodes
class CountNode :
	public LogicNode
{
public:
	CountNode(const String& name, var params);

	Category getCategory() const override { return EVENT; }
	void reset() override { primed = false; }
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;

protected:
	virtual bool getCount(int& count) const = 0;	// false : nothing to watch

private:
	int lastCount = 0;
	bool primed = false;
};

// - triggers

//a person enters (Entered) or leaves (Exited) the zone : same as the Entered / Exited triggers of the zone
class ZoneTriggerNode :
	public LogicNode
{
public:
	ZoneTriggerNode(const String& name, var params, bool entered);

	TargetParameter* zoneTarget;
	const bool entered;

	Category getCategory() const override { return EVENT; }
	void setup(SpatialModule* m) override;
	void reset() override { lastIds.clear(); primed = false; }
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;
	String getSummary() const override { return LogicTargets::getZoneName(zoneTarget); }

private:
	Array<int> lastIds;
	int firedId = -1;
	bool primed = false;
};

class ZoneEnteredNode : public ZoneTriggerNode
{
public:
	ZoneEnteredNode(var params = var()) : ZoneTriggerNode(getTypeStringStatic(), params, true) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Entered"; }
};

class ZoneExitedNode : public ZoneTriggerNode
{
public:
	ZoneExitedNode(var params = var()) : ZoneTriggerNode(getTypeStringStatic(), params, false) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Exited"; }
};

//a person takes (Joined) or loses (Left) a player slot : same as the Joined / Left triggers of the slot
class SlotTriggerNode :
	public LogicNode
{
public:
	SlotTriggerNode(const String& name, var params, bool joined);

	TargetParameter* surfaceTarget;
	IntParameter* slotNumber;	// 0 = any slot
	const bool joined;

	Category getCategory() const override { return EVENT; }
	void setup(SpatialModule* m) override;
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;
	String getSummary() const override { return LogicTargets::getSlotSummary(surfaceTarget, slotNumber); }

private:
	int firedSlot = 0, firedId = -1;
};

class SlotJoinedNode : public SlotTriggerNode
{
public:
	SlotJoinedNode(var params = var()) : SlotTriggerNode(getTypeStringStatic(), params, true) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Slot Joined"; }
};

class SlotLeftNode : public SlotTriggerNode
{
public:
	SlotLeftNode(var params = var()) : SlotTriggerNode(getTypeStringStatic(), params, false) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Slot Left"; }
};

//a person appears in the tracking (Appeared) or is lost (Lost), every source and surface
class PersonTriggerNode :
	public LogicNode
{
public:
	PersonTriggerNode(const String& name, var params, bool appeared);

	const bool appeared;

	Category getCategory() const override { return EVENT; }
	void reset() override { lastIds.clear(); primed = false; }
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;

private:
	Array<int> lastIds;
	int firedId = -1;
	bool primed = false;
};

class PersonAppearedNode : public PersonTriggerNode
{
public:
	PersonAppearedNode(var params = var()) : PersonTriggerNode(getTypeStringStatic(), params, true) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Person Appeared"; }
};

class PersonLostNode : public PersonTriggerNode
{
public:
	PersonLostNode(var params = var()) : PersonTriggerNode(getTypeStringStatic(), params, false) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Person Lost"; }
};

// - booleans

//Presence of the zone : at least one person in it
class ZonePresenceNode :
	public StateNode
{
public:
	ZonePresenceNode(var params = var());
	TargetParameter* zoneTarget;
	void setup(SpatialModule* m) override;
	bool getState(bool& valid) const override;
	String getSummary() const override { return LogicTargets::getZoneName(zoneTarget); }
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Presence"; }
};

//Person Count of the zone >= its Capacity
class ZoneFullNode :
	public StateNode
{
public:
	ZoneFullNode(var params = var());
	TargetParameter* zoneTarget;
	void setup(SpatialModule* m) override;
	bool getState(bool& valid) const override;
	String getSummary() const override { return LogicTargets::getZoneName(zoneTarget); }
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Full"; }
};

//Occupied (has a person, or is held for one) or Held (waits for a lost person) of one player slot
class SlotStateNode :
	public StateNode
{
public:
	SlotStateNode(const String& name, var params, bool held);
	TargetParameter* surfaceTarget;
	IntParameter* slotNumber;	// from 1
	const bool held;
	void setup(SpatialModule* m) override;
	bool getState(bool& valid) const override;
	String getSummary() const override { return LogicTargets::getSlotSummary(surfaceTarget, slotNumber); }
};

class SlotOccupiedNode : public SlotStateNode
{
public:
	SlotOccupiedNode(var params = var()) : SlotStateNode(getTypeStringStatic(), params, false) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Slot Occupied"; }
};

class SlotHeldNode : public SlotStateNode
{
public:
	SlotHeldNode(var params = var()) : SlotStateNode(getTypeStringStatic(), params, true) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Slot Held"; }
};

// - integers

//Person Count of the Cockpit : people tracked, every source and surface
class PersonCountNode :
	public CountNode
{
public:
	PersonCountNode(var params = var()) : CountNode(getTypeStringStatic(), params) {}
	bool getCount(int& count) const override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Person Count"; }
};

//Person Count of the zone
class ZonePersonCountNode :
	public CountNode
{
public:
	ZonePersonCountNode(var params = var());
	TargetParameter* zoneTarget;
	void setup(SpatialModule* m) override;
	bool getCount(int& count) const override;
	String getSummary() const override { return LogicTargets::getZoneName(zoneTarget); }
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Person Count"; }
};

//Free Slots of a surface
class FreeSlotsNode :
	public CountNode
{
public:
	FreeSlotsNode(var params = var());
	TargetParameter* surfaceTarget;
	void setup(SpatialModule* m) override;
	bool getCount(int& count) const override;
	String getSummary() const override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Free Slots"; }
};

// - time

//fires once when the graph starts (session loaded, graph enabled)
class OnStartNode :
	public LogicNode
{
public:
	OnStartNode(var params = var());

	Category getCategory() const override { return EVENT; }
	void reset() override { pending = true; }
	void process(LogicGraph& g) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "On Start"; }

private:
	bool pending = false;
};

//fires every Interval (at the rate of the zones, 50 Hz at most)
class TimerNode :
	public LogicNode
{
public:
	TimerNode(var params = var());

	FloatParameter* interval;	// seconds, shown HH:MM:SS:FF

	Category getCategory() const override { return EVENT; }
	void reset() override { lastMs = 0; count = 0; }
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;
	String getSummary() const override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Timer"; }

private:
	double lastMs = 0;
	int count = 0;
};

// --- value nodes : a value of the Cockpit (time, percentage, meters) + Changed, fired each time it changes

class ValueNode :
	public LogicNode
{
public:
	ValueNode(const String& name, var params);

	Category getCategory() const override { return READ; }
	void reset() override { primed = false; }
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;

protected:
	void addValuePin(const String& name, LogicPin::Kind kind);
	virtual bool read(LogicGraph& g, Array<var>& values) = 0;	// one value per value pin, in order ; false : nothing to read

private:
	bool readSafe(LogicGraph& g, Array<var>& values);	// against a loop of data links
	StringArray valueNames;
	Array<var> lastValues;
	bool primed = false;
	bool reading = false;
};

//value of a zone
class ZoneValueNode :
	public ValueNode
{
public:
	ZoneValueNode(const String& name, var params, const String& help);
	TargetParameter* zoneTarget;
	void setup(SpatialModule* m) override;
	String getSummary() const override { return LogicTargets::getZoneName(zoneTarget); }
};

//value of one player slot
class SlotValueNode :
	public ValueNode
{
public:
	SlotValueNode(const String& name, var params);
	TargetParameter* surfaceTarget;
	IntParameter* slotNumber;	// from 1
	void setup(SpatialModule* m) override;
	PlayerSlot* getSlot() const;
	String getSummary() const override { return LogicTargets::getSlotSummary(surfaceTarget, slotNumber); }
};

//value of one tracked person : the Person Id setting, or the Person Id input when it is linked
class PersonValueNode :
	public ValueNode
{
public:
	PersonValueNode(const String& name, var params);
	IntParameter* personId;
	SpatialPerson* getPerson(LogicGraph& g) const;	// nullptr when not tracked
	String getSummary() const override { return "person " + String(personId->intValue()) + " (or input)"; }
};

class ZonePresenceTimeNode : public ZoneValueNode
{
public:
	ZonePresenceTimeNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Presence Time"; }
};

class ZoneOccupancyNode : public ZoneValueNode
{
public:
	ZoneOccupancyNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Occupancy"; }
};

class ZoneDistanceNode : public ZoneValueNode
{
public:
	ZoneDistanceNode(var params = var());
	int getWidth() const override { return 280; }
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Average Distance To Center"; }
};

class SlotHoldRemainingNode : public SlotValueNode
{
public:
	SlotHoldRemainingNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Slot Hold Remaining"; }
};

class SlotPositionNode : public SlotValueNode
{
public:
	SlotPositionNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Slot Position"; }
};

class PersonAgeNode : public PersonValueNode
{
public:
	PersonAgeNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Person Age"; }
};

class PersonPositionNode : public PersonValueNode
{
public:
	PersonPositionNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Person Position"; }
};

class PersonHeightNode : public PersonValueNode
{
public:
	PersonHeightNode(var params = var());
	bool read(LogicGraph& g, Array<var>& values) override;
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Person Height"; }
};

//In -> True when the Condition input is on, False otherwise (the Condition setting when the input is not linked)
class BranchNode :
	public LogicNode
{
public:
	BranchNode(var params = var());

	BoolParameter* condition;

	void execute(LogicGraph& g, const String& inPin) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Branch"; }
};

// --- compare nodes : drawn as pills, a Result (On-Off) read by Branch

//a setting compared to an input, shown in the unit of the value linked to that input :
//one parameter per unit, only the one of the current unit is shown in the Inspector
class LogicThreshold
{
public:
	LogicThreshold(ControllableContainer* cc, const String& name, const String& help, double number, double seconds, double percent, double meters);

	FloatParameter* number;
	FloatParameter* time;
	FloatParameter* percent;
	FloatParameter* meters;
	LogicPin::Kind unit = LogicPin::NUMBER;

	bool setUnit(LogicPin::Kind k);	// true when it changed : the Inspector must be rebuilt
	FloatParameter* current() const;
	double get() const { return current()->floatValue(); }
	String toText() const { return format(get(), unit); }
	static String format(double v, LogicPin::Kind k);
};

class CompareNode :
	public LogicNode
{
public:
	CompareNode(var params = var());

	enum Op { EQUAL, NOT_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL };
	EnumParameter* op;
	LogicThreshold b;

	Category getCategory() const override { return COMPARE; }
	int getWidth() const override { return 176; }
	String getSymbol() const override;
	String getSummary() const override { return bLinked ? String() : b.toText(); }
	void linksChanged(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Compare"; }

private:
	bool bLinked = false;
};

//Result on when Min <= Value <= Max
class InRangeNode :
	public LogicNode
{
public:
	InRangeNode(var params = var());

	LogicThreshold minValue, maxValue;

	Category getCategory() const override { return COMPARE; }
	int getWidth() const override { return 260; }
	String getSymbol() const override { return "IN"; }
	String getSummary() const override;
	void linksChanged(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "In Range"; }

private:
	bool minLinked = false, maxLinked = false;
};

//And, Or, Xor, Not of On-Off inputs (an input not linked is off)
class GateNode :
	public LogicNode
{
public:
	enum Gate { AND, OR, XOR, NOT };
	GateNode(const String& name, var params, Gate gate);
	const Gate gate;

	Category getCategory() const override { return COMPARE; }
	int getWidth() const override { return 120; }
	String getSymbol() const override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;
};

class AndNode : public GateNode
{
public:
	AndNode(var params = var()) : GateNode(getTypeStringStatic(), params, AND) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "And"; }
};

class OrNode : public GateNode
{
public:
	OrNode(var params = var()) : GateNode(getTypeStringStatic(), params, OR) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Or"; }
};

class XorNode : public GateNode
{
public:
	XorNode(var params = var()) : GateNode(getTypeStringStatic(), params, XOR) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Xor"; }
};

class NotNode : public GateNode
{
public:
	NotNode(var params = var()) : GateNode(getTypeStringStatic(), params, NOT) {}
	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Not"; }
};

//writes a line in the Chataigne logger
class LogNode :
	public LogicNode
{
public:
	LogNode(var params = var());

	StringParameter* message;

	Category getCategory() const override { return ACTION; }
	void execute(LogicGraph& g, const String& inPin) override;
	String getSummary() const override { return message->stringValue(); }

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Log"; }
};

class LogicNodeManager :
	public BaseManager<LogicNode>
{
public:
	LogicNodeManager();
	~LogicNodeManager();

	Factory<LogicNode> factory;
	SpatialModule* module = nullptr;	// set by the graph before anything is added

	LogicNode* createItemFromData(var data) override;	// the module is given before the node loads (its targets)
	void addItemInternal(LogicNode* item, var data) override;
};

class LogicGraph :
	public BaseItem,
	public BaseManager<LogicNode>::ManagerListener
{
public:
	LogicGraph(SpatialModule* module = nullptr);
	~LogicGraph();

	struct Link
	{
		String fromNode, fromPin, toNode, toPin;	// node uids, pin names
		bool operator==(const Link& o) const { return fromNode == o.fromNode && fromPin == o.fromPin && toNode == o.toNode && toPin == o.toPin; }
	};

	SpatialModule* module;
	LogicNodeManager nodes;
	Array<Link> links;

	LogicNode* getNode(const String& uid) const;

	//undoable ; an exec output and a data input take one link only : the old one goes
	void connect(const Link& l);
	void disconnect(const Link& l);
	Array<UndoableAction*> getDisconnectNodeActions(LogicNode* n);	// its links, before removing it

	//raw, for the undo actions
	void addLinkInternal(const Link& l);
	void removeLinkInternal(const Link& l);
	void notifyLinksChanged(const String& nodeUid);	// empty : every node

	//engine
	void process();
	void trigger(LogicNode* from, const String& outPin);	// follows the exec links of outPin
	var getInput(LogicNode* node, const String& inPin);		// value of the data link of inPin, void if none
	bool isLinked(LogicNode* node, const String& inPin) const;
	LogicPin::Kind getInputKind(LogicNode* node, const String& inPin, LogicPin::Kind fallback) const;	// kind of the output linked to inPin

	void itemRemoved(LogicNode* n) override;
	void itemsRemoved(Array<LogicNode*> ns) override;

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternal(var data) override;

	String getTypeString() const override { return "Graph"; }

private:
	bool running = false;	// false while disabled : the nodes are reset when it runs again
	int depth = 0;	// exec chain depth in one update, against loops
	static const int maxDepth = 64;

	class LinkAction :
		public UndoableAction
	{
	public:
		LinkAction(LogicGraph* g, const Link& l, bool add) : graphRef(g), link(l), add(add) {}
		WeakReference<ControllableContainer> graphRef;
		Link link;
		bool add;
		bool perform() override;
		bool undo() override;
	};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogicGraph)
};

class LogicGraphManager :
	public SpatialSectionManager<LogicGraph>
{
public:
	LogicGraphManager();
	~LogicGraphManager();

	SpatialModule* module = nullptr;	// set by the module before anything is added
	LogicGraph* createItem() override { return new LogicGraph(module); }

	void process();	// every graph, module timer
};
