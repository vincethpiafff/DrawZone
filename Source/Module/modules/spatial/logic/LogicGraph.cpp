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
	default: return Colour(0xffa7adb7);
	}
}

bool LogicPin::canConnect(Kind out, Kind in)
{
	if (out == EXEC || in == EXEC) return out == in;
	return in == ANY || out == ANY || in == out;
}

// LogicNode

LogicNode::LogicNode(const String& name, var params) :
	BaseItem(name)
{
	uid = addStringParameter("Uid", "Identifier of this node for the links", Uuid().toString());
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

// ZoneEnteredNode

ZoneEnteredNode::ZoneEnteredNode(var params) :
	LogicNode(getTypeStringStatic(), params)
{
	zoneTarget = addTargetParameter("Zone", "Zone watched : this event fires once for each person who enters it");
	addPin("Then", LogicPin::EXEC, true);
	addPin("Person Id", LogicPin::NUMBER, true);
}

void ZoneEnteredNode::setup(SpatialModule* m)
{
	LogicNode::setup(m);
	if (m == nullptr) return;
	zoneTarget->targetType = TargetParameter::CONTAINER;
	zoneTarget->maxDefaultSearchLevel = 0;
	zoneTarget->defaultContainerTypeCheckFunc = [](ControllableContainer* cc) { return dynamic_cast<SpatialZone*>(cc) != nullptr; };
	zoneTarget->setRootContainer(&m->zoneManager, false, true);
}

void ZoneEnteredNode::process(LogicGraph& g)
{
	SpatialZone* z = dynamic_cast<SpatialZone*>(zoneTarget->targetContainer.get());
	if (z == nullptr || !enabled->boolValue())
	{
		lastIds.clear();
		primed = false;
		return;
	}

	Array<int> ids = z->idsInside;
	if (primed)
	{
		for (int id : ids)
		{
			if (lastIds.contains(id)) continue;
			firedId = id;
			g.trigger(this, "Then");
		}
	}
	lastIds = ids;
	primed = true;
}

var ZoneEnteredNode::getOutputValue(LogicGraph&, const String& outPin)
{
	return outPin == "Person Id" ? var(firedId) : var();
}

String ZoneEnteredNode::getSummary() const
{
	ControllableContainer* z = zoneTarget->targetContainer.get();
	return z != nullptr ? z->niceName : String("(no zone)");
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
	factory.defs.add(Factory<LogicNode>::Definition::createDef<ZoneEnteredNode>("Events"));
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
	if (!enabled->boolValue()) return;
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
