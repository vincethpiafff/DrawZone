/*
  ==============================================================================

	LogicGraphPanel.cpp
	Created: 6 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

namespace LogicGraphStyle
{
	const Colour canvasBg(0xff0c0c0c);
	const Colour dots(0xff262a31);
	const Colour headerBg(0xff131518);
	const Colour nodeBg(0xff16181c);
	const Colour nodeBorder(0xff2b2f36);
	const Colour nodeLine(0xff22252b);
	const Colour chipBg(0xff202329);
	const Colour text(0xffeef0f3);
	const Colour dim(0xffa7adb7);
	const Colour accent(0xff4fb3ff);
	const Colour live(0xffe5484d);

	inline void drawToggleChip(Graphics& g, Rectangle<float> r, bool on, const String& label)
	{
		g.setColour(nodeBg);
		g.fillRoundedRectangle(r, r.getHeight() / 2);
		g.setColour(nodeBorder);
		g.drawRoundedRectangle(r.reduced(.5f), r.getHeight() / 2, 1);
		Rectangle<float> box = Rectangle<float>(14, 14).withCentre(Point<float>(r.getX() + 16, r.getCentreY()));
		if (on)
		{
			g.setColour(accent);
			g.fillRoundedRectangle(box, 4);
			TrackingMonitorStyle::drawTick(g, Point<float>(box.getX() + 3.5f, box.getCentreY()), Point<float>(box.getX() + 6, box.getBottom() - 4),
				Point<float>(box.getRight() - 3.5f, box.getY() + 4), canvasBg);
		}
		else
		{
			g.setColour(Colour(0xff6b717c));
			g.drawRoundedRectangle(box.reduced(.75f), 4, 1.5f);
		}
		g.setColour(on ? text : dim);
		g.setFont(Font(12.0f));
		g.drawText(label, r.withTrimmedLeft(28).withTrimmedRight(8), Justification::centredLeft, true);
	}

	//exec = diamond, data = ring, filled when linked
	inline void drawPin(Graphics& g, Point<float> s, LogicPin::Kind kind, bool linked, float size, float alpha, float zoom)
	{
		Colour c = LogicPin::getColour(kind);
		if (kind == LogicPin::EXEC)
		{
			Path d;
			d.addRectangle(-size / 2.8f, -size / 2.8f, size / 1.4f, size / 1.4f);
			d.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 4).translated(s));
			g.setColour(c.withMultipliedAlpha(linked ? alpha : alpha * .7f));
			g.fillPath(d);
			return;
		}
		Rectangle<float> e = Rectangle<float>(size, size).withCentre(s);
		g.setColour(linked ? c : canvasBg);
		g.fillEllipse(e);
		g.setColour(c.withMultipliedAlpha(alpha));
		g.drawEllipse(e.reduced(1 * zoom), 2 * zoom);
	}

	inline void drawCategoryIcon(Graphics& g, LogicNode::Category c, Rectangle<float> r)
	{
		Colour col = LogicNode::getCategoryColour(c);
		g.setColour(col.withAlpha(.18f));
		g.fillRoundedRectangle(r, r.getWidth() * .3f);
		g.setColour(col);
		Rectangle<float> i = r.reduced(r.getWidth() * .22f);
		Path p;
		switch (c)
		{
		case LogicNode::EVENT:
			p.startNewSubPath(i.getX() + i.getWidth() * .6f, i.getY());
			p.lineTo(i.getX() + i.getWidth() * .2f, i.getY() + i.getHeight() * .58f);
			p.lineTo(i.getX() + i.getWidth() * .5f, i.getY() + i.getHeight() * .58f);
			p.lineTo(i.getX() + i.getWidth() * .4f, i.getBottom());
			p.lineTo(i.getX() + i.getWidth() * .8f, i.getY() + i.getHeight() * .42f);
			p.lineTo(i.getX() + i.getWidth() * .5f, i.getY() + i.getHeight() * .42f);
			p.closeSubPath();
			g.fillPath(p);
			return;
		case LogicNode::READ: p.addEllipse(i.reduced(i.getWidth() * .1f)); break;
		case LogicNode::ACTION:
			p.startNewSubPath(i.getX(), i.getCentreY()); p.lineTo(i.getRight(), i.getCentreY());
			p.startNewSubPath(i.getCentreX(), i.getY()); p.lineTo(i.getRight(), i.getCentreY()); p.lineTo(i.getCentreX(), i.getBottom());
			break;
		case LogicNode::VARIABLE: p.addRoundedRectangle(i.reduced(i.getWidth() * .08f), i.getWidth() * .2f); break;
		default:
			p.startNewSubPath(i.getX(), i.getCentreY()); p.lineTo(i.getCentreX(), i.getCentreY());
			p.lineTo(i.getRight(), i.getY()); p.startNewSubPath(i.getCentreX(), i.getCentreY()); p.lineTo(i.getRight(), i.getBottom());
			break;
		}
		g.strokePath(p, PathStrokeType(jmax(1.0f, r.getWidth() * .08f), PathStrokeType::curved, PathStrokeType::rounded));
	}
}

LogicGraphPanel::LogicGraphPanel(const String& contentName) :
	ShapeShifterContentComponent(contentName)
{
	setWantsKeyboardFocus(true);
	startTimerHz(refreshRateHz);
}

LogicGraphPanel::~LogicGraphPanel()
{
	stopTimer();
}

SpatialModule* LogicGraphPanel::getSpatialModule()
{
	for (auto& m : ModuleManager::getInstance()->items)
		if (SpatialModule* sm = dynamic_cast<SpatialModule*>(m)) return sm;
	return nullptr;
}

LogicGraph* LogicGraphPanel::getGraph()
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr || sm->logicManager.items.isEmpty()) return nullptr;
	graphIndex = jlimit(0, sm->logicManager.items.size() - 1, graphIndex);
	return sm->logicManager.items[graphIndex];
}

// --- layout and view

void LogicGraphPanel::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(8);
	headerVisibleRect = r.removeFromTop(30);

	//natural width : title, every tab, +, the chips on the right
	Font tabFont(12.0f);
	int tabsWidth = 0;
	if (SpatialModule* sm = getSpatialModule())
		for (auto& gr : sm->logicManager.items) tabsWidth += jlimit(48, 180, (int)tabFont.getStringWidthFloat(gr->niceName) + 26) + 6;
	const int neededWidth = 112 + tabsWidth + 30 + 12 + 140 + 8 + 80 + 8 + 64 + 8 + 64 + 8;
	headerOverflow = jmax(0, neededWidth - headerVisibleRect.getWidth());
	headerScroll = jlimit(0, headerOverflow, headerScroll);
	headerRect = headerOverflow > 0 ? headerVisibleRect.withX(headerVisibleRect.getX() - headerScroll).withWidth(neededWidth) : headerVisibleRect;
	r.removeFromTop(6);
	canvasRect = r;
	miniMapRect = Rectangle<int>(200, 120).withPosition(canvasRect.getRight() - 212, canvasRect.getBottom() - 132);

	Rectangle<int> h = headerRect;
	liveRect = h.removeFromRight(64).reduced(0, 2);
	h.removeFromRight(8);
	zoomRect = h.removeFromRight(64).reduced(0, 2);
	h.removeFromRight(8);
	gridRect = h.removeFromRight(80).reduced(0, 2);
	h.removeFromRight(8);
	snapNodesRect = h.removeFromRight(140).reduced(0, 2);

	//tabs : after the title, as wide as their names
	tabRects.clearQuick();
	h = headerRect.withRight(snapNodesRect.getX() - 12).withTrimmedLeft(112);
	if (SpatialModule* sm = getSpatialModule())
	{
		for (auto& gr : sm->logicManager.items)
		{
			int w = jlimit(48, 180, (int)tabFont.getStringWidthFloat(gr->niceName) + 26);
			if (h.getWidth() < w) break;
			tabRects.add(h.removeFromLeft(w).reduced(0, 2));
			h.removeFromLeft(6);
		}
	}
	addTabRect = h.getWidth() >= 30 ? h.removeFromLeft(30).reduced(0, 2) : Rectangle<int>();
}

Point<float> LogicGraphPanel::toScreen(Point<float> c) const
{
	return canvasRect.getPosition().toFloat() + (c - viewOrigin) * zoom;
}

Point<float> LogicGraphPanel::toCanvas(Point<float> s) const
{
	return (s - canvasRect.getPosition().toFloat()) / zoom + viewOrigin;
}

void LogicGraphPanel::frameAll()
{
	LogicGraph* graph = getGraph();
	Rectangle<float> b = graph != nullptr ? getContentBounds(graph) : Rectangle<float>();
	if (b.isEmpty() || canvasRect.isEmpty())
	{
		zoom = 1;
		viewOrigin = { -40, -40 };
		repaint();
		return;
	}
	b = b.expanded(60);
	zoom = jlimit(.3f, 1.5f, jmin(canvasRect.getWidth() / b.getWidth(), canvasRect.getHeight() / b.getHeight()));
	viewOrigin = b.getCentre() - Point<float>((float)canvasRect.getWidth(), (float)canvasRect.getHeight()) / (2 * zoom);
	repaint();
}

Rectangle<float> LogicGraphPanel::getContentBounds(LogicGraph* graph) const
{
	Rectangle<float> b;
	auto add = [&b](Rectangle<float> r) { b = b.isEmpty() ? r : b.getUnion(r); };
	for (auto& n : graph->nodes.items) if (graph->getCollapsedGroupOf(n) == nullptr) add(getNodeRect(n));
	for (auto& gr : graph->groups.items) add(getGroupRect(graph, gr));
	return b;
}

void LogicGraphPanel::timerCallback()
{
	SpatialModule* sm = getSpatialModule();
	bool wasLive = live;
	live = sm != nullptr && sm->liveMode.isLive();
	if (live)
	{
		if (!wasLive)
		{
			dragMode = NONE;
			closeCreateMenu();
			guides.clearQuick();
			repaint();
		}
		return; //Live Mode : frozen, the graphs keep running in the module
	}
	resized(); //the tabs follow the graphs and their names
	repaint();
}

// --- geometry

Rectangle<float> LogicGraphPanel::getNodeRect(LogicNode* n) const
{
	int in = 0, out = 0;
	for (auto& p : n->pins) (p.output ? out : in)++;
	Point<float> pos = n->viewUIPosition->getPoint();
	if (n->getSymbol().isNotEmpty())	// pill : no header, the summary is inside
		return Rectangle<float>(pos.x, pos.y, (float)n->getWidth(), (float)jmax(44, jmax(in, out, 1) * rowHeight + 18));
	int rows = jmax(in, out) + (n->getSummaryText().isNotEmpty() ? 1 : 0);
	return Rectangle<float>(pos.x, pos.y, (float)n->getWidth(), (float)(headerHeight + padding * 2 + rows * rowHeight));
}

Point<float> LogicGraphPanel::getPinPosition(LogicNode* n, const String& pin, bool output) const
{
	Rectangle<float> r = getNodeRect(n);
	if (n->getSymbol().isNotEmpty())
	{
		//pill : pins of a side centred around the middle, on the round end
		int count = 0;
		for (auto& p : n->pins) if (p.output == output) count++;
		float dy = (n->getPinRow(pin, output) - (count - 1) / 2.0f) * rowHeight;
		float rad = r.getHeight() / 2;
		float inset = rad - std::sqrt(jmax(0.0f, rad * rad - dy * dy));
		return Point<float>(output ? r.getRight() - inset : r.getX() + inset, r.getCentreY() + dy);
	}
	int row = n->getPinRow(pin, output) + (n->getSummaryText().isNotEmpty() ? 1 : 0);
	return Point<float>(output ? r.getRight() : r.getX(), r.getY() + headerHeight + padding + row * rowHeight + rowHeight / 2.0f);
}

LogicGraphPanel::PinRef LogicGraphPanel::getPinAt(Point<float> c) const
{
	PinRef found;
	LogicGraph* graph = const_cast<LogicGraphPanel*>(this)->getGraph();
	if (graph == nullptr) return found;
	float radius = jmax(9.0f, 9.0f / zoom);
	for (auto& n : graph->nodes.items)
	{
		if (graph->getCollapsedGroupOf(n) != nullptr) continue;	// hidden in a collapsed group
		for (auto& p : n->pins)
			if (getPinPosition(n, p.name, p.output).getDistanceFrom(c) <= radius)
			{
				found.node = n;
				found.pin = p.name;
				found.output = p.output;
				found.kind = p.kind;
			}
	}
	return found;
}

LogicNode* LogicGraphPanel::getNodeAt(Point<float> c) const
{
	LogicGraph* graph = const_cast<LogicGraphPanel*>(this)->getGraph();
	if (graph == nullptr) return nullptr;
	for (int i = graph->nodes.items.size() - 1; i >= 0; i--)
		if (getNodeRect(graph->nodes.items[i]).contains(c) && graph->getCollapsedGroupOf(graph->nodes.items[i]) == nullptr) return graph->nodes.items[i];
	return nullptr;
}

// --- magnetic placement

Point<float> LogicGraphPanel::snapPosition(LogicNode* n, Point<float> wanted, bool free)
{
	guides.clearQuick();
	snapLabel.clear();
	Point<float> p = wanted;
	if (free) return p;
	if (snapToGrid) p = Point<float>(std::round(p.x / gridStep) * gridStep, std::round(p.y / gridStep) * gridStep);
	if (!snapToNodes) return p;

	LogicGraph* graph = getGraph();
	if (graph == nullptr) return p;

	//the closest edge or centre of another node, within 8 screen pixels, on each axis
	Rectangle<float> r = getNodeRect(n).withPosition(p);
	const float threshold = 8 / zoom;
	float bestX = threshold, bestY = threshold;
	float dx = 0, dy = 0;
	LogicNode* onX = nullptr; LogicNode* onY = nullptr;
	String labelX, labelY;
	for (auto& o : graph->nodes.items)
	{
		if (o == n || graph->getCollapsedGroupOf(o) != nullptr) continue;
		Rectangle<float> q = getNodeRect(o);
		struct Pair { float mine, theirs; const char* name; };
		Pair xs[] = { { r.getX(), q.getX(), "left" }, { r.getRight(), q.getRight(), "right" }, { r.getCentreX(), q.getCentreX(), "centre" } };
		Pair ys[] = { { r.getY(), q.getY(), "top" }, { r.getBottom(), q.getBottom(), "bottom" }, { r.getCentreY(), q.getCentreY(), "middle" } };
		for (auto& a : xs)
			if (std::abs(a.theirs - a.mine) < bestX) { bestX = std::abs(a.theirs - a.mine); dx = a.theirs - a.mine; onX = o; labelX = a.name; }
		for (auto& a : ys)
			if (std::abs(a.theirs - a.mine) < bestY) { bestY = std::abs(a.theirs - a.mine); dy = a.theirs - a.mine; onY = o; labelY = a.name; }
	}

	p += Point<float>(onX != nullptr ? dx : 0, onY != nullptr ? dy : 0);
	r = r.withPosition(p);
	StringArray parts;
	if (onX != nullptr)
	{
		Rectangle<float> q = getNodeRect(onX);
		float x = labelX == "left" ? r.getX() : labelX == "right" ? r.getRight() : r.getCentreX();
		guides.add(Line<float>(x, jmin(r.getY(), q.getY()) - 16, x, jmax(r.getBottom(), q.getBottom()) + 16));
		parts.add("aligned " + labelX + " on " + onX->niceName);
	}
	if (onY != nullptr)
	{
		Rectangle<float> q = getNodeRect(onY);
		float y = labelY == "top" ? r.getY() : labelY == "bottom" ? r.getBottom() : r.getCentreY();
		guides.add(Line<float>(jmin(r.getX(), q.getX()) - 16, y, jmax(r.getRight(), q.getRight()) + 16, y));
		parts.add(labelY + " on " + onY->niceName);
	}
	snapLabel = parts.joinIntoString(", ");
	if (snapLabel.isNotEmpty()) snapLabel = snapLabel.substring(0, 1).toUpperCase() + snapLabel.substring(1);
	return p;
}

// --- groups

Rectangle<float> LogicGraphPanel::getGroupRect(LogicGraph* graph, LogicGroup* gr) const
{
	Point<float> pos = gr->viewUIPosition->getPoint();
	if (!gr->collapsed->boolValue()) return Rectangle<float>(pos.x, pos.y, gr->viewUISize->getPoint().x, gr->viewUISize->getPoint().y);
	int in = 0, out = 0;
	for (auto& p : getGroupPins(graph, gr)) (p.output ? out : in)++;
	return Rectangle<float>(pos.x, pos.y, 260, (float)(groupTitleHeight + 22 + jmax(in, out) * rowHeight + 10));
}

Rectangle<float> LogicGraphPanel::getGroupColorRect(LogicGroup* gr) const
{
	Point<float> pos = gr->viewUIPosition->getPoint();
	return Rectangle<float>(pos.x + 38, pos.y + 14, 28, 16);
}

Array<LogicGraphPanel::GroupPin> LogicGraphPanel::getGroupPins(LogicGraph* graph, LogicGroup* gr) const
{
	Array<GroupPin> pins;
	if (!gr->collapsed->boolValue()) return pins;
	StringArray ids = StringArray::fromTokens(gr->members->stringValue(), ",", "");
	auto add = [&](const String& uid, const String& pin, bool output)
		{
			for (auto& p : pins) if (p.node == uid && p.pin == pin && p.output == output) return;
			LogicNode* n = graph->getNode(uid);
			if (n == nullptr) return;
			GroupPin gp;
			gp.node = uid;
			gp.pin = pin;
			gp.output = output;
			if (const LogicPinDef* d = n->getPin(pin, output)) gp.kind = d->kind;
			gp.label = pin;
			pins.add(gp);
		};
	//the links that leave the group : outputs on the right, inputs on the left
	for (auto& l : graph->links)
	{
		bool fromIn = ids.contains(l.fromNode), toIn = ids.contains(l.toNode);
		if (fromIn && !toIn) add(l.fromNode, l.fromPin, true);
		else if (toIn && !fromIn) add(l.toNode, l.toPin, false);
	}
	//two pins of the same name on one side : named after their node
	for (auto& p : pins)
	{
		int same = 0;
		for (auto& o : pins) if (o.output == p.output && o.pin == p.pin) same++;
		if (same > 1)
			if (LogicNode* n = graph->getNode(p.node)) p.label = n->niceName + " : " + p.pin;
	}
	return pins;
}

Point<float> LogicGraphPanel::getGroupPinPosition(LogicGroup* gr, int row, bool output) const
{
	Point<float> pos = gr->viewUIPosition->getPoint();
	return Point<float>(output ? pos.x + 260 : pos.x, pos.y + groupTitleHeight + 22 + row * rowHeight + rowHeight / 2.0f);
}

bool LogicGraphPanel::getLinkEnd(LogicGraph* graph, const String& uid, const String& pin, bool output, Point<float>& pos) const
{
	LogicNode* n = graph->getNode(uid);
	if (n == nullptr) return false;
	LogicGroup* gr = graph->getCollapsedGroupOf(n);
	if (gr == nullptr)
	{
		pos = getPinPosition(n, pin, output);
		return true;
	}
	int row = 0;
	for (auto& p : getGroupPins(graph, gr))
	{
		if (p.output != output) continue;
		if (p.node == uid && p.pin == pin)
		{
			pos = getGroupPinPosition(gr, row, output);
			return true;
		}
		row++;
	}
	return false;	// a link inside the collapsed group
}

LogicGroup* LogicGraphPanel::getGroupAt(Point<float> c, GroupPart& part) const
{
	part = GROUP_NONE;
	LogicGraph* graph = const_cast<LogicGraphPanel*>(this)->getGraph();
	if (graph == nullptr) return nullptr;
	for (int i = graph->groups.items.size() - 1; i >= 0; i--)
	{
		LogicGroup* gr = graph->groups.items[i];
		Rectangle<float> r = getGroupRect(graph, gr);
		if (!r.contains(c)) continue;
		Rectangle<float> title = r.withHeight((float)groupTitleHeight);
		if (getGroupColorRect(gr).expanded(3).contains(c)) part = GROUP_COLOR;
		else if (title.withWidth(34).contains(c)) part = GROUP_ARROW;
		else if (gr->collapsed->boolValue() || title.contains(c)) part = GROUP_TITLE;
		else if (Rectangle<float>(r.getRight() - 22, r.getBottom() - 22, 22, 22).contains(c)) part = GROUP_RESIZE;
		else continue;	// the body of an expanded group works like the void (selection rectangle)
		return gr;
	}
	return nullptr;
}

// --- painting

void LogicGraphPanel::paint(Graphics& g)
{
	using namespace LogicGraphStyle;
	g.fillAll(canvasBg);

	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr)
	{
		g.setColour(dim);
		g.setFont(Font(14.0f));
		g.drawFittedText("No Cockpit module. Add one from Modules > DrawZone > Cockpit.", getLocalBounds().reduced(8), Justification::centred, 3);
		return;
	}

	{
		Graphics::ScopedSaveState state(g);
		g.reduceClipRegion(headerVisibleRect);
		paintHeader(g);
	}
	paintHeaderEdges(g);
	{
		Graphics::ScopedSaveState state(g);
		g.reduceClipRegion(canvasRect);
		paintGrid(g);
		if (LogicGraph* graph = getGraph())
		{
			for (auto& gr : graph->groups.items) paintGroup(g, graph, gr);
			paintLinks(g, graph);
			for (auto& n : graph->nodes.items) paintNode(g, graph, n);
			paintGuides(g);
			paintSelectRect(g);
			paintMiniMap(g, graph);
			if (graph->nodes.items.isEmpty())
			{
				g.setColour(dim);
				g.setFont(Font(13.0f));
				g.drawText("Right click to add a node", canvasRect, Justification::centred);
			}
		}
		else
		{
			g.setColour(dim);
			g.setFont(Font(13.0f));
			g.drawText("No graph : click + to add one", canvasRect, Justification::centred);
		}
	}
	paintLive(g);
}

void LogicGraphPanel::paintHeader(Graphics& g)
{
	using namespace LogicGraphStyle;
	g.setColour(headerBg);
	g.fillRoundedRectangle(headerRect.toFloat(), 10);
	g.setColour(nodeLine);
	g.drawRoundedRectangle(headerRect.toFloat().reduced(.5f), 10, 1);

	g.setColour(text);
	g.setFont(Font(14.0f, Font::bold));
	g.drawText("Logic Graph", headerRect.withTrimmedLeft(12).withWidth(96), Justification::centredLeft);

	SpatialModule* sm = getSpatialModule();
	for (int i = 0; i < tabRects.size() && sm != nullptr && i < sm->logicManager.items.size(); i++)
	{
		Rectangle<float> t = tabRects[i].toFloat();
		bool selected = i == graphIndex;
		g.setColour(selected ? Colour(0xff23324a) : nodeBg);
		g.fillRoundedRectangle(t, t.getHeight() / 2);
		g.setColour(selected ? Colour(0xff3a6db3) : nodeBorder);
		g.drawRoundedRectangle(t.reduced(.5f), t.getHeight() / 2, 1);
		g.setColour(selected ? Colours::white : dim);
		g.setFont(Font(12.0f));
		g.drawText(sm->logicManager.items[i]->niceName, t.reduced(10, 0), Justification::centred, true);
	}
	if (!addTabRect.isEmpty())
	{
		Rectangle<float> t = addTabRect.toFloat();
		g.setColour(nodeBorder);
		g.drawRoundedRectangle(t.reduced(.5f), t.getHeight() / 2, 1);
		g.setColour(dim);
		g.setFont(Font(15.0f));
		g.drawText("+", t, Justification::centred);
	}

	drawToggleChip(g, snapNodesRect.toFloat(), snapToNodes, "Snap to nodes");
	drawToggleChip(g, gridRect.toFloat(), snapToGrid, "Grid");

	Rectangle<float> z = zoomRect.toFloat();
	g.setColour(nodeBg);
	g.fillRoundedRectangle(z, z.getHeight() / 2);
	g.setColour(nodeBorder);
	g.drawRoundedRectangle(z.reduced(.5f), z.getHeight() / 2, 1);
	g.setColour(dim);
	g.setFont(Font(12.0f));
	g.drawText(String(roundToInt(zoom * 100)) + " %", z, Justification::centred);

	Rectangle<float> l = liveRect.toFloat();
	if (live)
	{
		g.setColour(LogicGraphStyle::live);
		g.fillRoundedRectangle(l, l.getHeight() / 2);
	}
	else
	{
		g.setColour(Colour(0xff6b717c));
		g.drawRoundedRectangle(l.reduced(.5f), l.getHeight() / 2, 1.2f);
	}
	g.setColour(live ? Colours::white : dim);
	g.fillEllipse(Rectangle<float>(7, 7).withCentre(Point<float>(l.getX() + 13, l.getCentreY())));
	g.setFont(Font(12.0f, Font::bold));
	g.drawText("LIVE", l.withTrimmedLeft(22), Justification::centredLeft);
}

void LogicGraphPanel::paintGrid(Graphics& g)
{
	using namespace LogicGraphStyle;
	//dots every 20 canvas px, sparser when zoomed out so they stay readable
	float step = gridStep;
	while (step * zoom < 12) step *= 2;
	Point<float> a = toCanvas(canvasRect.getTopLeft().toFloat());
	Point<float> b = toCanvas(canvasRect.getBottomRight().toFloat());
	g.setColour(snapToGrid ? dots.brighter(.35f) : dots);
	float r = jlimit(.8f, 1.6f, 1.3f * zoom);
	for (float x = std::floor(a.x / step) * step; x <= b.x; x += step)
		for (float y = std::floor(a.y / step) * step; y <= b.y; y += step)
		{
			Point<float> s = toScreen(Point<float>(x, y));
			g.fillEllipse(s.x - r, s.y - r, r * 2, r * 2);
		}
}

Path LogicGraphPanel::getWirePath(Point<float> from, Point<float> to) const
{
	float d = jmax(40.0f * zoom, std::abs(to.x - from.x) * .5f);
	Path p;
	p.startNewSubPath(from);
	p.cubicTo(from.translated(d, 0), to.translated(-d, 0), to);
	return p;
}

void LogicGraphPanel::paintLinks(Graphics& g, LogicGraph* graph)
{
	for (auto& l : graph->links)
	{
		LogicNode* from = graph->getNode(l.fromNode);
		if (from == nullptr) continue;
		const LogicPinDef* pin = from->getPin(l.fromPin, true);
		if (pin == nullptr) continue;
		//a node hidden in a collapsed group : the link leaves from the edge of the group
		Point<float> a, b;
		if (!getLinkEnd(graph, l.fromNode, l.fromPin, true, a) || !getLinkEnd(graph, l.toNode, l.toPin, false, b)) continue;
		g.setColour(LogicPin::getColour(pin->kind));
		g.strokePath(getWirePath(toScreen(a), toScreen(b)), PathStrokeType(jmax(1.2f, 2 * zoom)));
	}

	//the link being pulled
	if (dragMode == WIRE && wireFrom.isValid())
		if (LogicNode* n = dynamic_cast<LogicNode*>(wireFrom.node.get()))
		{
			Point<float> pin = toScreen(getPinPosition(n, wireFrom.pin, wireFrom.output));
			Point<float> mouse = toScreen(wireMouse);
			g.setColour(LogicPin::getColour(wireFrom.kind));
			Path p = wireFrom.output ? getWirePath(pin, mouse) : getWirePath(mouse, pin);
			Path dashed;
			const float dash[] = { 6, 5 };
			PathStrokeType(2).createDashedStroke(dashed, p, dash, 2);
			g.fillPath(dashed);
		}
}

void LogicGraphPanel::paintNode(Graphics& g, LogicGraph* graph, LogicNode* n)
{
	using namespace LogicGraphStyle;
	if (graph->getCollapsedGroupOf(n) != nullptr) return;	// hidden in a collapsed group
	Rectangle<float> rc = getNodeRect(n);
	Rectangle<float> r(toScreen(rc.getTopLeft()), toScreen(rc.getBottomRight()));
	if (!r.intersects(canvasRect.toFloat())) return;

	float alpha = n->enabled->boolValue() ? 1.0f : .4f;	// hidden (H) : dimmed
	String symbol = n->getSymbol();
	bool pill = symbol.isNotEmpty();
	float radius = pill ? r.getHeight() / 2 : 10 * zoom;
	bool selected = n->isSelected;
	bool showText = zoom >= .45f;
	LogicGroup* group = graph->getGroupOf(n);	// its color tints the node

	if (selected)
	{
		g.setColour(accent.withAlpha(.15f));
		g.fillRoundedRectangle(r.expanded(4 * zoom), radius + 4 * zoom);
	}
	g.setColour(nodeBg.withMultipliedAlpha(alpha));
	g.fillRoundedRectangle(r, radius);
	if (group != nullptr)
	{
		Colour tint = group->color->getColor();
		if (pill)
		{
			g.setColour(tint.withAlpha(.16f * alpha));
			g.fillRoundedRectangle(r, radius);
		}
		else
		{
			Path head;
			head.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(), headerHeight * zoom, radius, radius, true, true, false, false);
			g.setColour(tint.withAlpha(.32f * alpha));
			g.fillPath(head);
		}
	}
	Colour border = pill ? LogicNode::getCategoryColour(n->getCategory()).withMultipliedAlpha(alpha) : group != nullptr ? group->color->getColor().withAlpha(.7f * alpha) : nodeBorder;
	g.setColour(selected ? accent : border);
	g.drawRoundedRectangle(r.reduced(.5f), radius, pill || selected ? 1.5f : 1);

	if (pill)
	{
		paintPillNode(g, graph, n, r, symbol, alpha, showText);
		return;
	}

	float hh = headerHeight * zoom;
	Rectangle<float> header = r.withHeight(hh);
	g.setColour(nodeLine);
	g.drawHorizontalLine((int)header.getBottom(), r.getX() + 1, r.getRight() - 1);

	LogicGraphStyle::drawCategoryIcon(g, n->getCategory(), Rectangle<float>(20 * zoom, 20 * zoom).withCentre(Point<float>(r.getX() + 22 * zoom, header.getCentreY())));
	if (showText)
	{
		Colour cat = LogicNode::getCategoryColour(n->getCategory());
		g.setFont(Font(10 * zoom, Font::bold));
		String catName = n->enabled->boolValue() ? LogicNode::getCategoryName(n->getCategory()).toUpperCase() : String("HIDDEN");
		float catW = Font(10 * zoom, Font::bold).getStringWidthFloat(catName) + 12 * zoom;
		g.setColour(cat.withMultipliedAlpha(alpha));
		g.drawText(catName, header.withTrimmedRight(12 * zoom).withLeft(header.getRight() - catW - 12 * zoom), Justification::centredRight);
		g.setColour(text.withMultipliedAlpha(alpha));
		g.setFont(Font(13 * zoom, Font::bold));
		g.drawText(n->niceName, header.withTrimmedLeft(38 * zoom).withTrimmedRight(catW + 14 * zoom), Justification::centredLeft, true);
	}

	//summary (zone, message...) as a chip on its own row
	float y = header.getBottom() + padding * zoom;
	String summary = n->getSummaryText();
	if (summary.isNotEmpty())
	{
		if (showText)
		{
			Font f(Font::getDefaultMonospacedFontName(), 11 * zoom, Font::plain);
			float w = jmin(r.getWidth() - 28 * zoom, f.getStringWidthFloat(summary) + 18 * zoom);
			Rectangle<float> chip(r.getX() + 14 * zoom, y + 4 * zoom, w, (rowHeight - 8) * zoom);
			g.setColour(chipBg);
			g.fillRoundedRectangle(chip, chip.getHeight() / 2);
			g.setColour(text.withMultipliedAlpha(alpha));
			g.setFont(f);
			g.drawText(summary, chip.reduced(9 * zoom, 0), Justification::centredLeft, true);
		}
		y += rowHeight * zoom;
	}

	//pins : label inside, glyph on the edge (exec = diamond, data = ring, filled when linked)
	for (auto& p : n->pins)
	{
		Point<float> s = paintPin(g, graph, n, p, alpha);
		if (showText)
		{
			g.setColour(dim.withMultipliedAlpha(alpha));
			g.setFont(Font(12 * zoom));
			Rectangle<float> row(r.getX() + 14 * zoom, s.y - rowHeight * zoom / 2, r.getWidth() - 28 * zoom, rowHeight * zoom);
			g.drawText(p.name, row, p.output ? Justification::centredRight : Justification::centredLeft, true);
		}
	}
}

Point<float> LogicGraphPanel::paintPin(Graphics& g, LogicGraph* graph, LogicNode* n, const LogicPinDef& p, float alpha)
{
	using namespace LogicGraphStyle;
	Point<float> s = toScreen(getPinPosition(n, p.name, p.output));
	String uid = n->uid->stringValue();
	bool linked = false;
	for (auto& l : graph->links)
		if ((p.output && l.fromNode == uid && l.fromPin == p.name) || (!p.output && l.toNode == uid && l.toPin == p.name)) { linked = true; break; }

	bool hovered = hoveredPin.isValid() && hoveredPin.node == n && hoveredPin.pin == p.name && hoveredPin.output == p.output;
	LogicGraphStyle::drawPin(g, s, p.kind, linked, (hovered ? 14.0f : 12.0f) * zoom, alpha, zoom);
	return s;
}

//compare nodes : pill without header, symbol + setting chip ; small input names only when there are several inputs
void LogicGraphPanel::paintPillNode(Graphics& g, LogicGraph* graph, LogicNode* n, Rectangle<float> r, const String& symbol, float alpha, bool showText)
{
	using namespace LogicGraphStyle;
	int numIn = 0;
	for (auto& p : n->pins) if (!p.output) numIn++;

	float x = r.getX() + r.getHeight() * .35f;
	for (auto& p : n->pins)
	{
		Point<float> s = paintPin(g, graph, n, p, alpha);
		if (showText && !p.output && numIn > 1)
		{
			g.setColour(dim.withMultipliedAlpha(alpha));
			g.setFont(Font(10 * zoom));
			g.drawText(p.name, Rectangle<float>(s.x + 9 * zoom, s.y - 7 * zoom, 40 * zoom, 14 * zoom), Justification::centredLeft, false);
			x = jmax(x, s.x + 9 * zoom + Font(10 * zoom).getStringWidthFloat(p.name) + 6 * zoom);
		}
	}
	if (!showText) return;

	Font symFont((symbol.length() == 1 ? 26.0f : 14.0f) * zoom, Font::bold);
	float symW = symFont.getStringWidthFloat(symbol) + 8 * zoom;
	String summary = n->getSummaryText();
	float right = r.getRight() - r.getHeight() * .35f;
	if (summary.isEmpty()) x = jmax(x, r.getCentreX() - symW / 2);	// symbol alone : centred
	g.setColour(LogicNode::getCategoryColour(n->getCategory()).withMultipliedAlpha(alpha));
	g.setFont(symFont);
	g.drawText(symbol, Rectangle<float>(x, r.getY(), symW, r.getHeight()), Justification::centred, false);

	if (summary.isEmpty()) return;
	Font f(Font::getDefaultMonospacedFontName(), 11.5f * zoom, Font::plain);
	float left = x + symW + 4 * zoom;
	float w = jmin(right - left, f.getStringWidthFloat(summary) + 16 * zoom);
	if (w <= 10 * zoom) return;
	Rectangle<float> chip(left + (right - left - w) / 2, r.getCentreY() - 10 * zoom, w, 20 * zoom);
	g.setColour(chipBg);
	g.fillRoundedRectangle(chip, chip.getHeight() / 2);
	g.setColour(nodeBorder);
	g.drawRoundedRectangle(chip.reduced(.5f), chip.getHeight() / 2, 1);
	g.setColour(text.withMultipliedAlpha(alpha));
	g.setFont(f);
	g.drawText(summary, chip.reduced(8 * zoom, 0), Justification::centred, true);
}

void LogicGraphPanel::paintGroup(Graphics& g, LogicGraph* graph, LogicGroup* gr)
{
	using namespace LogicGraphStyle;
	Rectangle<float> rc = getGroupRect(graph, gr);
	Rectangle<float> r(toScreen(rc.getTopLeft()), toScreen(rc.getBottomRight()));
	if (!r.intersects(canvasRect.toFloat())) return;

	Colour col = gr->color->getColor();
	Colour light = col.interpolatedWith(Colours::white, .5f);
	bool coll = gr->collapsed->boolValue();
	float radius = 14 * zoom;
	g.setColour(col.withAlpha(coll ? .34f : .2f));
	g.fillRoundedRectangle(r, radius);
	g.setColour(gr->isSelected ? accent : col.withAlpha(coll ? .8f : .35f));
	g.drawRoundedRectangle(r.reduced(.5f), radius, gr->isSelected ? 1.5f : 1);

	//title : arrow, color capsule, name, number of nodes
	float th = groupTitleHeight * zoom;
	Point<float> a(r.getX() + 20 * zoom, r.getY() + th / 2);
	float s = 6 * zoom;
	Path arrow;
	if (coll) arrow.addTriangle(a.x - s * .6f, a.y - s, a.x - s * .6f, a.y + s, a.x + s, a.y);
	else arrow.addTriangle(a.x - s, a.y - s * .6f, a.x + s, a.y - s * .6f, a.x, a.y + s);
	g.setColour(light);
	g.fillPath(arrow);

	Rectangle<float> cr = getGroupColorRect(gr);
	Rectangle<float> capsule(toScreen(cr.getTopLeft()), toScreen(cr.getBottomRight()));
	g.setColour(col);
	g.fillRoundedRectangle(capsule, capsule.getHeight() / 2);
	g.setColour(Colours::white.withAlpha(.75f));
	g.drawRoundedRectangle(capsule.reduced(.5f), capsule.getHeight() / 2, 1.2f);

	Array<LogicNode*> inside = graph->getGroupNodes(gr);
	if (zoom >= .3f)
	{
		float textX = capsule.getRight() + 10 * zoom;
		g.setColour(light.withAlpha(.92f));
		g.setFont(Font((coll ? 16.0f : 22.0f) * zoom, Font::bold));
		g.drawText(gr->niceName, Rectangle<float>(textX, r.getY(), jmax(0.0f, r.getRight() - textX - 76 * zoom), th), Justification::centredLeft, true);
		g.setColour(light.withAlpha(.6f));
		g.setFont(Font(11 * zoom));
		g.drawText(String(inside.size()) + (inside.size() == 1 ? " node" : " nodes"), Rectangle<float>(r.getRight() - 80 * zoom, r.getY(), 66 * zoom, th), Justification::centredRight, true);
	}

	if (!coll)
	{
		//resize corner
		g.setColour(light.withAlpha(.45f));
		for (int i = 1; i <= 3; i++)
		{
			float d = i * 4 * zoom, e = 5 * zoom;
			g.drawLine(r.getRight() - e - d, r.getBottom() - e, r.getRight() - e, r.getBottom() - e - d, 1.2f);
		}
		return;
	}

	//collapsed : a dot per hidden node (its category), then the pins of the links leaving the group
	for (int i = 0; i < jmin(16, inside.size()); i++)
	{
		g.setColour(LogicNode::getCategoryColour(inside[i]->getCategory()));
		g.fillEllipse(r.getX() + (16 + i * 14) * zoom, r.getY() + th + 4 * zoom, 9 * zoom, 9 * zoom);
	}
	int rows[2] = { 0, 0 };
	for (auto& p : getGroupPins(graph, gr))
	{
		Point<float> sp = toScreen(getGroupPinPosition(gr, rows[p.output ? 1 : 0]++, p.output));
		LogicGraphStyle::drawPin(g, sp, p.kind, true, 12 * zoom, 1, zoom);
		if (zoom < .45f) continue;
		g.setColour(text.withAlpha(.85f));
		g.setFont(Font(12 * zoom));
		Rectangle<float> row(r.getX() + 14 * zoom, sp.y - rowHeight * zoom / 2, r.getWidth() - 28 * zoom, rowHeight * zoom);
		g.drawText(p.label, row, p.output ? Justification::centredRight : Justification::centredLeft, true);
	}
}

void LogicGraphPanel::paintGuides(Graphics& g)
{
	using namespace LogicGraphStyle;
	if (dragMode != MOVE_NODE || guides.isEmpty()) return;
	g.setColour(accent);
	for (auto& l : guides) g.drawLine(Line<float>(toScreen(l.getStart()), toScreen(l.getEnd())), 1.5f);

	LogicNode* n = dynamic_cast<LogicNode*>(dragNode.get());
	if (n == nullptr || snapLabel.isEmpty()) return;
	Rectangle<float> rc = getNodeRect(n);
	Point<float> s = toScreen(rc.getBottomLeft()).translated(0, 10);
	Font f(11.0f, Font::bold);
	Rectangle<float> pill(s.x, s.y, f.getStringWidthFloat(snapLabel) + 20, 20);
	g.setColour(accent);
	g.fillRoundedRectangle(pill, 10);
	g.setColour(canvasBg);
	g.setFont(f);
	g.drawText(snapLabel, pill, Justification::centred);
}

void LogicGraphPanel::paintMiniMap(Graphics& g, LogicGraph* graph)
{
	using namespace LogicGraphStyle;
	if (graph->nodes.items.isEmpty() || canvasRect.getWidth() < 420 || canvasRect.getHeight() < 260) return;

	Rectangle<float> view(toCanvas(canvasRect.getTopLeft().toFloat()), toCanvas(canvasRect.getBottomRight().toFloat()));
	Rectangle<float> world = view;
	Rectangle<float> content = getContentBounds(graph);
	if (!content.isEmpty()) world = world.getUnion(content);
	world = world.expanded(40);

	Rectangle<float> box = miniMapRect.toFloat();
	g.setColour(headerBg.withAlpha(.95f));
	g.fillRoundedRectangle(box, 10);
	g.setColour(nodeLine);
	g.drawRoundedRectangle(box.reduced(.5f), 10, 1);
	Rectangle<float> inner = box.reduced(10, 10).withTrimmedTop(12);
	g.setColour(Colour(0xff6b717c));
	g.setFont(Font(9.0f, Font::bold));
	g.drawText("OVERVIEW", box.reduced(10, 6).withHeight(12), Justification::centredLeft);

	float s = jmin(inner.getWidth() / world.getWidth(), inner.getHeight() / world.getHeight());
	Point<float> o = inner.getCentre() - world.getCentre() * s;
	auto map = [s, o](Rectangle<float> r) { return Rectangle<float>(r.getX() * s + o.x, r.getY() * s + o.y, r.getWidth() * s, r.getHeight() * s); };
	for (auto& gr : graph->groups.items)
	{
		g.setColour(gr->color->getColor().withAlpha(gr->collapsed->boolValue() ? .7f : .3f));
		g.fillRoundedRectangle(map(getGroupRect(graph, gr)), 3);
	}
	for (auto& n : graph->nodes.items)
	{
		if (graph->getCollapsedGroupOf(n) != nullptr) continue;
		g.setColour(LogicNode::getCategoryColour(n->getCategory()).withAlpha(.85f));
		g.fillRoundedRectangle(map(getNodeRect(n)), 2);
	}
	g.setColour(accent);
	g.drawRoundedRectangle(map(view), 3, 1);
}

void LogicGraphPanel::paintLive(Graphics& g)
{
	using namespace LogicGraphStyle;
	if (!live) return;
	g.setColour(Colours::black.withAlpha(.6f));
	g.fillRect(canvasRect);
	Rectangle<float> banner = Rectangle<float>(420, 74).withCentre(canvasRect.toFloat().getCentre());
	g.setColour(LogicGraphStyle::live);
	g.fillRoundedRectangle(banner, 8);
	g.setColour(Colours::white);
	g.setFont(Font(18.0f, Font::bold));
	g.drawText("LIVE", banner.withHeight(36).translated(0, 6), Justification::centred);
	g.setFont(Font(12.0f));
	g.drawText("Graphs keep running. Editing locked : LIVE or Ctrl + Shift + L to leave.", banner.withTrimmedTop(40).reduced(10, 0), Justification::centredTop);
}

// --- menus and actions

namespace
{
	//two columns : sections and types on the left (hover = show its nodes), the nodes of the type on the right
	class LogicCreateMenu :
		public Component
	{
	public:
		struct Entry { String label, type, help, key; bool enabled = true; };
		struct Column { String section, name; Colour colour; bool diamond = false, frame = false; Array<Entry> entries; };

		Array<Column> columns;
		int current = 0;
		std::function<void(const String&)> onChoose;
		std::function<void()> onDismiss;

		static constexpr int margin = 14, leftWidth = 196, rightWidth = 340, rowHeight = 28, sectionHeight = 26, itemHeight = 46, pad = 6;

		void build()
		{
			//left rows : a section header when the section changes, then one row per column
			leftRows.clearQuick();
			headers.clearQuick();
			int y = margin + pad;
			String section;
			for (auto& c : columns)
			{
				if (c.section != section)
				{
					section = c.section;
					headers.add({ section, Rectangle<int>(margin + pad, y, leftWidth - pad * 2, sectionHeight) });
					y += sectionHeight;
				}
				leftRows.add(Rectangle<int>(margin + pad, y, leftWidth - pad * 2, rowHeight));
				y += rowHeight;
			}
			int maxEntries = 0;
			for (auto& c : columns) maxEntries = jmax(maxEntries, c.entries.size());
			int h = jmax(y + pad, margin + pad + sectionHeight + maxEntries * itemHeight + pad) + margin;
			setSize(margin * 2 + leftWidth + rightWidth, h);
			if (!columns.isEmpty() && columns[current].entries.isEmpty()) current = 0;
		}

		void paint(Graphics& g) override
		{
			using namespace LogicGraphStyle;
			Rectangle<float> box = getLocalBounds().reduced(margin).toFloat();
			DropShadow(Colours::black.withAlpha(.65f), 22, { 0, 8 }).drawForRectangle(g, box.toNearestInt());
			g.setColour(nodeBg);
			g.fillRoundedRectangle(box, 10);
			g.setColour(nodeBorder);
			g.drawRoundedRectangle(box.reduced(.5f), 10, 1);
			float split = box.getX() + leftWidth;
			g.setColour(nodeLine);
			g.drawVerticalLine((int)split, box.getY() + 8, box.getBottom() - 8);

			for (auto& hd : headers) paintHeader(g, hd.name, hd.rect.toFloat());
			for (int i = 0; i < columns.size(); i++)
			{
				const Column& c = columns.getReference(i);
				Rectangle<float> r = leftRows[i].toFloat();
				if (i == current || i == hoveredLeft)
				{
					g.setColour(Colour(i == current ? 0xff23272e : 0xff1d2026));
					g.fillRoundedRectangle(r, 6);
				}
				paintGlyph(g, c, Point<float>(r.getX() + 16, r.getCentreY()), 1);
				g.setColour(i == current ? text : text.withAlpha(.85f));
				g.setFont(Font(13.0f));
				g.drawText(c.name, r.withTrimmedLeft(30).withTrimmedRight(22), Justification::centredLeft, true);
				g.setColour(dim);
				g.drawText(">", r.withTrimmedRight(10), Justification::centredRight);
			}

			if (!isPositiveAndBelow(current, columns.size())) return;
			const Column& c = columns.getReference(current);
			paintHeader(g, c.name, Rectangle<float>(split + pad, box.getY() + pad, rightWidth - pad * 2, (float)sectionHeight));
			for (int i = 0; i < c.entries.size(); i++)
			{
				const Entry& e = c.entries.getReference(i);
				Rectangle<float> r = getEntryRect(i).toFloat();
				float a = e.enabled ? 1.0f : .4f;
				if (i == hoveredRight && e.enabled)
				{
					g.setColour(Colour(0xff23272e));
					g.fillRoundedRectangle(r, 6);
				}
				paintGlyph(g, c, Point<float>(r.getX() + 16, r.getY() + 15), a);
				g.setColour(text.withMultipliedAlpha(a));
				g.setFont(Font(13.0f, Font::bold));
				g.drawText(e.label, Rectangle<float>(r.getX() + 30, r.getY() + 4, r.getWidth() - 74, 20), Justification::centredLeft, true);
				g.setColour(dim.withMultipliedAlpha(a));
				g.setFont(Font(11.5f));
				g.drawText(e.help, Rectangle<float>(r.getX() + 30, r.getY() + 23, r.getWidth() - 40, 17), Justification::centredLeft, true);
				if (e.key.isNotEmpty())
				{
					Rectangle<float> chip(r.getRight() - 34, r.getY() + 6, 24, 18);
					g.setColour(nodeBorder.withMultipliedAlpha(a));
					g.drawRoundedRectangle(chip, 4, 1);
					g.setColour(dim.withMultipliedAlpha(a));
					g.setFont(Font(Font::getDefaultMonospacedFontName(), 11.0f, Font::plain));
					g.drawText(e.key, chip, Justification::centred);
				}
			}
		}

		void mouseMove(const MouseEvent& e) override
		{
			int l = -1, r = -1;
			for (int i = 0; i < leftRows.size(); i++) if (leftRows[i].contains(e.getPosition())) l = i;
			if (isPositiveAndBelow(current, columns.size()))
				for (int i = 0; i < columns[current].entries.size(); i++) if (getEntryRect(i).contains(e.getPosition())) r = i;
			if (l >= 0 && l != current && !columns[l].entries.isEmpty()) { current = l; r = -1; }
			if (l != hoveredLeft || r != hoveredRight) { hoveredLeft = l; hoveredRight = r; repaint(); }
		}

		void mouseExit(const MouseEvent&) override { hoveredLeft = hoveredRight = -1; repaint(); }
		void mouseDown(const MouseEvent& e) override { mouseMove(e); }

		void mouseUp(const MouseEvent& e) override
		{
			if (!isPositiveAndBelow(current, columns.size())) return;
			for (int i = 0; i < columns[current].entries.size(); i++)
			{
				const Entry& en = columns[current].entries.getReference(i);
				if (getEntryRect(i).contains(e.getPosition()) && en.enabled && onChoose) { onChoose(en.type); return; }
			}
		}

		bool keyPressed(const KeyPress& k) override
		{
			if (k == KeyPress::escapeKey && onDismiss) { onDismiss(); return true; }
			return false;
		}

	private:
		struct Header { String name; Rectangle<int> rect; };
		Array<Rectangle<int>> leftRows;
		Array<Header> headers;
		int hoveredLeft = -1, hoveredRight = -1;

		Rectangle<int> getEntryRect(int i) const
		{
			return Rectangle<int>(margin + leftWidth + pad, margin + pad + sectionHeight + i * itemHeight, rightWidth - pad * 2, itemHeight - 2);
		}

		static void paintHeader(Graphics& g, const String& name, Rectangle<float> r)
		{
			using namespace LogicGraphStyle;
			Font f(10.0f, Font::bold);
			String t = name.toUpperCase();
			g.setColour(dim);
			g.setFont(f);
			g.drawText(t, r.withTrimmedLeft(10), Justification::centredLeft, false);
			float x = r.getX() + 10 + f.getStringWidthFloat(t) + 8;
			g.setColour(nodeLine);
			g.drawHorizontalLine((int)r.getCentreY(), x, r.getRight() - 6);
		}

		static void paintGlyph(Graphics& g, const Column& c, Point<float> p, float alpha)
		{
			if (c.frame)
			{
				g.setColour(LogicGraphStyle::dim.withMultipliedAlpha(alpha));
				g.drawRoundedRectangle(Rectangle<float>(13, 10).withCentre(p), 3, 1.5f);
				return;
			}
			g.setColour(c.colour.withMultipliedAlpha(alpha));
			if (!c.diamond) { g.fillEllipse(Rectangle<float>(10, 10).withCentre(p)); return; }
			Path d;
			d.addRectangle(-4.0f, -4.0f, 8.0f, 8.0f);
			d.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 4).translated(p));
			g.fillPath(d);
		}
	};
}

void LogicGraphPanel::showCanvasMenu(Point<float> canvas)
{
	LogicGraph* graph = getGraph();
	if (graph == nullptr) return;
	closeCreateMenu();
	Point<float> pos = snapToGrid ? Point<float>(std::round(canvas.x / gridStep) * gridStep, std::round(canvas.y / gridStep) * gridStep) : canvas;

	auto menu = std::make_unique<LogicCreateMenu>();
	using Column = LogicCreateMenu::Column;
	using Entry = LogicCreateMenu::Entry;

	Column group;
	group.section = "Grouping";
	group.name = "Group";
	group.frame = true;
	Entry sel { "Group Selection", "#group", "Frame around the selected nodes", "G" };
	sel.enabled = !getSelectedNodes().isEmpty();
	if (!sel.enabled) sel.help = "Select nodes first (left drag in the void)";
	group.entries.add(sel);
	group.entries.add({ "Empty Group", "#emptyGroup", "A frame to fill later", "" });
	Entry ungroup { "Ungroup", "#ungroup", "Removes the selected groups, keeps their nodes", "Shift+G" };
	ungroup.enabled = !getSelectedGroups().isEmpty();
	if (!ungroup.enabled) ungroup.help = "Select a group first (click its title)";
	group.entries.add(ungroup);
	menu->columns.add(group);

	//the node types in the order of the factory, sorted into Input Data, Logic, Output Data
	auto sectionOf = [](const String& path)
		{
			if (path == "Compare" || path == "Flow") return String("Logic");
			if (path == "Outputs") return String("Output Data");
			return String("Input Data");
		};
	auto colourOf = [](const String& path)
		{
			if (path == "Trigger") return LogicPin::getColour(LogicPin::EXEC);
			if (path == "On-Off") return LogicPin::getColour(LogicPin::BOOL);
			if (path == "Count") return LogicPin::getColour(LogicPin::NUMBER);
			if (path == "Time") return LogicPin::getColour(LogicPin::TIME);
			if (path == "Percentage") return LogicPin::getColour(LogicPin::PERCENT);
			if (path == "Meters") return LogicPin::getColour(LogicPin::METERS);
			if (path == "Compare") return LogicNode::getCategoryColour(LogicNode::COMPARE);
			if (path == "Outputs") return LogicNode::getCategoryColour(LogicNode::ACTION);
			return LogicNode::getCategoryColour(LogicNode::FLOW);
		};
	const StringArray sections { "Input Data", "Logic", "Output Data" };
	for (auto& sec : sections)
		for (auto& d : graph->nodes.factory.defs)
		{
			if (sectionOf(d->menuPath) != sec) continue;
			Column* col = nullptr;
			for (auto& c : menu->columns) if (c.name == d->menuPath) col = &c;
			if (col == nullptr)
			{
				Column c;
				c.section = sec;
				c.name = d->menuPath;
				c.colour = colourOf(d->menuPath);
				c.diamond = d->menuPath == "Trigger";
				menu->columns.add(c);
				col = &menu->columns.getReference(menu->columns.size() - 1);
			}
			col->entries.add({ d->type, d->type, LogicNodeManager::getDescription(d->type), "" });
		}

	menu->current = 1;	// Trigger : the first nodes of a graph
	menu->build();
	Component::SafePointer<LogicGraphPanel> safe(this);
	menu->onChoose = [safe, pos](const String& type)
		{
			//after the click : the menu is deleted outside its own callback
			MessageManager::callAsync([safe, pos, type] { if (safe != nullptr) { safe->closeCreateMenu(); safe->createFromMenu(type, pos); } });
		};
	menu->onDismiss = [safe] { MessageManager::callAsync([safe] { if (safe != nullptr) safe->closeCreateMenu(); }); };

	//top left at the click, kept inside the panel
	Point<int> at = toScreen(canvas).toInt() - Point<int>(LogicCreateMenu::margin, LogicCreateMenu::margin);
	Rectangle<int> b = menu->getBounds().withPosition(at).constrainedWithin(getLocalBounds());
	menu->setBounds(b);
	addAndMakeVisible(menu.get());
	menu->setWantsKeyboardFocus(true);
	menu->grabKeyboardFocus();
	createMenu = std::move(menu);
}

void LogicGraphPanel::closeCreateMenu()
{
	if (createMenu == nullptr) return;
	removeChildComponent(createMenu.get());
	createMenu.reset();
	grabKeyboardFocus();
	repaint();
}

void LogicGraphPanel::createFromMenu(const String& type, Point<float> canvas)
{
	LogicGraph* graph = getGraph();
	if (graph == nullptr) return;
	if (type == "#group") { groupSelectedNodes(); return; }
	if (type == "#ungroup") { ungroupSelected(); return; }
	if (type == "#emptyGroup")
	{
		LogicGroup* gr = new LogicGroup();
		gr->viewUIPosition->setPoint(canvas);
		graph->groups.addItem(gr);
		return;
	}
	for (auto& d : graph->nodes.factory.defs)
		if (d->type == type)
		{
			if (LogicNode* n = dynamic_cast<LogicNode*>(d->create())) graph->nodes.addItem(n, canvas);
			return;
		}
}

void LogicGraphPanel::showTabMenu(int index)
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr || index < 0 || index >= sm->logicManager.items.size()) return;
	LogicGraph* graph = sm->logicManager.items[index];
	PopupMenu m;
	m.addItem(1, "Rename...");
	m.addItem(2, "Delete graph");
	WeakReference<ControllableContainer> ref(graph);
	WeakReference<Component> panelRef(this);
	m.showMenuAsync(PopupMenu::Options(), [ref, panelRef](int result)
		{
			LogicGraphPanel* panel = dynamic_cast<LogicGraphPanel*>(panelRef.get());
			LogicGraph* gr = dynamic_cast<LogicGraph*>(ref.get());
			if (gr == nullptr || panel == nullptr) return;
			if (result == 1) panel->renameGraph(gr);
			else if (result == 2)
				if (SpatialModule* sm = panel->getSpatialModule()) sm->logicManager.removeItem(gr);
		});
}

void LogicGraphPanel::renameGraph(LogicGraph* graph)
{
	AlertWindow* w = new AlertWindow("Rename graph", String(), MessageBoxIconType::NoIcon, this);
	w->addTextEditor("name", graph->niceName);
	w->addButton("OK", 1, KeyPress(KeyPress::returnKey));
	w->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));
	WeakReference<ControllableContainer> ref(graph);
	w->enterModalState(true, ModalCallbackFunction::create([w, ref](int result)
		{
			LogicGraph* gr = dynamic_cast<LogicGraph*>(ref.get());
			String name = w->getTextEditorContents("name").trim();
			if (gr != nullptr && result == 1 && name.isNotEmpty()) gr->setNiceName(name);
		}), true);
}

void LogicGraphPanel::removeNode(LogicNode* n)
{
	LogicGraph* graph = getGraph();
	if (graph == nullptr || n == nullptr) return;
	Array<UndoableAction*> actions = graph->getDisconnectNodeActions(n);
	actions.addArray(graph->nodes.getRemoveItemUndoableAction(n));
	UndoMaster::getInstance()->performActions("Remove " + n->niceName, actions);
}

Array<LogicNode*> LogicGraphPanel::getSelectedNodes()
{
	Array<LogicNode*> result;
	if (LogicGraph* graph = getGraph())
		for (auto& n : graph->nodes.items) if (n->isSelected) result.add(n);
	return result;
}

void LogicGraphPanel::removeSelectedNodes()
{
	LogicGraph* graph = getGraph();
	Array<LogicNode*> sel = getSelectedNodes();
	Array<LogicGroup*> groupSel = getSelectedGroups();	// a removed group leaves its nodes
	if (graph == nullptr || (sel.isEmpty() && groupSel.isEmpty())) return;
	if (sel.size() == 1 && groupSel.isEmpty()) { removeNode(sel[0]); return; }
	Array<UndoableAction*> actions;
	for (auto& n : sel) actions.addArray(graph->getDisconnectNodeActions(n));
	for (auto& n : sel) actions.addArray(graph->nodes.getRemoveItemUndoableAction(n));
	for (auto& gr : groupSel) actions.addArray(graph->groups.getRemoveItemUndoableAction(gr));
	StringArray what;
	if (!sel.isEmpty()) what.add(String(sel.size()) + (sel.size() == 1 ? " node" : " nodes"));
	if (!groupSel.isEmpty()) what.add(String(groupSel.size()) + (groupSel.size() == 1 ? " group" : " groups"));
	UndoMaster::getInstance()->performActions("Remove " + what.joinIntoString(" and "), actions);
}

void LogicGraphPanel::toggleHideSelected()
{
	Array<LogicNode*> sel = getSelectedNodes();
	if (sel.isEmpty()) return;
	bool anyShown = false;
	for (auto& n : sel) anyShown |= n->enabled->boolValue();
	Array<UndoableAction*> actions;
	for (auto& n : sel)
		if (n->enabled->boolValue() != !anyShown)
			if (UndoableAction* a = n->enabled->setUndoableValue(n->enabled->boolValue(), !anyShown, true)) actions.add(a);
	if (!actions.isEmpty()) UndoMaster::getInstance()->performActions(String(anyShown ? "Hide " : "Show ") + String(actions.size()) + " nodes", actions);
	repaint();
}

Array<LogicGroup*> LogicGraphPanel::getSelectedGroups()
{
	Array<LogicGroup*> result;
	if (LogicGraph* graph = getGraph())
		for (auto& gr : graph->groups.items) if (gr->isSelected) result.add(gr);
	return result;
}

void LogicGraphPanel::groupSelectedNodes()
{
	LogicGraph* graph = getGraph();
	Array<LogicNode*> sel = getSelectedNodes();
	if (graph == nullptr || sel.isEmpty()) return;
	Rectangle<float> b = getNodeRect(sel[0]);
	for (auto& n : sel) b = b.getUnion(getNodeRect(n));
	b = b.expanded(24).withTrimmedTop(-(float)groupTitleHeight);	// room for the title above the nodes
	LogicGroup* gr = new LogicGroup();
	gr->viewUIPosition->setPoint(b.getPosition());
	gr->viewUISize->setPoint(b.getWidth(), b.getHeight());
	graph->groups.addItem(gr);
	if (InspectableSelectionManager::mainSelectionManager != nullptr) InspectableSelectionManager::mainSelectionManager->clearSelection();
	gr->selectThis();
	repaint();
}

void LogicGraphPanel::ungroupSelected()
{
	LogicGraph* graph = getGraph();
	Array<LogicGroup*> sel = getSelectedGroups();
	if (graph == nullptr || sel.isEmpty()) return;
	Array<UndoableAction*> actions;
	for (auto& gr : sel) actions.addArray(graph->groups.getRemoveItemUndoableAction(gr));
	UndoMaster::getInstance()->performActions(sel.size() == 1 ? String("Ungroup") : "Ungroup " + String(sel.size()) + " groups", actions);
	repaint();
}

void LogicGraphPanel::toggleGroup(LogicGroup* gr)
{
	bool collapse = !gr->collapsed->boolValue();
	gr->collapsed->setUndoableValue(!collapse, collapse);
	//hidden nodes leave the selection : Delete or H never act on what cannot be seen
	if (collapse)
		if (LogicGraph* graph = getGraph())
			for (auto& n : graph->getGroupNodes(gr)) if (n->isSelected) n->deselectThis();
	repaint();
}

namespace
{
	//color capsule of a group : the picker changes its color live, one undo step when it closes
	class GroupColourPicker :
		public ColourSelector,
		private ChangeListener
	{
	public:
		GroupColourPicker(LogicGroup* g) :
			ColourSelector(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace),
			groupRef(g),
			startValue(g->color->getValue())
		{
			setCurrentColour(g->color->getColor(), dontSendNotification);
			setSize(300, 320);
			addChangeListener(this);
		}

		~GroupColourPicker() override
		{
			removeChangeListener(this);
			LogicGroup* g = dynamic_cast<LogicGroup*>(groupRef.get());
			if (g == nullptr) return;
			var endValue = g->color->getValue();
			if (endValue == startValue) return;
			g->color->setValue(startValue, true);
			g->color->setUndoableValue(startValue, endValue);
		}

	private:
		void changeListenerCallback(ChangeBroadcaster*) override
		{
			if (LogicGroup* g = dynamic_cast<LogicGroup*>(groupRef.get())) g->color->setColor(getCurrentColour());
		}

		WeakReference<ControllableContainer> groupRef;
		var startValue;
	};
}

void LogicGraphPanel::showGroupColorPicker(LogicGroup* gr)
{
	Rectangle<float> cr = getGroupColorRect(gr);
	Rectangle<int> area = Rectangle<float>(toScreen(cr.getTopLeft()), toScreen(cr.getBottomRight())).getSmallestIntegerContainer();
	CallOutBox::launchAsynchronously(std::make_unique<GroupColourPicker>(gr), area, this);
}

void LogicGraphPanel::paintSelectRect(Graphics& g)
{
	using namespace LogicGraphStyle;
	if (dragMode != SELECT_RECT) return;
	Rectangle<float> r(toScreen(selectStart), toScreen(selectEnd));
	g.setColour(accent.withAlpha(.08f));
	g.fillRect(r);
	g.setColour(accent.withAlpha(.8f));
	g.drawRect(r, 1.0f);
}

// --- mouse and keys

void LogicGraphPanel::mouseMove(const MouseEvent& e)
{
	if (live) return;
	PinRef p = canvasRect.contains(e.getPosition()) ? getPinAt(toCanvas(e.position)) : PinRef();
	if (p.node != hoveredPin.node || p.pin != hoveredPin.pin || p.output != hoveredPin.output)
	{
		hoveredPin = p;
		repaint();
	}
}

void LogicGraphPanel::mouseDown(const MouseEvent& e)
{
	grabKeyboardFocus();
	if (createMenu != nullptr) { closeCreateMenu(); return; }	// a click beside the create menu closes it
	//header too wide for the panel : a right drag over it scrolls it (in Live Mode too)
	if (!replayingClick && headerOverflow > 0 && e.mods.isRightButtonDown() && headerVisibleRect.contains(e.getPosition()))
	{
		headerScrolling = true;
		headerScrollLastX = e.position.x;
		return;
	}
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;

	if (hitHeader(liveRect, e.getPosition()))
	{
		sm->liveMode.enabled->setValue(!sm->liveMode.isLive());
		timerCallback();
		return;
	}
	if (live) return; //Live Mode : no editing

	//header
	for (int i = 0; i < tabRects.size(); i++)
	{
		if (!hitHeader(tabRects[i], e.getPosition())) continue;
		graphIndex = i;
		if (e.mods.isRightButtonDown()) showTabMenu(i);
		repaint();
		return;
	}
	if (hitHeader(addTabRect, e.getPosition()))
	{
		sm->logicManager.addItem(new LogicGraph(sm));
		graphIndex = sm->logicManager.items.size() - 1;
		resized();
		repaint();
		return;
	}
	if (hitHeader(snapNodesRect, e.getPosition())) { snapToNodes = !snapToNodes; repaint(); return; }
	if (hitHeader(gridRect, e.getPosition())) { snapToGrid = !snapToGrid; repaint(); return; }
	if (hitHeader(zoomRect, e.getPosition())) { frameAll(); return; }
	if (!canvasRect.contains(e.getPosition())) return;

	LogicGraph* graph = getGraph();
	if (graph == nullptr) return;
	Point<float> c = toCanvas(e.position);

	if (e.mods.isLeftButtonDown() && miniMapRect.contains(e.getPosition()) && !graph->nodes.items.isEmpty() && canvasRect.getWidth() >= 420 && canvasRect.getHeight() >= 260)
	{
		dragMode = MINIMAP;
		mouseDrag(e);
		return;
	}

	//right or middle : pan when dragged, menus on a plain right click (mouseUp)
	if (!e.mods.isLeftButtonDown())
	{
		dragMode = PAN;
		panLastMouse = e.position;
		return;
	}

	PinRef pin = getPinAt(c);
	if (pin.isValid())
	{
		//an input already linked : its link is picked up again from its source
		LogicNode* n = dynamic_cast<LogicNode*>(pin.node.get());
		if (!pin.output && n != nullptr)
		{
			for (auto& l : graph->links)
			{
				if (l.toNode != n->uid->stringValue() || l.toPin != pin.pin) continue;
				if (LogicNode* src = graph->getNode(l.fromNode))
				{
					PinRef from;
					from.node = src;
					from.pin = l.fromPin;
					from.output = true;
					from.kind = pin.kind;
					if (const LogicPinDef* d = src->getPin(l.fromPin, true)) from.kind = d->kind;
					LogicGraph::Link old = l;
					graph->disconnect(old);
					wireFrom = from;
					wireMouse = c;
					dragMode = WIRE;
					repaint();
					return;
				}
			}
		}
		wireFrom = pin;
		wireMouse = c;
		dragMode = WIRE;
		repaint();
		return;
	}

	if (LogicNode* n = getNodeAt(c))
	{
		//Ctrl / Shift : adds or removes it from the selection ; a node of the selection moves the whole selection
		if (e.mods.isCtrlDown() || e.mods.isShiftDown())
		{
			if (n->isSelected) { n->deselectThis(); repaint(); return; }
			n->selectThis(true);
		}
		else if (!n->isSelected) n->selectThis();

		dragNode = n;
		dragStartNode = n->viewUIPosition->getPoint();
		dragStartMouse = c;
		dragOthers.clearQuick();
		dragOthersStart.clearQuick();
		for (auto& o : getSelectedNodes())
		{
			if (o == n) continue;
			dragOthers.add(o);
			dragOthersStart.add(o->viewUIPosition->getPoint());
		}
		dragMode = MOVE_NODE;
		repaint();
		return;
	}

	//group : arrow = collapse, capsule = color, title (or a collapsed card) = move with its nodes, corner = resize
	GroupPart part;
	if (LogicGroup* gr = getGroupAt(c, part))
	{
		if (part == GROUP_ARROW) { toggleGroup(gr); return; }
		if (part == GROUP_COLOR) { showGroupColorPicker(gr); return; }
		if (e.mods.isCtrlDown() || e.mods.isShiftDown())
		{
			if (gr->isSelected) gr->deselectThis();
			else gr->selectThis(true);
			repaint();
			return;
		}
		if (!gr->isSelected) gr->selectThis();
		dragGroup = gr;
		dragStartMouse = c;
		dragOthers.clearQuick();
		dragOthersStart.clearQuick();
		if (part == GROUP_RESIZE)
		{
			dragGroupStart = gr->viewUISize->getPoint();
			dragMode = RESIZE_GROUP;
		}
		else
		{
			dragGroupStart = gr->viewUIPosition->getPoint();
			for (auto& n : graph->getGroupNodes(gr))
			{
				dragOthers.add(n);
				dragOthersStart.add(n->viewUIPosition->getPoint());
			}
			dragMode = MOVE_GROUP;
		}
		repaint();
		return;
	}

	//left in the void : selection rectangle when dragged (Ctrl / Shift : adds), a plain click clears the selection (mouseUp)
	dragMode = SELECT_RECT;
	selectStart = selectEnd = c;
	selectAdd = e.mods.isCtrlDown() || e.mods.isShiftDown();
	repaint();
}

void LogicGraphPanel::mouseDrag(const MouseEvent& e)
{
	if (headerScrolling)
	{
		headerScroll -= roundToInt(e.position.x - headerScrollLastX);
		headerScrollLastX = e.position.x;
		resized();
		repaint();
		return;
	}
	if (live) return;
	switch (dragMode)
	{
	case PAN:
		viewOrigin -= (e.position - panLastMouse) / zoom;
		panLastMouse = e.position;
		break;

	case MOVE_NODE:
		if (LogicNode* n = dynamic_cast<LogicNode*>(dragNode.get()))
		{
			n->viewUIPosition->setPoint(snapPosition(n, dragStartNode + (toCanvas(e.position) - dragStartMouse), e.mods.isAltDown()));
			Point<float> delta = n->viewUIPosition->getPoint() - dragStartNode;	// the others follow the snapped node
			for (int i = 0; i < dragOthers.size(); i++)
				if (LogicNode* o = dynamic_cast<LogicNode*>(dragOthers[i].get())) o->viewUIPosition->setPoint(dragOthersStart[i] + delta);
		}
		break;

	case SELECT_RECT:
		selectEnd = toCanvas(e.position);
		break;

	case MOVE_GROUP:
	case RESIZE_GROUP:
		if (LogicGroup* gr = dynamic_cast<LogicGroup*>(dragGroup.get()))
		{
			Point<float> delta = toCanvas(e.position) - dragStartMouse;
			if (snapToGrid && !e.mods.isAltDown()) delta = Point<float>(std::round(delta.x / gridStep) * gridStep, std::round(delta.y / gridStep) * gridStep);
			if (dragMode == RESIZE_GROUP)
			{
				gr->viewUISize->setPoint(jmax(200.0f, dragGroupStart.x + delta.x), jmax(100.0f, dragGroupStart.y + delta.y));
				break;
			}
			gr->viewUIPosition->setPoint(dragGroupStart + delta);
			for (int i = 0; i < dragOthers.size(); i++)
				if (LogicNode* o = dynamic_cast<LogicNode*>(dragOthers[i].get())) o->viewUIPosition->setPoint(dragOthersStart[i] + delta);
		}
		break;

	case WIRE:
		wireMouse = toCanvas(e.position);
		hoveredPin = getPinAt(wireMouse);
		break;

	case MINIMAP:
	{
		//the view is centred on the clicked point of the overview
		LogicGraph* graph = getGraph();
		if (graph == nullptr || graph->nodes.items.isEmpty()) break;
		Rectangle<float> view(toCanvas(canvasRect.getTopLeft().toFloat()), toCanvas(canvasRect.getBottomRight().toFloat()));
		Rectangle<float> world = view;
		Rectangle<float> content = getContentBounds(graph);
		if (!content.isEmpty()) world = world.getUnion(content);
		world = world.expanded(40);
		Rectangle<float> inner = miniMapRect.toFloat().reduced(10, 10).withTrimmedTop(12);
		float s = jmin(inner.getWidth() / world.getWidth(), inner.getHeight() / world.getHeight());
		Point<float> o = inner.getCentre() - world.getCentre() * s;
		Point<float> target = (e.position - o) / s;
		viewOrigin = target - Point<float>((float)canvasRect.getWidth(), (float)canvasRect.getHeight()) / (2 * zoom);
		break;
	}

	default: return;
	}
	repaint();
}

void LogicGraphPanel::mouseUp(const MouseEvent& e)
{
	if (headerScrolling)
	{
		headerScrolling = false;
		if (e.getDistanceFromDragStart() < 4)
		{
			replayingClick = true;
			mouseDown(e);
			replayingClick = false;
		}
		return;
	}
	if (live) return;
	LogicGraph* graph = getGraph();
	Point<float> c = toCanvas(e.position);

	if (dragMode == MOVE_NODE)
	{
		//one undo step for the whole move, every node moved
		Array<UndoableAction*> actions;
		auto addMove = [&actions](LogicNode* n, Point<float> start)
			{
				Point<float> end = n->viewUIPosition->getPoint();
				if (end == start) return;
				n->viewUIPosition->setPoint(start);
				if (UndoableAction* a = n->viewUIPosition->setUndoablePoint(start, end, true)) actions.add(a);
			};
		if (LogicNode* n = dynamic_cast<LogicNode*>(dragNode.get())) addMove(n, dragStartNode);
		for (int i = 0; i < dragOthers.size(); i++)
			if (LogicNode* o = dynamic_cast<LogicNode*>(dragOthers[i].get())) addMove(o, dragOthersStart[i]);
		if (!actions.isEmpty()) UndoMaster::getInstance()->performActions(actions.size() > 1 ? "Move " + String(actions.size()) + " nodes" : String("Move node"), actions);
		dragOthers.clearQuick();
		dragOthersStart.clearQuick();
		guides.clearQuick();
		snapLabel.clear();
	}
	else if (dragMode == MOVE_GROUP || dragMode == RESIZE_GROUP)
	{
		//one undo step : the group and the nodes moved with it
		Array<UndoableAction*> actions;
		auto addPoint = [&actions](Point2DParameter* p, Point<float> start)
			{
				Point<float> end = p->getPoint();
				if (end == start) return;
				p->setPoint(start);
				if (UndoableAction* a = p->setUndoablePoint(start, end, true)) actions.add(a);
			};
		if (LogicGroup* gr = dynamic_cast<LogicGroup*>(dragGroup.get())) addPoint(dragMode == RESIZE_GROUP ? gr->viewUISize : gr->viewUIPosition, dragGroupStart);
		for (int i = 0; i < dragOthers.size(); i++)
			if (LogicNode* o = dynamic_cast<LogicNode*>(dragOthers[i].get())) addPoint(o->viewUIPosition, dragOthersStart[i]);
		if (!actions.isEmpty()) UndoMaster::getInstance()->performActions(dragMode == RESIZE_GROUP ? "Resize group" : "Move group", actions);
		dragOthers.clearQuick();
		dragOthersStart.clearQuick();
	}
	else if (dragMode == SELECT_RECT && graph != nullptr)
	{
		//every node touched by the rectangle ; a plain click selects nothing
		if (!selectAdd && InspectableSelectionManager::mainSelectionManager != nullptr) InspectableSelectionManager::mainSelectionManager->clearSelection();
		if (e.getDistanceFromDragStart() >= 4)
		{
			Rectangle<float> r(selectStart, selectEnd);
			for (auto& n : graph->nodes.items)
				if (r.intersects(getNodeRect(n)) && !n->isSelected && graph->getCollapsedGroupOf(n) == nullptr) n->selectThis(true);
		}
	}
	else if (dragMode == WIRE && graph != nullptr)
	{
		PinRef target = getPinAt(c);
		LogicNode* a = dynamic_cast<LogicNode*>(wireFrom.node.get());
		LogicNode* b = dynamic_cast<LogicNode*>(target.node.get());
		if (a != nullptr && b != nullptr && target.output != wireFrom.output)
		{
			LogicGraph::Link l;
			if (wireFrom.output) l = { a->uid->stringValue(), wireFrom.pin, b->uid->stringValue(), target.pin };
			else l = { b->uid->stringValue(), target.pin, a->uid->stringValue(), wireFrom.pin };
			graph->connect(l);
		}
	}
	else if (dragMode == PAN && e.mods.isRightButtonDown() && e.getDistanceFromDragStart() < 4 && graph != nullptr)
	{
		//on a group title : that group is selected first, so Ungroup acts on it
		GroupPart part;
		if (LogicGroup* gr = getGroupAt(c, part))
			if (part != GROUP_NONE && !gr->isSelected) gr->selectThis();
		showCanvasMenu(c);	// a plain right click only creates nodes (Delete and H act on the selection)
	}

	dragMode = NONE;
	wireFrom = PinRef();
	repaint();
}

void LogicGraphPanel::mouseDoubleClick(const MouseEvent& e)
{
	if (live) return;
	for (int i = 0; i < tabRects.size(); i++)
	{
		if (!hitHeader(tabRects[i], e.getPosition())) continue;
		if (SpatialModule* sm = getSpatialModule())
			if (i < sm->logicManager.items.size()) renameGraph(sm->logicManager.items[i]);
		return;
	}
	if (!canvasRect.contains(e.getPosition())) return;
	Point<float> c = toCanvas(e.position);
	if (getNodeAt(c) != nullptr) return;
	GroupPart part;
	if (LogicGroup* gr = getGroupAt(c, part))
	{
		if (part == GROUP_TITLE) toggleGroup(gr);	// double click on the title : collapse / expand
		return;
	}
	frameAll();
}

void LogicGraphPanel::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	if (headerOverflow > 0 && headerVisibleRect.contains(e.getPosition()))
	{
		float d = std::abs(wheel.deltaX) > std::abs(wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
		headerScroll -= roundToInt(d * 160);
		resized();
		repaint();
		return;
	}
	if (live || !canvasRect.contains(e.getPosition())) return;
	Point<float> c = toCanvas(e.position);
	zoom = jlimit(.3f, 2.0f, zoom * (1 + wheel.deltaY * .6f));
	viewOrigin = c - (e.position - canvasRect.getPosition().toFloat()) / zoom;
	repaint();
}

bool LogicGraphPanel::keyPressed(const KeyPress& key)
{
	SpatialModule* sm = getSpatialModule();
	if (key == KeyPress('L', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0) && sm != nullptr)
	{
		sm->liveMode.enabled->setValue(!sm->liveMode.isLive());
		timerCallback();
		return true;
	}
	if (live) return false;

	if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
	{
		if (getSelectedNodes().isEmpty() && getSelectedGroups().isEmpty()) return false;
		removeSelectedNodes();
		return true;
	}
	if (key == KeyPress('g', ModifierKeys::shiftModifier, 0) || key == KeyPress('G', ModifierKeys::shiftModifier, 0))
	{
		if (getSelectedGroups().isEmpty()) return false;
		ungroupSelected();
		return true;
	}
	if (key == KeyPress('g') || key == KeyPress('G'))
	{
		if (getSelectedNodes().isEmpty()) return false;
		groupSelectedNodes();
		return true;
	}
	if (key == KeyPress('h') || key == KeyPress('H'))
	{
		if (getSelectedNodes().isEmpty()) return false;
		toggleHideSelected();
		return true;
	}
	if (key == KeyPress('f') || key == KeyPress('F'))
	{
		frameAll();
		return true;
	}
	return false;
}

void LogicGraphPanel::paintHeaderEdges(Graphics& g)
{
	using namespace LogicGraphStyle;
	if (headerOverflow <= 0) return;

	//a fade and an arrow on each side that still hides something : right drag (or the wheel) scrolls the header
	auto edge = [&](bool left)
		{
			Rectangle<float> r = headerVisibleRect.toFloat();
			r = left ? r.removeFromLeft(28) : r.removeFromRight(28);
			ColourGradient grad(canvasBg, left ? r.getX() : r.getRight(), r.getY(), canvasBg.withAlpha(0.0f), left ? r.getRight() : r.getX(), r.getY(), false);
			g.setGradientFill(grad);
			g.fillRect(r);
			Path p;
			float cx = left ? r.getX() + 8 : r.getRight() - 8, cy = r.getCentreY();
			if (left) p.addTriangle(cx + 3, cy - 5, cx + 3, cy + 5, cx - 3, cy);
			else p.addTriangle(cx - 3, cy - 5, cx - 3, cy + 5, cx + 3, cy);
			g.setColour(text.withAlpha(.8f));
			g.fillPath(p);
		};
	if (headerScroll > 0) edge(true);
	if (headerScroll < headerOverflow) edge(false);
}
