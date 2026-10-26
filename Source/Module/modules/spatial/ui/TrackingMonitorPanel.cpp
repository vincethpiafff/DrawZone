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
	showHiddenButton.onClick = [this]() { showHiddenZones(); };
	addChildComponent(showHiddenButton);

	startTimerHz(refreshRateHz);
}

void TrackingMonitorPanel::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(8);

	headerRect = r.removeFromTop(lineHeight * 3);
	r.removeFromTop(8);

	//the table keeps a fixed height (header + 6 people), the top view takes the rest
	tableRect = r.removeFromBottom(lineHeight * 7 + 2);
	r.removeFromBottom(8);

	if (r.getHeight() > 120)
	{
		showHiddenButton.setBounds(r.removeFromBottom(24).withSizeKeepingCentre(200, 22));
		r.removeFromBottom(4);
		topViewRect = r;
	}
	else
	{
		//not enough room : no top view, the table gets everything
		topViewRect = Rectangle<int>();
		showHiddenButton.setBounds(Rectangle<int>());
		tableRect = tableRect.withTop(headerRect.getBottom() + 8);
	}
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

		zones.clearQuick();
		for (auto& z : sm->zoneManager.items)
		{
			ZoneView zv;
			zv.zone = z;
			zv.name = z->niceName;
			Point<float> size = z->size->getPoint();
			zv.bounds = Rectangle<float>(size.x, size.y).withCentre(z->center->getPoint());
			zv.color = z->itemColor->getColor();
			zv.enabled = z->enabled->boolValue();
			zv.presence = z->presence->boolValue();
			zv.hidden = z->hiddenInMonitor->boolValue();
			zones.add(zv);
		}
	}
	else
	{
		snapshot = TrackingData::Snapshot();
		zones.clear();
	}

	int numHidden = 0;
	for (auto& z : zones) if (z.hidden) numHidden++;
	showHiddenButton.setButtonText("Show hidden zones (" + String(numHidden) + ")");
	showHiddenButton.setVisible(numHidden > 0 && !topViewRect.isEmpty());

	repaint();
}

void TrackingMonitorPanel::paint(Graphics& g)
{
	g.fillAll(BG_COLOR);
	sceneScale = 0; //set again by paintTopView if the top view is visible

	if (!hasModule)
	{
		g.setColour(TEXT_COLOR.withAlpha(.6f));
		g.setFont(Font(14.0f));
		g.drawFittedText("No Spatial module. Add one from Modules > DrawZone > Spatial.", getLocalBounds().reduced(8), Justification::centred, 3);
		return;
	}

	Rectangle<int> r = headerRect;

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

	if (!topViewRect.isEmpty()) paintTopView(g, topViewRect);

	//Table
	r = tableRect;
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

	//room for the meter labels on the top and left edges (kept on both sides so the scene stays centered)
	const int labelSize = 16;
	Rectangle<float> area = r.toFloat().reduced(labelSize * 2.0f, (float)labelSize);

	//keep the scene proportions, origin top left like Augmenta, centered in the panel
	float scale = jmin(area.getWidth() / size.x, area.getHeight() / size.y);
	Rectangle<float> scene = Rectangle<float>(size.x * scale, size.y * scale).withCentre(area.getCentre());
	sceneScreenRect = scene;
	sceneScale = scale;

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

	//Zones, under the people, filled when someone is inside
	g.setFont(Font(11.0f));
	for (int i = 0; i < zones.size(); i++)
	{
		auto& z = zones.getReference(i);
		if (z.hidden) continue;

		//only draw the part inside the scene : huge coordinates crash the renderer
		Rectangle<float> zr = toScreen(z.bounds).getIntersection(scene.expanded(2));
		if (zr.isEmpty()) continue;

		Colour c = z.enabled ? z.color : NORMAL_COLOR;

		if (z.presence)
		{
			g.setColour(c.withAlpha(.35f));
			g.fillRect(zr);
		}
		g.setColour(c.withAlpha(z.enabled ? 1 : .5f));
		g.drawRect(zr, z.presence ? 2.0f : 1.0f);
		g.drawText(z.name, zr.reduced(3).withHeight(14), Justification::topLeft);

		//resize handles on the zone under the mouse or being edited
		bool isDragged = dragZone != nullptr && dragZone.get() == z.zone.get();
		if (i == hoveredZone || isDragged)
		{
			g.setColour(c);
			Rectangle<float> h(handleSize, handleSize);
			g.fillRect(h.withCentre(zr.getTopLeft()));
			g.fillRect(h.withCentre(zr.getTopRight()));
			g.fillRect(h.withCentre(zr.getBottomRight()));
			g.fillRect(h.withCentre(zr.getBottomLeft()));
		}
	}

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

Rectangle<float> TrackingMonitorPanel::toScreen(Rectangle<float> meters) const
{
	return Rectangle<float>(sceneScreenRect.getX() + meters.getX() * sceneScale, sceneScreenRect.getY() + meters.getY() * sceneScale,
		meters.getWidth() * sceneScale, meters.getHeight() * sceneScale);
}

TrackingMonitorPanel::ZoneHit TrackingMonitorPanel::hitTest(Point<float> pos) const
{
	ZoneHit hit;
	if (sceneScale <= 0) return hit;

	//last drawn = on top, so test from the end
	for (int i = zones.size() - 1; i >= 0; i--)
	{
		if (zones[i].hidden) continue;

		Rectangle<float> zr = toScreen(zones[i].bounds);
		const Point<float> corners[4] = { zr.getTopLeft(), zr.getTopRight(), zr.getBottomRight(), zr.getBottomLeft() };
		for (int c = 0; c < 4; c++)
		{
			if (corners[c].getDistanceFrom(pos) <= handleSize)
			{
				hit.zoneIndex = i;
				hit.corner = c;
				return hit;
			}
		}

		if (zr.contains(pos))
		{
			hit.zoneIndex = i;
			return hit;
		}
	}

	return hit;
}

void TrackingMonitorPanel::mouseMove(const MouseEvent& e)
{
	ZoneHit hit = hitTest(e.position);

	if (hit.zoneIndex != hoveredZone)
	{
		hoveredZone = hit.zoneIndex;
		repaint();
	}

	if (hit.corner == 0 || hit.corner == 2) setMouseCursor(MouseCursor::TopLeftCornerResizeCursor);
	else if (hit.corner == 1 || hit.corner == 3) setMouseCursor(MouseCursor::TopRightCornerResizeCursor);
	else if (hit.zoneIndex >= 0) setMouseCursor(MouseCursor::DraggingHandCursor);
	else setMouseCursor(MouseCursor::NormalCursor);
}

void TrackingMonitorPanel::mouseDown(const MouseEvent& e)
{
	ZoneHit hit = hitTest(e.position);
	SpatialZone* zone = hit.zoneIndex >= 0 ? dynamic_cast<SpatialZone*>(zones[hit.zoneIndex].zone.get()) : nullptr;

	if (e.mods.isRightButtonDown())
	{
		PopupMenu m;
		if (zone != nullptr)
		{
			m.addItem(1, "Duplicate");
			m.addItem(2, "Delete");
			m.addItem(3, "Hide");
		}
		else if (sceneScale > 0 && sceneScreenRect.contains(e.position))
		{
			m.addItem(4, "Create zone");
		}

		if (m.getNumItems() == 0) return;

		Point<float> clickMeters = (e.position - sceneScreenRect.getTopLeft()) / sceneScale;
		WeakReference<ControllableContainer> zoneRef(zone);
		WeakReference<Component> panelRef(this);
		m.showMenuAsync(PopupMenu::Options(), [zoneRef, panelRef, clickMeters](int result)
			{
				SpatialZone* z = dynamic_cast<SpatialZone*>(zoneRef.get());
				switch (result)
				{
				case 1: if (z != nullptr) z->duplicate(); break;
				case 2: if (z != nullptr) z->remove(); break;
				case 3: if (z != nullptr) z->hiddenInMonitor->setUndoableValue(false, true); break;
				case 4: if (auto* p = dynamic_cast<TrackingMonitorPanel*>(panelRef.get())) p->createZoneAt(clickMeters); break;
				}
			});
		return;
	}

	if (zone == nullptr) return;

	zone->selectThis();

	dragZone = zone;
	dragCorner = hit.corner;
	dragStartCenter = zone->center->getPoint();
	dragStartSize = zone->size->getPoint();
	dragStartBounds = Rectangle<float>(dragStartSize.x, dragStartSize.y).withCentre(dragStartCenter);
}

void TrackingMonitorPanel::mouseDrag(const MouseEvent& e)
{
	SpatialZone* zone = dynamic_cast<SpatialZone*>(dragZone.get());
	if (zone == nullptr || sceneScale <= 0) return;

	Point<float> delta = e.getOffsetFromDragStart().toFloat() / sceneScale;

	if (dragCorner < 0)
	{
		zone->center->setPoint(dragStartCenter + delta);
	}
	else
	{
		//the opposite corner stays in place, the dragged one follows the mouse
		const Point<float> corners[4] = { dragStartBounds.getTopLeft(), dragStartBounds.getTopRight(), dragStartBounds.getBottomRight(), dragStartBounds.getBottomLeft() };
		Point<float> fixedCorner = corners[(dragCorner + 2) % 4];
		Point<float> movedCorner = corners[dragCorner] + delta;

		Rectangle<float> scene(snapshot.sceneSize.x, snapshot.sceneSize.y);
		Rectangle<float> r = Rectangle<float>(fixedCorner, movedCorner).getIntersection(scene);

		zone->size->setPoint(r.getWidth(), r.getHeight());
		zone->center->setPoint(r.getCentre());
	}

	timerCallback(); //refresh right away, not at the next 10 Hz tick
}

void TrackingMonitorPanel::mouseUp(const MouseEvent& e)
{
	SpatialZone* zone = dynamic_cast<SpatialZone*>(dragZone.get());
	dragZone = nullptr;
	if (zone == nullptr) return;

	//one undo step for the whole drag
	Array<UndoableAction*> actions;
	if (zone->center->getPoint() != dragStartCenter) actions.add(zone->center->setUndoablePoint(dragStartCenter, zone->center->getPoint(), true));
	if (zone->size->getPoint() != dragStartSize) actions.add(zone->size->setUndoablePoint(dragStartSize, zone->size->getPoint(), true));
	if (!actions.isEmpty()) UndoMaster::getInstance()->performActions("Edit " + zone->niceName, actions);

	mouseMove(e);
}

void TrackingMonitorPanel::showHiddenZones()
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;

	for (auto& z : sm->zoneManager.items) z->hiddenInMonitor->setValue(false);
	timerCallback();
}

void TrackingMonitorPanel::createZoneAt(Point<float> positionMeters)
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;

	SpatialZone* z = sm->zoneManager.addItem();
	if (z == nullptr) return;

	z->center->setPoint(positionMeters);
	timerCallback();
}
