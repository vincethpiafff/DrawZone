/*
  ==============================================================================

	TrackingMonitorPanel.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Dockable panel showing, in real time, the people received by the
	first Spatial module and its zones : a top view on the left, a Zones /
	People column on the right (collapsible to a narrow rail).
	Refreshed at a fixed rate from a TrackingData snapshot, so the display
	stays readable whatever the input message rate.

  ==============================================================================
*/

#pragma once

class SpatialModule;
class SpatialZone;

class TrackingMonitorPanel :
	public ShapeShifterContentComponent,
	private Timer
{
public:
	TrackingMonitorPanel(const String& contentName);
	~TrackingMonitorPanel();

	static ShapeShifterContent* create(const String& contentName) { return new TrackingMonitorPanel(contentName); }

	static const int refreshRateHz = 10;

	void paint(Graphics& g) override;
	void resized() override;
	void timerCallback() override;

	//Zone editing in the top view : drag to move, corner handles to resize, right click to duplicate
	void mouseMove(const MouseEvent& e) override;
	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override; // on a zone name : rename it
	bool keyPressed(const KeyPress& key) override; // while drawing a free shape : Escape cancels, Backspace removes the last point

private:
	SpatialModule* getSpatialModule();
	void paintHeader(Graphics& g);
	void paintTopView(Graphics& g, Rectangle<int> r);
	void paintColumn(Graphics& g);
	void paintRail(Graphics& g);

	static Colour getPersonColor(int id);

	static constexpr float handleSize = 8;
	static const int lineHeight = 18;
	static const int columnWidth = 340;
	static const int railWidth = 56;

	//layout, computed in resized()
	Rectangle<int> headerRect;
	Rectangle<int> topViewRect;
	Rectangle<int> columnRect;
	Rectangle<int> slotBarRect;	// player slots strip under the top view

	//player slots strip : one chip per slot, like a game HUD
	static const int slotBarHeight = 40;
	void layoutSlotBar();
	void paintSlotBar(Graphics& g);
	Rectangle<int> slotBarLabelRect;
	Array<Rectangle<int>> slotChipRects;	// one per slot, empty when it does not fit
	TextButton slotMinusButton;
	TextButton slotPlusButton;

	//SwapSlots mode chip, at the left of the strip : opens a menu with both modes explained
	Rectangle<int> slotModeRect;
	bool slotModeHovered = false;
	void showSlotModeMenu();
	void changeSlotCount(int delta);
	int getSlotChipAt(Point<int> pos) const;
	void showSlotMenu(int slotIndex, Point<int> clickPosition);

	//column layout, computed in layoutColumn() (depends on the number of zones)
	void layoutColumn();
	Rectangle<int> zonesHeaderRect;
	Array<Rectangle<int>> zoneRowRects;	// one per zone, empty when it does not fit
	Rectangle<int> peopleRect;

	//people rows of the column or dots of the rail from the last paint : a click selects the person (Inspector)
	Array<Rectangle<int>> personRowRects;
	Array<int> personRowIds;
	int getPersonAt(Point<float> pos) const;	// id of the person in the top view, column or rail, -1 = none
	void selectPerson(int id);

	//player slot by hand : right click menu on a person, or keys 1-9 / 0 on the selected person
	void showPersonMenu(int personId, Point<int> clickPosition);
	void assignSlot(int personId, int slotIndex);	// slotIndex -1 = release

	//drag and drop : a person onto a slot chip, or a slot chip onto a person
	int slotDragPersonId = -1;	// dragged person
	int slotDragSlot = -1;		// dragged slot chip
	bool slotDragging = false;	// moved far enough from the press to be a drag
	Point<float> slotDragStart, slotDragMouse;
	Point<float> getPersonScreenPosition(int id) const;	// top view dot, else its row in the column / rail
	void paintSlotDrag(Graphics& g);

	//"Shortcuts" chip in the header : unfolds the list of shortcuts on hover, pinned open by a click
	Rectangle<int> shortcutsRect;
	bool shortcutsHovered = false;
	bool shortcutsPinned = false;
	Rectangle<int> getShortcutsPanelRect() const;
	void paintShortcuts(Graphics& g);
	void mouseExit(const MouseEvent& e) override;
	int selectedPersonId = -1;	// shown highlighted

	//zone cards : name, count and occupancy bar ; the arrow unfolds the detail of the values
	static const int cardHeight = 50;
	static const int detailLineHeight = 18;
	static const int numDetailLines = 5;
	Array<WeakReference<ControllableContainer>> expandedZones;
	bool isExpanded(ControllableContainer* zone) const;
	static Rectangle<int> getCardArrowRect(Rectangle<int> card);

	bool columnCollapsed = false;
	TextButton collapseButton;
	TextButton createZoneButton;
	TextButton showHiddenButton;
	int numHidden = 0;
	void showHiddenZones();
	void createZone();
	void createZoneAt(Point<float> positionMeters);

	//Chataigne's own color picker on the zone or slot color, opened from the right click menu
	std::unique_ptr<ColorParameterUI> zoneColorUI;
	void showZoneColorEditor(SpatialZone* zone, Point<int> position);
	void showColorEditor(Inspectable* owner, ColorParameter* color, Point<int> position);

	//right click menu on a zone (top view or column), or "Create zone" on the empty scene
	void showContextMenu(SpatialZone* zone, Point<int> clickPosition, bool canCreate);
	//right click on a free shape point : Linear / Quadratic / Cubic
	void showPointMenu(SpatialZone* zone, int pointIndex, Point<int> clickPosition);

	//free shape drawing : each click adds a point, a click on the first point or a double click closes it
	bool drawingFreeShape = false;
	Array<Point<float>> drawPoints;	// meters
	Point<float> drawMouse;			// screen
	uint32 drawFinishedMs = 0;		// the double click that closes the shape must not open the rename editor
	void startFreeShapeDrawing();
	void finishFreeShapeDrawing();
	void cancelFreeShapeDrawing();
	void paintFreeShapeDrawing(Graphics& g);
	Point<float> toMeters(Point<float> screen) const;	// clamped to the scene

	//rectangle / ellipse drawing : press, drag, release
	int drawingBoxShape = -1;	// SpatialZone::RECTANGLE or ELLIPSE, -1 = not drawing
	bool boxDragging = false;
	Point<float> boxStart, boxEnd;	// meters
	void startBoxDrawing(int boxShape);
	void finishBoxDrawing();
	void cancelBoxDrawing();
	void paintBoxDrawing(Graphics& g);
	void paintDrawingHelp(Graphics& g, const String& text);

	//inline rename of a zone or a slot, opened by a double click on its name (column, top view or slots strip)
	TextEditor renameEditor;
	WeakReference<ControllableContainer> renameZone;
	void showRenameEditor(ControllableContainer* zone, Rectangle<int> bounds);
	void closeRenameEditor(bool apply);

	//top view geometry from the last paint, used to convert mouse positions to meters
	Rectangle<float> sceneScreenRect;
	float sceneScale = 0;	// pixels per meter, 0 = no top view

	Rectangle<float> toScreen(Rectangle<float> meters) const;

	//hit test : zone index (topmost first), corner (0 top left, 1 top right, 2 bottom right, 3 bottom left, -1 inside) and free shape point (-1 none)
	struct ZoneHit { int zoneIndex = -1; int corner = -1; int pointIndex = -1; int handle = 0; };	// handle of pointIndex : 1 in, 2 out
	ZoneHit hitTest(Point<float> pos) const;

	int hoveredZone = -1;

	WeakReference<ControllableContainer> dragZone;
	int dragCorner = -1;
	Rectangle<float> dragStartBounds;	// meters
	Point<float> dragStartCenter;
	Point<float> dragStartSize;
	int dragPoint = -1;	// free shape point being moved
	Array<Point<float>> dragStartPointsMeters;
	Array<Point<float>> dragStartPoints;	// 0-1 in the box, for the undo
	int dragHandle = 0;	// curve handle being moved (1 in, 2 out) of dragPoint
	Array<Point<float>> dragStartHandlesIn, dragStartHandlesOut;	// meters from each point

	//copied on each refresh, so painting never touches the network data
	bool hasModule = false;
	String moduleName;
	int localPort = 0;
	TrackingData::Snapshot snapshot;

	struct ZoneView
	{
		WeakReference<ControllableContainer> zone;
		String name;
		Rectangle<float> bounds;	// meters
		bool ellipse = false;
		bool freeShape = false;	// neither : rectangle
		Path outline;				// meters, free shape only
		Array<Point<float>> points;	// meters, free shape only
		Array<int> interpolations;	// ZonePoint::Interpolation of each point
		Array<Point<float>> handlesIn, handlesOut;	// curve control points, meters
		Colour color;
		bool enabled = true;
		bool presence = false;
		bool hidden = false;
		bool selected = false;
		int personCount = 0;

		//live values shown on the zone cards
		int capacity = 1;
		float occupancy = 0;		// 0-1
		float presenceTime = 0;		// seconds
		int lastEnteredId = -1;
		Point<float> averagePosition;	// 0-1 in the box
		float averageDistance = 0;	// meters
	};
	Array<ZoneView> zones;

	struct SlotView
	{
		WeakReference<ControllableContainer> slot;
		String name;
		Colour color;
		bool occupied = false;
		bool held = false;
		int personId = -1;
		Point<float> position;		// meters
		float holdRemaining = 0;	// seconds, -1 = held until reclaimed
	};
	Array<SlotView> slots;
	bool slotsEnabled = false;
	int slotReleaseMode = 0;	// PlayerSlots::ReleaseMode
	int freeSlots = 0;
	float reclaimRadius = 0;	// meters
	float holdTime = 0;			// seconds, 0 = until reclaimed
	int getSlotIndexForPerson(int id) const;	// -1 = no slot
	Array<int> personZones;	// zone index for each person of the snapshot, -1 = in no zone
	Array<Point<float>> personRelativePositions;	// for each person of the snapshot, from Values > People, -1 = in no zone

	//motion measured here from the successive positions (independent of the source velocity units)
	struct Motion
	{
		Point<float> lastPosition;	// meters
		uint32 lastMs = 0;
		Point<float> velocity;		// meters per second, smoothed
		Array<Point<float>> trail;	// recent positions in meters, oldest first
		uint32 lastTrailMs = 0;
	};
	std::map<int, Motion> motions;	// by person id
	void updateMotions();

	static constexpr float minArrowSpeed = .15f;	// m/s, below that the person is considered still
	static const int trailStepMs = 100;			// one trail point every 100 ms...
	static const int trailLength = 20;			// ...kept for 2 s

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingMonitorPanel)
};
