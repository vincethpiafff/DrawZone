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

	//Top view above the table, the table keeps at least a few rows
	const int minTableHeight = lineHeight * 5;
	if (r.getHeight() > minTableHeight + 80)
	{
		paintTopView(g, r.removeFromTop(jmax(80, (int)(r.getHeight() * .6f))));
		r.removeFromTop(8);
	}

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

void TrackingMonitorPanel::paintTopView(Graphics& g, Rectangle<int> r)
{
	Point<float> size = snapshot.sceneSize;
	if (size.x <= 0 || size.y <= 0)
	{
		g.setColour(TEXT_COLOR.withAlpha(.5f));
		g.drawFittedText("Waiting for scene size (/scene)", r, Justification::centred, 1);
		return;
	}

	//room for the meter labels on the top and left edges
	const int labelSize = 16;
	Rectangle<float> area = r.withTrimmedTop(labelSize).withTrimmedLeft(labelSize * 2).reduced(2).toFloat();

	//keep the scene proportions, origin top left like Augmenta
	float scale = jmin(area.getWidth() / size.x, area.getHeight() / size.y);
	Rectangle<float> scene(area.getX(), area.getY(), size.x * scale, size.y * scale);

	g.setColour(BG_COLOR.darker(.3f));
	g.fillRect(scene);

	//1 m grid, labels thinned out when the meters get too small on screen
	int labelStep = 1;
	while (labelStep * scale < 24) labelStep *= 2;

	g.setFont(Font(11.0f));
	for (int i = 0; i <= (int)size.x; i++)
	{
		float x = scene.getX() + i * scale;
		g.setColour(NORMAL_COLOR.withAlpha(.35f));
		g.drawVerticalLine((int)x, scene.getY(), scene.getBottom());
		if (i % labelStep == 0)
		{
			g.setColour(TEXT_COLOR.withAlpha(.6f));
			g.drawText(String(i), Rectangle<float>(x - 15, scene.getY() - labelSize, 30, (float)labelSize), Justification::centred);
		}
	}

	for (int i = 0; i <= (int)size.y; i++)
	{
		float y = scene.getY() + i * scale;
		g.setColour(NORMAL_COLOR.withAlpha(.35f));
		g.drawHorizontalLine((int)y, scene.getX(), scene.getRight());
		if (i % labelStep == 0)
		{
			g.setColour(TEXT_COLOR.withAlpha(.6f));
			g.drawText(String(i), Rectangle<float>(scene.getX() - labelSize * 2, y - 8, (float)labelSize * 2 - 4, 16), Justification::centredRight);
		}
	}

	g.setColour(NORMAL_COLOR);
	g.drawRect(scene, 1);

	//People
	const float radius = 6;
	g.setFont(Font(12.0f, Font::bold));
	for (auto& p : snapshot.persons)
	{
		Point<float> pos(scene.getX() + p.position.x * scene.getWidth(), scene.getY() + p.position.y * scene.getHeight());
		Colour c = Colour::fromHSV((p.id * .13f) - (int)(p.id * .13f), .7f, 1, 1);

		g.setColour(c);
		g.fillEllipse(Rectangle<float>(radius * 2, radius * 2).withCentre(pos));
		g.drawText(String(p.id), Rectangle<float>(pos.x + radius + 2, pos.y - 8, 40, 16), Justification::centredLeft);
	}
}
