/*
  ==============================================================================

	LogicGraphPanel.h
	Created: 6 Nov 2026
	Author:  DrawZone

	"Logic Graph" panel : edits the graphs of the Logic block of the first
	Cockpit module (look B of 06/11/2026 : like the Tracking Monitor).
	Tabs for the graphs, a dotted canvas (wheel = zoom, right or middle drag
	= pan), nodes created with a right click, moved by dragging (magnetic on
	the edges and centres of the other nodes, optional 20 px grid, Alt = free),
	links pulled from pin to pin, a mini-map. Everything goes through the undo.

	The graphs never run here : the module runs them (LogicGraph.h). Live Mode
	freezes this panel and locks editing.

  ==============================================================================
*/

#pragma once

class LogicGraphPanel :
	public ShapeShifterContentComponent,
	private Timer
{
public:
	LogicGraphPanel(const String& contentName);
	~LogicGraphPanel();

	static ShapeShifterContent* create(const String& contentName) { return new LogicGraphPanel(contentName); }

	static const int refreshRateHz = 15;

	void paint(Graphics& g) override;
	void resized() override;
	void timerCallback() override;

	void mouseMove(const MouseEvent& e) override;
	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
	bool keyPressed(const KeyPress& key) override;	// Delete : node, F : frame all, Ctrl + Shift + L : Live Mode

private:
	SpatialModule* getSpatialModule();
	LogicGraph* getGraph();	// the graph of the selected tab
	int graphIndex = 0;

	//layout
	Rectangle<int> headerRect, canvasRect, miniMapRect;
	//header wider than the panel : scrolled by a right drag or the wheel over it, clicks only in its visible part
	Rectangle<int> headerVisibleRect;
	int headerScroll = 0, headerOverflow = 0;
	bool headerScrolling = false;
	bool replayingClick = false;	// a right click without drag on the header keeps its usual action
	float headerScrollLastX = 0;
	bool hitHeader(Rectangle<int> r, Point<int> p) const { return r.contains(p) && headerVisibleRect.contains(p); }
	Array<Rectangle<int>> tabRects;
	Rectangle<int> addTabRect, snapNodesRect, gridRect, zoomRect, liveRect;

	//view : screen = canvasRect top left + (canvas - viewOrigin) x zoom
	Point<float> viewOrigin { -40, -40 };
	float zoom = 1;
	Point<float> toScreen(Point<float> canvas) const;
	Point<float> toCanvas(Point<float> screen) const;
	void frameAll();

	//options of the panel (not saved in the session)
	bool snapToNodes = true;
	bool snapToGrid = false;
	static constexpr float gridStep = 20;

	//node geometry, canvas pixels
	static const int headerHeight = 34;
	static const int rowHeight = 26;
	static const int padding = 6;
	Rectangle<float> getNodeRect(LogicNode* n) const;
	Point<float> getPinPosition(LogicNode* n, const String& pin, bool output) const;

	struct PinRef
	{
		WeakReference<Inspectable> node;
		String pin;
		bool output = false;
		LogicPin::Kind kind = LogicPin::EXEC;
		bool isValid() const { return node != nullptr && pin.isNotEmpty(); }
	};
	PinRef getPinAt(Point<float> canvas) const;
	LogicNode* getNodeAt(Point<float> canvas) const;

	//interactions
	enum DragMode { NONE, PAN, MOVE_NODE, WIRE, MINIMAP };
	DragMode dragMode = NONE;
	WeakReference<Inspectable> dragNode;
	Point<float> dragStartNode, dragStartMouse;	// canvas
	Point<float> panLastMouse;					// screen
	PinRef wireFrom;
	Point<float> wireMouse;						// canvas
	PinRef hoveredPin;

	//magnetic guides while a node moves (canvas), and what it is aligned on
	Array<Line<float>> guides;
	String snapLabel;
	Point<float> snapPosition(LogicNode* n, Point<float> wanted, bool free);

	void showCanvasMenu(Point<float> canvas);
	void showNodeMenu(LogicNode* n);
	void showPinMenu(const PinRef& p);
	void showTabMenu(int index);
	void removeNode(LogicNode* n);
	void renameGraph(LogicGraph* g);

	bool live = false;

	void paintHeader(Graphics& g);
	void paintHeaderEdges(Graphics& g);
	void paintGrid(Graphics& g);
	void paintLinks(Graphics& g, LogicGraph* graph);
	void paintNode(Graphics& g, LogicGraph* graph, LogicNode* n);
	void paintGuides(Graphics& g);
	void paintMiniMap(Graphics& g, LogicGraph* graph);
	void paintLive(Graphics& g);
	Path getWirePath(Point<float> from, Point<float> to) const;	// screen

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogicGraphPanel)
};
