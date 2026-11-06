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
class LogicGraph;

namespace LogicPin
{
	enum Kind { EXEC, NUMBER, BOOL, TEXT, ANY };
	Colour getColour(Kind k);
	//an output can feed an input : exec to exec, data to data (ANY takes every data kind)
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

	enum Category { EVENT, FLOW, READ, ACTION, VARIABLE };

	StringParameter* uid;	// hidden, saved : what links point to
	SpatialModule* module = nullptr;
	Array<LogicPinDef> pins;

	virtual Category getCategory() const { return FLOW; }
	virtual int getWidth() const { return 220; }
	virtual void setup(SpatialModule* m) { module = m; }	// the module is known : target parameters can be set up

	//engine, message thread, module timer
	virtual void process(LogicGraph&) {}							// each update (events look for what happened)
	virtual void execute(LogicGraph&, const String& inPin) {}		// an exec link reached inPin
	virtual var getOutputValue(LogicGraph&, const String& outPin) { return var(); }

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

// --- brick 1 nodes : enough to see a graph run

//fires once for each person who enters the zone (same membership as the zone, with its anti-flicker)
class ZoneEnteredNode :
	public LogicNode
{
public:
	ZoneEnteredNode(var params = var());

	TargetParameter* zoneTarget;

	Category getCategory() const override { return EVENT; }
	void setup(SpatialModule* m) override;
	void process(LogicGraph& g) override;
	var getOutputValue(LogicGraph& g, const String& outPin) override;
	String getSummary() const override;

	String getTypeString() const override { return getTypeStringStatic(); }
	static String getTypeStringStatic() { return "Zone Entered"; }

private:
	Array<int> lastIds;
	int firedId = -1;
	bool primed = false;	// the people already inside when the graph starts do not fire
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

	//engine
	void process();
	void trigger(LogicNode* from, const String& outPin);	// follows the exec links of outPin
	var getInput(LogicNode* node, const String& inPin);		// value of the data link of inPin, void if none

	void itemRemoved(LogicNode* n) override;
	void itemsRemoved(Array<LogicNode*> ns) override;

	var getJSONData(bool includeNonOverriden = false) override;
	void loadJSONDataInternal(var data) override;

	String getTypeString() const override { return "Graph"; }

private:
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
