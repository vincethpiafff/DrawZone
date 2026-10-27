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

private:
	SpatialModule* getSpatialModule();
	void paintHeader(Graphics& g);
	void paintTopView(Graphics& g, Rectangle<int> r);
	void paintColumn(Graphics& g);
	void paintRail(Graphics& g);

	static Colour getPersonColor(int id);

	static constexpr float handleSize = 8;
	static const int lineHeight = 18;
	static const int columnWidth = 240;
	static const int railWidth = 56;

	//layout, computed in resized()
	Rectangle<int> headerRect;
	Rectangle<int> topViewRect;
	Rectangle<int> columnRect;

	//column layout, computed in layoutColumn() (depends on the number of zones)
	void layoutColumn();
	Rectangle<int> zonesHeaderRect;
	Array<Rectangle<int>> zoneRowRects;	// one per zone, empty when it does not fit
	Rectangle<int> peopleRect;

	bool columnCollapsed = false;
	TextButton collapseButton;
	TextButton createZoneButton;
	TextButton showHiddenButton;
	int numHidden = 0;
	void showHiddenZones();
	void createZone();
	void createZoneAt(Point<float> positionMeters);

	//Chataigne's own color picker on the zone color, opened from the right click menu
	std::unique_ptr<ColorParameterUI> zoneColorUI;
	void showZoneColorEditor(SpatialZone* zone, Point<int> position);

	//top view geometry from the last paint, used to convert mouse positions to meters
	Rectangle<float> sceneScreenRect;
	float sceneScale = 0;	// pixels per meter, 0 = no top view

	Rectangle<float> toScreen(Rectangle<float> meters) const;

	//hit test : zone index (topmost first) and corner (0 top left, 1 top right, 2 bottom right, 3 bottom left, -1 inside)
	struct ZoneHit { int zoneIndex = -1; int corner = -1; };
	ZoneHit hitTest(Point<float> pos) const;

	int hoveredZone = -1;

	WeakReference<ControllableContainer> dragZone;
	int dragCorner = -1;
	Rectangle<float> dragStartBounds;	// meters
	Point<float> dragStartCenter;
	Point<float> dragStartSize;

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
		Colour color;
		bool enabled = true;
		bool presence = false;
		bool hidden = false;
		bool selected = false;
		int personCount = 0;
	};
	Array<ZoneView> zones;
	Array<int> personZones;	// zone index for each person of the snapshot, -1 = in no zone

	//motion measured here from the successive positions (independent of the source velocity units)
	struct Motion
	{
		Point<float> lastPosition;	// meters
		uint32 lastMs = 0;
		Point<float> velocity;		// meters per second, smoothed
	};
	std::map<int, Motion> motions;	// by person id
	void updateMotions();

	static constexpr float minArrowSpeed = .15f;	// m/s, below that the person is considered still

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingMonitorPanel)
};
