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
	if (graph == nullptr || graph->nodes.items.isEmpty() || canvasRect.isEmpty())
	{
		zoom = 1;
		viewOrigin = { -40, -40 };
		repaint();
		return;
	}
	Rectangle<float> b = getNodeRect(graph->nodes.items[0]);
	for (auto& n : graph->nodes.items) b = b.getUnion(getNodeRect(n));
	b = b.expanded(60);
	zoom = jlimit(.3f, 1.5f, jmin(canvasRect.getWidth() / b.getWidth(), canvasRect.getHeight() / b.getHeight()));
	viewOrigin = b.getCentre() - Point<float>((float)canvasRect.getWidth(), (float)canvasRect.getHeight()) / (2 * zoom);
	repaint();
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
	int rows = jmax(in, out) + (n->getSummaryText().isNotEmpty() ? 1 : 0);
	Point<float> pos = n->viewUIPosition->getPoint();
	return Rectangle<float>(pos.x, pos.y, (float)n->getWidth(), (float)(headerHeight + padding * 2 + rows * rowHeight));
}

Point<float> LogicGraphPanel::getPinPosition(LogicNode* n, const String& pin, bool output) const
{
	Rectangle<float> r = getNodeRect(n);
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
		for (auto& p : n->pins)
			if (getPinPosition(n, p.name, p.output).getDistanceFrom(c) <= radius)
			{
				found.node = n;
				found.pin = p.name;
				found.output = p.output;
				found.kind = p.kind;
			}
	return found;
}

LogicNode* LogicGraphPanel::getNodeAt(Point<float> c) const
{
	LogicGraph* graph = const_cast<LogicGraphPanel*>(this)->getGraph();
	if (graph == nullptr) return nullptr;
	for (int i = graph->nodes.items.size() - 1; i >= 0; i--)
		if (getNodeRect(graph->nodes.items[i]).contains(c)) return graph->nodes.items[i];
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
		if (o == n) continue;
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
			paintLinks(g, graph);
			for (auto& n : graph->nodes.items) paintNode(g, graph, n);
			paintGuides(g);
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
		LogicNode* to = graph->getNode(l.toNode);
		if (from == nullptr || to == nullptr) continue;
		const LogicPinDef* pin = from->getPin(l.fromPin, true);
		if (pin == nullptr) continue;
		g.setColour(LogicPin::getColour(pin->kind));
		g.strokePath(getWirePath(toScreen(getPinPosition(from, l.fromPin, true)), toScreen(getPinPosition(to, l.toPin, false))), PathStrokeType(jmax(1.2f, 2 * zoom)));
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
	Rectangle<float> rc = getNodeRect(n);
	Rectangle<float> r(toScreen(rc.getTopLeft()), toScreen(rc.getBottomRight()));
	if (!r.intersects(canvasRect.toFloat())) return;

	float alpha = n->enabled->boolValue() ? 1.0f : .5f;
	float radius = 10 * zoom;
	bool selected = n->isSelected;

	if (selected)
	{
		g.setColour(accent.withAlpha(.15f));
		g.fillRoundedRectangle(r.expanded(4 * zoom), radius + 4 * zoom);
	}
	g.setColour(nodeBg.withMultipliedAlpha(alpha));
	g.fillRoundedRectangle(r, radius);
	g.setColour(selected ? accent : nodeBorder);
	g.drawRoundedRectangle(r.reduced(.5f), radius, selected ? 1.5f : 1);

	bool showText = zoom >= .45f;
	float hh = headerHeight * zoom;
	Rectangle<float> header = r.withHeight(hh);
	g.setColour(nodeLine);
	g.drawHorizontalLine((int)header.getBottom(), r.getX() + 1, r.getRight() - 1);

	LogicGraphStyle::drawCategoryIcon(g, n->getCategory(), Rectangle<float>(20 * zoom, 20 * zoom).withCentre(Point<float>(r.getX() + 22 * zoom, header.getCentreY())));
	if (showText)
	{
		Colour cat = LogicNode::getCategoryColour(n->getCategory());
		g.setFont(Font(10 * zoom, Font::bold));
		String catName = LogicNode::getCategoryName(n->getCategory()).toUpperCase();
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
		Point<float> s = toScreen(getPinPosition(n, p.name, p.output));
		String uid = n->uid->stringValue();
		bool linked = false;
		for (auto& l : graph->links)
			if ((p.output && l.fromNode == uid && l.fromPin == p.name) || (!p.output && l.toNode == uid && l.toPin == p.name)) { linked = true; break; }

		Colour c = LogicPin::getColour(p.kind);
		bool hovered = hoveredPin.isValid() && hoveredPin.node == n && hoveredPin.pin == p.name && hoveredPin.output == p.output;
		float size = (hovered ? 14.0f : 12.0f) * zoom;
		if (p.kind == LogicPin::EXEC)
		{
			Path d;
			d.addRectangle(-size / 2.8f, -size / 2.8f, size / 1.4f, size / 1.4f);
			d.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 4).translated(s));
			g.setColour(c.withMultipliedAlpha(linked ? alpha : alpha * .7f));
			g.fillPath(d);
		}
		else
		{
			Rectangle<float> e = Rectangle<float>(size, size).withCentre(s);
			g.setColour(linked ? c : canvasBg);
			g.fillEllipse(e);
			g.setColour(c.withMultipliedAlpha(alpha));
			g.drawEllipse(e.reduced(1 * zoom), 2 * zoom);
		}

		if (showText)
		{
			g.setColour(dim.withMultipliedAlpha(alpha));
			g.setFont(Font(12 * zoom));
			Rectangle<float> row(r.getX() + 14 * zoom, s.y - rowHeight * zoom / 2, r.getWidth() - 28 * zoom, rowHeight * zoom);
			g.drawText(p.name, row, p.output ? Justification::centredRight : Justification::centredLeft, true);
		}
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
	for (auto& n : graph->nodes.items) world = world.getUnion(getNodeRect(n));
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
	for (auto& n : graph->nodes.items)
	{
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

void LogicGraphPanel::showCanvasMenu(Point<float> canvas)
{
	LogicGraph* graph = getGraph();
	if (graph == nullptr) return;
	PopupMenu m = graph->nodes.factory.getMenu();
	WeakReference<ControllableContainer> ref(graph);
	Point<float> pos = snapToGrid ? Point<float>(std::round(canvas.x / gridStep) * gridStep, std::round(canvas.y / gridStep) * gridStep) : canvas;
	m.showMenuAsync(PopupMenu::Options(), [ref, pos](int result)
		{
			LogicGraph* gr = dynamic_cast<LogicGraph*>(ref.get());
			if (gr == nullptr || result <= 0) return;
			if (LogicNode* n = gr->nodes.factory.createFromMenuResult(result)) gr->nodes.addItem(n, pos);
		});
}

void LogicGraphPanel::showNodeMenu(LogicNode* n)
{
	LogicGraph* graph = getGraph();
	if (graph == nullptr) return;
	PopupMenu m;
	m.addItem(1, "Delete");
	m.addItem(2, "Disconnect all");
	WeakReference<Inspectable> ref(n);
	WeakReference<Component> panelRef(this);
	m.showMenuAsync(PopupMenu::Options(), [ref, panelRef](int result)
		{
			LogicGraphPanel* panel = dynamic_cast<LogicGraphPanel*>(panelRef.get());
			LogicNode* node = dynamic_cast<LogicNode*>(ref.get());
			if (node == nullptr || panel == nullptr) return;
			LogicGraph* gr = panel->getGraph();
			if (gr == nullptr) return;
			if (result == 1) panel->removeNode(node);
			else if (result == 2)
			{
				Array<UndoableAction*> actions = gr->getDisconnectNodeActions(node);
				if (!actions.isEmpty()) UndoMaster::getInstance()->performActions("Disconnect " + node->niceName, actions);
			}
			panel->repaint();
		});
}

void LogicGraphPanel::showPinMenu(const PinRef& p)
{
	LogicGraph* graph = getGraph();
	LogicNode* n = dynamic_cast<LogicNode*>(p.node.get());
	if (graph == nullptr || n == nullptr) return;
	PopupMenu m;
	m.addItem(1, "Disconnect");
	WeakReference<ControllableContainer> ref(graph);
	String uid = n->uid->stringValue(), pin = p.pin;
	bool output = p.output;
	m.showMenuAsync(PopupMenu::Options(), [ref, uid, pin, output](int result)
		{
			LogicGraph* gr = dynamic_cast<LogicGraph*>(ref.get());
			if (gr == nullptr || result != 1) return;
			for (auto& l : Array<LogicGraph::Link>(gr->links))
				if ((output && l.fromNode == uid && l.fromPin == pin) || (!output && l.toNode == uid && l.toPin == pin)) gr->disconnect(l);
		});
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
		n->selectThis();
		dragNode = n;
		dragStartNode = n->viewUIPosition->getPoint();
		dragStartMouse = c;
		dragMode = MOVE_NODE;
		repaint();
		return;
	}

	if (InspectableSelectionManager::mainSelectionManager != nullptr) InspectableSelectionManager::mainSelectionManager->clearSelection();
	dragMode = PAN;
	panLastMouse = e.position;
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
			n->viewUIPosition->setPoint(snapPosition(n, dragStartNode + (toCanvas(e.position) - dragStartMouse), e.mods.isAltDown()));
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
		for (auto& n : graph->nodes.items) world = world.getUnion(getNodeRect(n));
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
		//one undo step for the whole move
		if (LogicNode* n = dynamic_cast<LogicNode*>(dragNode.get()))
		{
			Point<float> end = n->viewUIPosition->getPoint();
			if (end != dragStartNode)
			{
				n->viewUIPosition->setPoint(dragStartNode);
				n->viewUIPosition->setUndoablePoint(dragStartNode, end);
			}
		}
		guides.clearQuick();
		snapLabel.clear();
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
		PinRef pin = getPinAt(c);
		if (pin.isValid()) showPinMenu(pin);
		else if (LogicNode* n = getNodeAt(c))
		{
			n->selectThis();
			showNodeMenu(n);
		}
		else showCanvasMenu(c);
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
	if (canvasRect.contains(e.getPosition()) && getNodeAt(toCanvas(e.position)) == nullptr) frameAll();
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
		LogicGraph* graph = getGraph();
		if (graph == nullptr) return false;
		for (auto& n : graph->nodes.items)
			if (n->isSelected)
			{
				removeNode(n);
				return true;
			}
		return false;
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
