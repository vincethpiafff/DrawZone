/*
  ==============================================================================

	TrackingMonitorPanel.cpp
	Created: 26 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

TrackingMonitorPanel::TrackingMonitorPanel(const String& contentName) :
	ShapeShifterContentComponent(contentName)
{
	startTimerHz(refreshRateHz);
}

TrackingMonitorPanel::~TrackingMonitorPanel()
{
	stopTimer();
}

SpatialModule* TrackingMonitorPanel::getSpatialModule()
{
	for (auto& m : ModuleManager::getInstance()->items)
	{
		if (SpatialModule* sm = dynamic_cast<SpatialModule*>(m)) return sm;
	}
	return nullptr;
}

void TrackingMonitorPanel::timerCallback()
{
	SpatialModule* sm = getSpatialModule();
	hasModule = sm != nullptr;

	if (hasModule)
	{
		moduleName = sm->niceName;
		localPort = sm->localPort != nullptr ? sm->localPort->intValue() : 0;
		snapshot = sm->trackingData.getSnapshot();
	}
	else
	{
		snapshot = TrackingData::Snapshot();
	}

	repaint();
}

void TrackingMonitorPanel::paint(Graphics& g)
{
	g.fillAll(BG_COLOR);

	Rectangle<int> r = getLocalBounds().reduced(8);
	const int lineHeight = 18;

	if (!hasModule)
	{
		g.setColour(TEXT_COLOR.withAlpha(.6f));
		g.setFont(Font(14.0f));
		g.drawFittedText("No Spatial module. Add one from Modules > DrawZone > Spatial.", r, Justification::centred, 3);
		return;
	}

	//Header : source and state
	uint32 now = Time::getMillisecondCounter();
	bool received = snapshot.lastMessageMs != 0;
	bool silent = !received || now - snapshot.lastMessageMs > SpatialModule::sourceTimeoutMs;

	String state = !received ? "No data yet" : (silent ? "Silent source" : "Receiving");
	Colour stateColor = !received ? NORMAL_COLOR : (silent ? Colours::orange : Colours::limegreen);

	g.setFont(Font(14.0f, Font::bold));
	g.setColour(TEXT_COLOR);
	Rectangle<int> line = r.removeFromTop(lineHeight);
	g.drawText(moduleName + " (Augmenta, port " + String(localPort) + ")", line, Justification::centredLeft);

	line = r.removeFromTop(lineHeight);
	g.setColour(stateColor);
	g.fillEllipse(line.removeFromLeft(lineHeight).reduced(4).toFloat());
	g.setFont(Font(13.0f));
	g.drawText(state, line.removeFromLeft(110), Justification::centredLeft);
	g.setColour(TEXT_COLOR);
	g.drawText(String(snapshot.messagesPerSecond, 0) + " msg/s", line, Justification::centredLeft);

	line = r.removeFromTop(lineHeight);
	g.drawText("Scene : " + String(snapshot.sceneSize.x, 2) + " x " + String(snapshot.sceneSize.y, 2) + " m"
		+ "    Frame : " + String(snapshot.frame)
		+ "    People : " + String(snapshot.persons.size()), line, Justification::centredLeft);

	r.removeFromTop(8);

	//Table
	const StringArray headers = { "id", "x norm", "y norm", "x (m)", "y (m)", "age (s)", "speed" };
	const int numColumns = headers.size();
	auto getCell = [&](Rectangle<int> row, int column)
	{
		int w = row.getWidth() / numColumns;
		return Rectangle<int>(row.getX() + column * w, row.getY(), w, row.getHeight()).reduced(4, 0);
	};

	Rectangle<int> headerRow = r.removeFromTop(lineHeight + 2);
	g.setColour(BG_COLOR.brighter(.15f));
	g.fillRect(headerRow);
	g.setColour(TEXT_COLOR);
	g.setFont(Font(13.0f, Font::bold));
	for (int i = 0; i < numColumns; i++) g.drawText(headers[i], getCell(headerRow, i), Justification::centredLeft);

	g.setFont(Font(13.0f));
	int rowIndex = 0;
	for (auto& p : snapshot.persons)
	{
		if (r.getHeight() < lineHeight) break;
		Rectangle<int> row = r.removeFromTop(lineHeight);
		if (rowIndex++ % 2 == 1)
		{
			g.setColour(BG_COLOR.brighter(.05f));
			g.fillRect(row);
		}

		g.setColour(TEXT_COLOR);
		const StringArray cells = {
			String(p.id),
			String(p.position.x, 3),
			String(p.position.y, 3),
			String(p.position.x * snapshot.sceneSize.x, 2),
			String(p.position.y * snapshot.sceneSize.y, 2),
			String(p.age, 1),
			String(p.velocity.getDistanceFromOrigin(), 3)
		};
		for (int i = 0; i < numColumns; i++) g.drawText(cells[i], getCell(row, i), Justification::centredLeft);
	}

	if (snapshot.persons.isEmpty())
	{
		g.setColour(TEXT_COLOR.withAlpha(.5f));
		g.drawText("Nobody tracked", r.removeFromTop(lineHeight), Justification::centredLeft);
	}
}
