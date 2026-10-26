/*
  ==============================================================================

	TrackingMonitorPanel.h
	Created: 26 Oct 2026
	Author:  DrawZone

	Dockable panel showing, in real time, the people received by the
	first Spatial module (id, position, age, speed).
	Refreshed at a fixed rate from a TrackingData snapshot, so the display
	stays readable whatever the input message rate.

  ==============================================================================
*/

#pragma once

class SpatialModule;

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
	void paintTopView(Graphics& g, Rectangle<int> r);

	static constexpr float handleSize = 8;
	static const int lineHeight = 18;

	//layout, computed in resized()
	Rectangle<int> headerRect;
	Rectangle<int> topViewRect;
	Rectangle<int> tableRect;

	TextButton showHiddenButton;
	void showHiddenZones();
	void createZoneAt(Point<float> positionMeters);

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
	};
	Array<ZoneView> zones;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingMonitorPanel)
};
