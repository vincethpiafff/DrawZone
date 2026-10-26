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
	void timerCallback() override;

private:
	SpatialModule* getSpatialModule();
	void paintTopView(Graphics& g, Rectangle<int> r);

	//copied on each refresh, so painting never touches the network data
	bool hasModule = false;
	String moduleName;
	int localPort = 0;
	TrackingData::Snapshot snapshot;

	struct ZoneView
	{
		String name;
		Rectangle<float> bounds;	// meters
		Colour color;
		bool enabled = true;
		bool presence = false;
	};
	Array<ZoneView> zones;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingMonitorPanel)
};
