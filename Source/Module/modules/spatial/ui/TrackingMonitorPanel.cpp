/*
  ==============================================================================

	TrackingMonitorPanel.cpp
	Created: 26 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

//dark "show" look of the Tracking Monitor
namespace TrackingMonitorStyle
{
	const Colour panelBg(0xff141414);
	const Colour sceneBg(0xff0c0c0c);
	const Colour columnBg(0xff1a1a1a);
	const Colour rowBg(0xff222222);
	const Colour borderColor(0xff333333);
	const Colour dimText(0xff9a9a9a);
	const String sep = String(CharPointer_UTF8(" \xc2\xb7 "));
	const String times = String(CharPointer_UTF8(" \xc3\x97 "));

	//soft halo around an occupied zone
	inline void drawGlow(Graphics& g, Rectangle<float> r, Colour c, bool ellipse)
	{
		for (int i = 1; i <= 4; i++)
		{
			g.setColour(c.withAlpha(.16f / i));
			if (ellipse) g.drawEllipse(r.expanded(i * 2.0f), 2.0f);
			else g.drawRect(r.expanded(i * 2.0f), 2.0f);
		}
	}

	inline void drawGlow(Graphics& g, const Path& p, Colour c)
	{
		for (int i = 1; i <= 4; i++)
		{
			g.setColour(c.withAlpha(.16f / i));
			g.strokePath(p, PathStrokeType(i * 4.0f, PathStrokeType::curved));
		}
	}
}

TrackingMonitorPanel::TrackingMonitorPanel(const String& contentName) :
	ShapeShifterContentComponent(contentName)
{
	collapseButton.setTooltip("Collapse / expand the Zones and People column");
	collapseButton.onClick = [this]()
	{
		columnCollapsed = !columnCollapsed;
		resized();
		repaint();
	};
	addChildComponent(collapseButton);

	createZoneButton.setTooltip("Create a zone at the center of the scene");
	createZoneButton.onClick = [this]() { createZone(); };
	addChildComponent(createZoneButton);

	showHiddenButton.setTooltip("Show the zones hidden in this view");
	showHiddenButton.onClick = [this]() { showHiddenZones(); };
	addChildComponent(showHiddenButton);

	renameEditor.onReturnKey = [this]() { closeRenameEditor(true); };
	renameEditor.onFocusLost = [this]() { closeRenameEditor(true); };
	renameEditor.onEscapeKey = [this]() { closeRenameEditor(false); };
	addChildComponent(renameEditor);

	setWantsKeyboardFocus(true); //Escape / Backspace while drawing a free shape

	startTimerHz(refreshRateHz);
}

void TrackingMonitorPanel::resized()
{
	Rectangle<int> r = getLocalBounds();
	columnRect = r.removeFromRight(columnCollapsed ? railWidth : columnWidth);

	r.reduce(8, 8);
	headerRect = r.removeFromTop(24);
	r.removeFromTop(8);

	//not enough room : no top view, the column still shows zones and people
	topViewRect = r.getWidth() > 80 && r.getHeight() > 80 ? r : Rectangle<int>();

	layoutColumn();
}

void TrackingMonitorPanel::layoutColumn()
{
	zoneRowRects.clearQuick();

	if (columnCollapsed)
	{
		//narrow rail : one square per visible zone, then the people as dots
		Rectangle<int> c = columnRect.reduced(6, 8);
		collapseButton.setButtonText(CharPointer_UTF8("\xc2\xab"));
		collapseButton.setBounds(c.removeFromTop(26));
		c.removeFromTop(8);
		zonesHeaderRect = c.removeFromTop(14);
		c.removeFromTop(4);

		for (auto& z : zones)
		{
			if (z.hidden || c.getHeight() < 40)
			{
				zoneRowRects.add(Rectangle<int>());
				continue;
			}
			zoneRowRects.add(c.removeFromTop(34));
			c.removeFromTop(6);
		}

		createZoneButton.setButtonText("+");
		createZoneButton.setBounds(c.removeFromTop(24));
		c.removeFromTop(4);
		showHiddenButton.setButtonText(String(numHidden) + " hid.");
		showHiddenButton.setBounds(numHidden > 0 ? c.removeFromTop(24) : Rectangle<int>());
		c.removeFromTop(12);
		peopleRect = c;
	}
	else
	{
		Rectangle<int> c = columnRect.reduced(10, 8);
		Rectangle<int> header = c.removeFromTop(22);
		collapseButton.setButtonText(CharPointer_UTF8("\xc2\xbb"));
		collapseButton.setBounds(header.removeFromRight(28));
		zonesHeaderRect = header;
		c.removeFromTop(4);

		for (int i = 0; i < zones.size(); i++)
		{
			//keep room for the buttons under the list
			if (c.getHeight() < 28 + 4 + 26)
			{
				zoneRowRects.add(Rectangle<int>());
				continue;
			}
			zoneRowRects.add(c.removeFromTop(28));
			c.removeFromTop(4);
		}

		Rectangle<int> buttons = c.removeFromTop(26);
		createZoneButton.setButtonText("+ Create zone");
		if (numHidden > 0)
		{
			showHiddenButton.setButtonText("Show hidden (" + String(numHidden) + ")");
			showHiddenButton.setBounds(buttons.removeFromRight(buttons.getWidth() / 2).withTrimmedLeft(3));
			buttons.removeFromRight(3);
		}
		createZoneButton.setBounds(buttons);
		c.removeFromTop(16);
		peopleRect = c;
	}

	collapseButton.setVisible(hasModule);
	createZoneButton.setVisible(hasModule);
	showHiddenButton.setVisible(hasModule && numHidden > 0);
}

TrackingMonitorPanel::~TrackingMonitorPanel()
{
	stopTimer();
	if (zoneColorUI != nullptr && zoneColorUI->colorEditor != nullptr) zoneColorUI->colorEditor->dismiss();
}

SpatialModule* TrackingMonitorPanel::getSpatialModule()
{
	for (auto& m : ModuleManager::getInstance()->items)
	{
		if (SpatialModule* sm = dynamic_cast<SpatialModule*>(m)) return sm;
	}
	return nullptr;
}

Colour TrackingMonitorPanel::getPersonColor(int id)
{
	return Colour::fromHSV((id * .13f) - (int)(id * .13f), .7f, 1, 1);
}

void TrackingMonitorPanel::timerCallback()
{
	SpatialModule* sm = getSpatialModule();
	hasModule = sm != nullptr;

	zones.clearQuick();
	personZones.clearQuick();
	personRelativePositions.clearQuick();

	if (hasModule)
	{
		moduleName = sm->niceName;
		localPort = sm->localPort != nullptr ? sm->localPort->intValue() : 0;
		snapshot = sm->trackingData.getSnapshot();

		for (auto& z : sm->zoneManager.items)
		{
			ZoneView zv;
			zv.zone = z;
			zv.name = z->niceName;
			Point<float> size = z->size->getPoint();
			zv.bounds = Rectangle<float>(size.x, size.y).withCentre(z->center->getPoint());
			zv.ellipse = z->isEllipse();
			zv.freeShape = z->isFreeShape();
			if (zv.freeShape)
			{
				zv.outline = z->getOutline();
				zv.points = z->getFreePointsMeters();
				for (int pi = 0; pi < z->freePoints.size(); pi++)
				{
					zv.interpolations.add((int)z->freePoints[pi]->getInterpolation());
					zv.handlesIn.add(z->getHandleMeters(pi, false));
					zv.handlesOut.add(z->getHandleMeters(pi, true));
				}
			}
			zv.color = z->itemColor->getColor();
			zv.enabled = z->enabled->boolValue();
			zv.presence = z->presence->boolValue();
			zv.hidden = z->hiddenInMonitor->boolValue();
			zv.selected = z->isSelected;
			zv.personCount = z->personCount->intValue();
			zones.add(zv);
		}

		//first enabled zone containing each person, same test as the zones themselves
		for (auto& p : snapshot.persons)
		{
			int zoneIndex = -1;
			for (int i = 0; i < sm->zoneManager.items.size(); i++)
			{
				SpatialZone* z = sm->zoneManager.items[i];
				if (z->idsInside.contains(p.id)) //same membership as the zone (with its anti-flicker)
				{
					zoneIndex = i;
					break;
				}
			}
			personZones.add(zoneIndex);

			//relative position computed by the module (zone or zones overlap)
			auto it = sm->people.find(p.id);
			personRelativePositions.add(it != sm->people.end() ? it->second->relativePosition->getPoint() : Point<float>(-1, -1));
		}
	}
	else
	{
		snapshot = TrackingData::Snapshot();
	}

	updateMotions();

	numHidden = 0;
	for (auto& z : zones) if (z.hidden) numHidden++;

	layoutColumn();
	repaint();
}

void TrackingMonitorPanel::updateMotions()
{
	uint32 now = Time::getMillisecondCounter();
	std::map<int, Motion> updated; //people who left are dropped

	for (auto& p : snapshot.persons)
	{
		Point<float> meters(p.position.x * snapshot.sceneSize.x, p.position.y * snapshot.sceneSize.y);

		Motion m;
		auto it = motions.find(p.id);
		if (it == motions.end())
		{
			m.lastPosition = meters;
			m.lastMs = now;
		}
		else
		{
			m = it->second;
			float dt = (now - m.lastMs) / 1000.0f;
			if (dt >= .05f) //timerCallback is also called while dragging a zone, keep a meaningful time step
			{
				Point<float> v = (meters - m.lastPosition) / dt;
				m.velocity = m.velocity * .6f + v * .4f;
				m.lastPosition = meters;
				m.lastMs = now;
			}
		}

		//trail sampled at a fixed step, whatever the refresh calls
		if (m.trail.isEmpty() || now - m.lastTrailMs >= (uint32)trailStepMs)
		{
			m.trail.add(meters);
			if (m.trail.size() > trailLength) m.trail.remove(0);
			m.lastTrailMs = now;
		}

		updated[p.id] = m;
	}

	motions = updated;
}

void TrackingMonitorPanel::paint(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	g.fillAll(panelBg);
	sceneScale = 0; //set again by paintTopView if the top view is visible

	if (!hasModule)
	{
		g.setColour(TEXT_COLOR.withAlpha(.6f));
		g.setFont(Font(14.0f));
		g.drawFittedText("No Spatial module. Add one from Modules > DrawZone > Spatial.", getLocalBounds().reduced(8), Justification::centred, 3);
		return;
	}

	paintHeader(g);
	if (!topViewRect.isEmpty()) paintTopView(g, topViewRect);

	g.setColour(columnBg);
	g.fillRect(columnRect);
	g.setColour(Colour(0xff2c2c2c));
	g.drawVerticalLine(columnRect.getX(), (float)columnRect.getY(), (float)columnRect.getBottom());

	if (columnCollapsed) paintRail(g);
	else paintColumn(g);
}

void TrackingMonitorPanel::paintHeader(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	Rectangle<int> r = headerRect;

	uint32 now = Time::getMillisecondCounter();
	bool received = snapshot.lastMessageMs != 0;
	bool silent = !received || now - snapshot.lastMessageMs > SpatialModule::sourceTimeoutMs;

	String state = !received ? "No data yet" : (silent ? "Silent source" : "Receiving");
	Colour stateColor = !received ? NORMAL_COLOR : (silent ? Colours::orange : Colours::limegreen);

	//source state pill
	Font pillFont(13.0f);
	String pillText = moduleName + sep + state;
	int pillWidth = pillFont.getStringWidth(pillText) + 32;
	Rectangle<float> pill = r.removeFromLeft(pillWidth).toFloat().reduced(0, 1);
	g.setColour(stateColor.withAlpha(.12f));
	g.fillRoundedRectangle(pill, pill.getHeight() / 2);
	g.setColour(stateColor.withAlpha(.6f));
	g.drawRoundedRectangle(pill, pill.getHeight() / 2, 1);
	g.setColour(stateColor);
	g.fillEllipse(Rectangle<float>(8, 8).withCentre(Point<float>(pill.getX() + 12, pill.getCentreY())));
	g.setFont(pillFont);
	g.setColour(stateColor.interpolatedWith(Colours::white, .4f));
	g.drawText(pillText, pill.withTrimmedLeft(22), Justification::centredLeft);
	r.removeFromLeft(10);

	//people count on the right
	g.setColour(dimText);
	g.setFont(Font(13.0f));
	g.drawText("people", r.removeFromRight(46), Justification::centredLeft);
	g.setColour(TEXT_COLOR);
	g.setFont(Font(18.0f, Font::bold));
	g.drawText(String(snapshot.persons.size()), r.removeFromRight(40), Justification::centredRight);
	r.removeFromRight(6);

	g.setColour(dimText);
	g.setFont(Font(13.0f));
	g.drawText(String(snapshot.messagesPerSecond, 0) + " msg/s" + sep + "Augmenta :" + String(localPort)
		+ sep + String(snapshot.sceneSize.x, 1) + times + String(snapshot.sceneSize.y, 1) + " m", r, Justification::centredLeft, true);
}

void TrackingMonitorPanel::paintTopView(Graphics& g, Rectangle<int> r)
{
	using namespace TrackingMonitorStyle;

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

	g.setColour(sceneBg);
	g.fillRect(scene);

	//1 m grid, labels thinned out when the meters get too small on screen
	int labelStep = 1;
	while (labelStep * scale < 24) labelStep *= 2;

	g.setFont(Font(11.0f));
	for (int i = 0; i <= (int)size.x; i++)
	{
		float x = scene.getX() + i * scale;
		g.setColour(NORMAL_COLOR.withAlpha(.18f));
		g.drawVerticalLine((int)x, scene.getY(), scene.getBottom());
		if (i % labelStep == 0)
		{
			g.setColour(TEXT_COLOR.withAlpha(.45f));
			g.drawText(String(i), Rectangle<float>(x - 15, scene.getY() - labelSize, 30, (float)labelSize), Justification::centred);
		}
	}

	for (int i = 0; i <= (int)size.y; i++)
	{
		float y = scene.getY() + i * scale;
		g.setColour(NORMAL_COLOR.withAlpha(.18f));
		g.drawHorizontalLine((int)y, scene.getX(), scene.getRight());
		if (i % labelStep == 0)
		{
			g.setColour(TEXT_COLOR.withAlpha(.45f));
			g.drawText(String(i), Rectangle<float>(scene.getX() - labelSize * 2, y - 8, (float)labelSize * 2 - 4, 16), Justification::centredRight);
		}
	}

	g.setColour(borderColor);
	g.drawRect(scene, 1);

	//Zones, under the people, glowing when someone is inside
	for (int i = 0; i < zones.size(); i++)
	{
		auto& z = zones.getReference(i);
		if (z.hidden) continue;

		//only draw the part inside the scene : huge coordinates crash the renderer
		Rectangle<float> zr = toScreen(z.bounds).getIntersection(scene.expanded(2));
		if (zr.isEmpty()) continue;

		//an ellipse is drawn whole (its bounds stay near the scene), clipped to the scene
		Rectangle<float> er = toScreen(z.bounds);
		Path sp = z.outline; //free shape, meters to screen
		sp.applyTransform(AffineTransform::scale(scale).translated(scene.getX(), scene.getY()));
		Graphics::ScopedSaveState clipState(g);
		g.reduceClipRegion(scene.expanded(2).getSmallestIntegerContainer());

		Colour c = z.enabled ? z.color : NORMAL_COLOR;
		Colour light = c.interpolatedWith(Colours::white, .5f);

		if (z.presence)
		{
			if (z.freeShape) drawGlow(g, sp, c);
			else drawGlow(g, z.ellipse ? er : zr, c, z.ellipse);
			g.setColour(c.withAlpha(.22f));
			if (z.freeShape) g.fillPath(sp);
			else if (z.ellipse) g.fillEllipse(er);
			else g.fillRect(zr);
		}

		g.setColour(z.selected ? HIGHLIGHT_COLOR : c.withAlpha(z.presence ? 1 : .6f));
		float lineWidth = z.presence || z.selected ? 2.0f : 1.0f;
		if (z.freeShape) g.strokePath(sp, PathStrokeType(lineWidth, PathStrokeType::curved));
		else if (z.ellipse) g.drawEllipse(er, lineWidth);
		else g.drawRect(zr, lineWidth);

		g.setColour(z.enabled ? light : NORMAL_COLOR);
		g.setFont(Font(12.0f));
		g.drawText(z.name, zr.reduced(5, 3).withHeight(16), Justification::topLeft, true);

		//person count, large when the zone is big enough on screen
		if (zr.getHeight() > 50 && zr.getWidth() > 40)
		{
			g.setColour(z.presence ? light : TEXT_COLOR.withAlpha(.25f));
			g.setFont(Font(jmin(30.0f, zr.getHeight() / 3), Font::bold));
			g.drawText(String(z.personCount), zr.reduced(6, 2), Justification::bottomRight);
		}

		//resize handles on the selected zone, the zone under the mouse or the one being edited
		bool isDragged = dragZone != nullptr && dragZone.get() == z.zone.get();
		if (i == hoveredZone || isDragged || z.selected)
		{
			g.setColour(z.selected ? HIGHLIGHT_COLOR : c);
			if (z.ellipse || z.freeShape) g.drawRect(zr, 1.0f); //the handles are on this box
			Rectangle<float> h(handleSize, handleSize);
			g.fillRect(h.withCentre(zr.getTopLeft()));
			g.fillRect(h.withCentre(zr.getTopRight()));
			g.fillRect(h.withCentre(zr.getBottomRight()));
			g.fillRect(h.withCentre(zr.getBottomLeft()));

			//curve handles of the selected free shape : line from the point, small diamond at the end
			if (z.selected)
			{
				for (int pi = 0; pi < z.points.size() && pi < z.interpolations.size(); pi++)
				{
					if (z.interpolations[pi] == ZonePoint::LINEAR) continue;
					Point<float> ps = scene.getTopLeft() + z.points[pi] * scale;
					for (auto& hm : { z.handlesIn[pi], z.handlesOut[pi] })
					{
						Point<float> hs = scene.getTopLeft() + hm * scale;
						g.setColour(HIGHLIGHT_COLOR.withAlpha(.6f));
						g.drawLine(Line<float>(ps, hs), 1.0f);
						Path diamond;
						diamond.addRectangle(Rectangle<float>(handleSize - 1, handleSize - 1).withCentre(Point<float>()));
						diamond.applyTransform(AffineTransform::rotation(MathConstants<float>::pi / 4).translated(hs.x, hs.y));
						g.setColour(HIGHLIGHT_COLOR);
						g.fillPath(diamond);
					}
				}
			}

			//free shape points : round handles, moved one by one (filled when the point is a curve)
			for (int pi = 0; pi < z.points.size(); pi++)
			{
				auto& pm = z.points.getReference(pi);
				bool curve = pi < z.interpolations.size() && z.interpolations[pi] != ZonePoint::LINEAR;
				Rectangle<float> ph = h.expanded(1).withCentre(scene.getTopLeft() + pm * scale);
				if (curve)
				{
					g.setColour(z.selected ? HIGHLIGHT_COLOR : c);
					g.fillEllipse(ph);
					continue;
				}
				g.setColour(sceneBg);
				g.fillEllipse(ph);
				g.setColour(z.selected ? HIGHLIGHT_COLOR : c);
				g.drawEllipse(ph, 2.0f);
			}
		}
	}

	if (drawingFreeShape) paintFreeShapeDrawing(g);
	if (drawingBoxShape >= 0) paintBoxDrawing(g);

	//Trails : last 2 s of each person, fading toward the oldest point
	for (auto& p : snapshot.persons)
	{
		auto it = motions.find(p.id);
		if (it == motions.end() || it->second.trail.size() < 2) continue;

		auto& trail = it->second.trail;
		Colour c = getPersonColor(p.id);
		for (int i = 1; i < trail.size(); i++)
		{
			Point<float> a = scene.getTopLeft() + trail[i - 1] * scale;
			Point<float> b = scene.getTopLeft() + trail[i] * scale;
			float age = (float)i / trail.size(); //0 = oldest, 1 = newest
			g.setColour(c.withAlpha(.05f + .45f * age));
			g.drawLine(Line<float>(a, b), 1.0f + 2.0f * age);
		}
	}

	//People : dot with a halo and the id, arrow toward where they go when moving
	g.setFont(Font(13.0f, Font::bold));
	for (auto& p : snapshot.persons)
	{
		Point<float> pos(scene.getX() + p.position.x * scene.getWidth(), scene.getY() + p.position.y * scene.getHeight());
		Colour c = getPersonColor(p.id);

		auto it = motions.find(p.id);
		if (it != motions.end())
		{
			Point<float> v = it->second.velocity;
			float speed = v.getDistanceFromOrigin();
			if (speed > minArrowSpeed)
			{
				//length = distance covered in 0.6 s, kept readable
				Point<float> dir = v / speed;
				float length = jlimit(14.0f, 60.0f, speed * .6f * scale);
				Point<float> start = pos + dir * 10.0f;
				g.setColour(c);
				g.drawArrow(Line<float>(start, start + dir * length), 2.0f, 9.0f, 9.0f);
			}
		}

		g.setColour(c.withAlpha(.18f));
		g.fillEllipse(Rectangle<float>(28, 28).withCentre(pos));
		g.setColour(c);
		g.fillEllipse(Rectangle<float>(14, 14).withCentre(pos));
		g.drawText(String(p.id), Rectangle<float>(pos.x + 10, pos.y - 20, 40, 16), Justification::centredLeft);
	}
}

void TrackingMonitorPanel::paintColumn(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	int numOccupied = 0;
	for (auto& z : zones) if (z.presence) numOccupied++;

	g.setFont(Font(11.0f));
	g.setColour(dimText);
	g.drawText("ZONES", zonesHeaderRect, Justification::centredLeft);
	g.drawText(String(zones.size()) + sep + String(numOccupied) + " occupied", zonesHeaderRect.withTrimmedRight(6), Justification::centredRight);

	for (int i = 0; i < zones.size() && i < zoneRowRects.size(); i++)
	{
		Rectangle<int> row = zoneRowRects[i];
		if (row.isEmpty()) continue;

		auto& z = zones.getReference(i);
		Colour c = z.enabled ? z.color : NORMAL_COLOR;
		Rectangle<float> rf = row.toFloat();

		if (!z.hidden)
		{
			g.setColour(z.selected ? HIGHLIGHT_COLOR.withAlpha(.12f) : rowBg);
			g.fillRoundedRectangle(rf, 3);
		}
		if (z.selected)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawRoundedRectangle(rf.reduced(.5f), 3, 1);
		}

		//filled glowing dot when occupied, ring when empty
		Rectangle<float> dot = Rectangle<float>(10, 10).withCentre(Point<float>(rf.getX() + 13, rf.getCentreY()));
		if (z.presence && !z.hidden)
		{
			g.setColour(c.withAlpha(.3f));
			g.fillEllipse(dot.expanded(3));
			g.setColour(c);
			g.fillEllipse(dot);
		}
		else
		{
			g.setColour(z.hidden ? NORMAL_COLOR : c);
			g.drawEllipse(dot, z.hidden ? 1.0f : 2.0f);
		}

		Rectangle<int> textR = row.withTrimmedLeft(26).withTrimmedRight(10);
		g.setFont(Font(13.0f, z.hidden ? Font::italic : Font::plain));
		g.setColour(z.hidden || !z.enabled ? dimText : TEXT_COLOR);
		g.drawText(z.hidden ? z.name + " (hidden)" : z.name, textR.withTrimmedRight(30), Justification::centredLeft, true);

		g.setFont(Font(13.0f, z.presence ? Font::bold : Font::plain));
		g.setColour(z.presence ? c.interpolatedWith(Colours::white, .5f) : dimText);
		g.drawText(String(z.personCount), textR, Justification::centredRight);
	}

	//People
	Rectangle<int> r = peopleRect;
	if (r.getHeight() < 40) return;

	g.setFont(Font(11.0f));
	g.setColour(dimText);
	Rectangle<int> header = r.removeFromTop(18);
	g.drawText("PEOPLE", header, Justification::centredLeft);
	g.drawText(String(snapshot.persons.size()), header.withTrimmedRight(6), Justification::centredRight);
	r.removeFromTop(2);

	Rectangle<int> columns = r.removeFromTop(16).reduced(8, 0);
	g.drawText("id", columns.removeFromLeft(36), Justification::centredLeft);
	g.drawText("x, y (m)", columns.removeFromLeft(columns.getWidth() / 3), Justification::centredLeft);
	g.drawText("relative", columns.removeFromLeft(columns.getWidth() / 2), Justification::centredLeft);
	g.drawText("zone", columns, Justification::centredLeft);
	r.removeFromTop(2);

	g.setFont(Font(13.0f));
	for (int i = 0; i < snapshot.persons.size(); i++)
	{
		if (r.getHeight() < 26) break;
		auto& p = snapshot.persons.getReference(i);

		Rectangle<int> row = r.removeFromTop(26);
		r.removeFromTop(3);
		g.setColour(rowBg);
		g.fillRoundedRectangle(row.toFloat(), 3);

		Rectangle<int> cells = row.reduced(8, 0);
		Rectangle<int> idR = cells.removeFromLeft(36);
		g.setColour(getPersonColor(p.id));
		g.fillEllipse(Rectangle<float>(10, 10).withCentre(Point<float>(idR.getX() + 5.0f, (float)idR.getCentreY())));
		g.setColour(TEXT_COLOR);
		g.drawText(String(p.id), idR.withTrimmedLeft(15), Justification::centredLeft);

		g.drawText(String(p.position.x * snapshot.sceneSize.x, 1) + ", " + String(p.position.y * snapshot.sceneSize.y, 1),
			cells.removeFromLeft(cells.getWidth() / 3), Justification::centredLeft);

		Point<float> rel = i < personRelativePositions.size() ? personRelativePositions[i] : Point<float>(-1, -1);
		Rectangle<int> relR = cells.removeFromLeft(cells.getWidth() / 2);
		if (rel.x < 0)
		{
			g.setColour(dimText);
			g.drawText("-", relR, Justification::centredLeft);
		}
		else
		{
			g.drawText(String(rel.x, 2) + ", " + String(rel.y, 2), relR, Justification::centredLeft);
		}

		int zoneIndex = i < personZones.size() ? personZones[i] : -1;
		if (zoneIndex >= 0 && zoneIndex < zones.size())
		{
			g.setColour(zones[zoneIndex].color.interpolatedWith(Colours::white, .5f));
			g.drawText(zones[zoneIndex].name, cells, Justification::centredLeft, true);
		}
		else
		{
			g.setColour(dimText);
			g.drawText("-", cells, Justification::centredLeft);
		}
	}

	if (snapshot.persons.isEmpty())
	{
		g.setColour(dimText);
		g.drawText("Nobody tracked", r.removeFromTop(lineHeight).reduced(8, 0), Justification::centredLeft);
	}
}

void TrackingMonitorPanel::paintRail(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	g.setFont(Font(10.0f));
	g.setColour(dimText);
	g.drawText("ZONES", zonesHeaderRect, Justification::centred);

	//one square per visible zone with its person count
	for (int i = 0; i < zones.size() && i < zoneRowRects.size(); i++)
	{
		Rectangle<int> row = zoneRowRects[i];
		if (row.isEmpty()) continue;

		auto& z = zones.getReference(i);
		Colour c = z.enabled ? z.color : NORMAL_COLOR;
		Rectangle<float> rf = row.toFloat();

		g.setColour(z.presence ? c.withAlpha(.2f) : rowBg);
		g.fillRoundedRectangle(rf, 4);
		if (z.presence)
		{
			g.setColour(c.withAlpha(.25f));
			g.drawRoundedRectangle(rf.expanded(2), 5, 2);
		}
		g.setColour(z.selected ? HIGHLIGHT_COLOR : c.withAlpha(z.presence ? 1 : .6f));
		g.drawRoundedRectangle(rf.reduced(.5f), 4, 1);

		g.setFont(Font(16.0f, z.presence ? Font::bold : Font::plain));
		g.setColour(z.presence ? c.interpolatedWith(Colours::white, .5f) : dimText);
		g.drawText(String(z.personCount), row, Justification::centred);
	}

	//people as numbered dots, ringed with the color of their zone
	Rectangle<int> r = peopleRect;
	if (r.getHeight() < 40) return;

	g.setColour(borderColor);
	g.drawHorizontalLine(r.getY(), r.getX() + 4.0f, r.getRight() - 4.0f);
	r.removeFromTop(8);
	g.setFont(Font(10.0f));
	g.setColour(dimText);
	g.drawText("PEOPLE", r.removeFromTop(14), Justification::centred);
	r.removeFromTop(6);

	int numDrawn = 0;
	for (int i = 0; i < snapshot.persons.size(); i++)
	{
		if (r.getHeight() < 32 + 16) break; //keep room for the "+N" line
		auto& p = snapshot.persons.getReference(i);

		Rectangle<float> cell = r.removeFromTop(26).toFloat().withSizeKeepingCentre(26, 26);
		r.removeFromTop(6);

		int zoneIndex = i < personZones.size() ? personZones[i] : -1;
		if (zoneIndex >= 0 && zoneIndex < zones.size())
		{
			g.setColour(zones[zoneIndex].color);
			g.drawEllipse(cell.expanded(2), 2);
		}
		g.setColour(getPersonColor(p.id));
		g.fillEllipse(cell);
		g.setColour(columnBg);
		g.setFont(Font(12.0f, Font::bold));
		g.drawText(String(p.id), cell, Justification::centred);
		numDrawn++;
	}

	if (numDrawn < snapshot.persons.size())
	{
		g.setColour(dimText);
		g.setFont(Font(12.0f));
		g.drawText("+" + String(snapshot.persons.size() - numDrawn), r.removeFromTop(16), Justification::centred);
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

		//curve handles, only shown on the selected zone
		if (zones[i].selected)
		{
			for (int p = 0; p < zones[i].points.size() && p < zones[i].interpolations.size(); p++)
			{
				if (zones[i].interpolations[p] == ZonePoint::LINEAR) continue;
				for (int side = 1; side <= 2; side++)
				{
					Point<float> hm = side == 1 ? zones[i].handlesIn[p] : zones[i].handlesOut[p];
					if ((sceneScreenRect.getTopLeft() + hm * sceneScale).getDistanceFrom(pos) > handleSize) continue;
					hit.zoneIndex = i;
					hit.pointIndex = p;
					hit.handle = side;
					return hit;
				}
			}
		}

		//free shape points first : they often sit on the corners of the box
		for (int p = 0; p < zones[i].points.size(); p++)
		{
			if ((sceneScreenRect.getTopLeft() + zones[i].points[p] * sceneScale).getDistanceFrom(pos) <= handleSize)
			{
				hit.zoneIndex = i;
				hit.pointIndex = p;
				return hit;
			}
		}

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

		bool inside = zr.contains(pos);
		if (zones[i].freeShape) //its curves can go out of the box
		{
			Path sp = zones[i].outline;
			sp.applyTransform(AffineTransform::scale(sceneScale).translated(sceneScreenRect.getX(), sceneScreenRect.getY()));
			inside = sp.contains(pos);
		}
		else if (inside && zones[i].ellipse)
		{
			Point<float> d = pos - zr.getCentre();
			float rx = zr.getWidth() / 2, ry = zr.getHeight() / 2;
			inside = rx > 0 && ry > 0 && (d.x * d.x) / (rx * rx) + (d.y * d.y) / (ry * ry) <= 1;
		}

		if (inside)
		{
			hit.zoneIndex = i;
			return hit;
		}
	}

	return hit;
}

void TrackingMonitorPanel::mouseMove(const MouseEvent& e)
{
	if (drawingFreeShape || drawingBoxShape >= 0)
	{
		drawMouse = e.position;
		setMouseCursor(MouseCursor::CrosshairCursor);
		repaint();
		return;
	}

	ZoneHit hit = hitTest(e.position);

	if (hit.zoneIndex != hoveredZone)
	{
		hoveredZone = hit.zoneIndex;
		repaint();
	}

	if (hit.pointIndex >= 0) setMouseCursor(MouseCursor::PointingHandCursor);
	else if (hit.corner == 0 || hit.corner == 2) setMouseCursor(MouseCursor::TopLeftCornerResizeCursor);
	else if (hit.corner == 1 || hit.corner == 3) setMouseCursor(MouseCursor::TopRightCornerResizeCursor);
	else if (hit.zoneIndex >= 0) setMouseCursor(MouseCursor::DraggingHandCursor);
	else setMouseCursor(MouseCursor::NormalCursor);
}

void TrackingMonitorPanel::mouseDown(const MouseEvent& e)
{
	if (drawingBoxShape >= 0)
	{
		if (e.mods.isRightButtonDown() || sceneScale <= 0 || !sceneScreenRect.contains(e.position)) return;
		boxStart = boxEnd = toMeters(e.position);
		boxDragging = true;
		return;
	}

	if (drawingFreeShape)
	{
		if (e.mods.isRightButtonDown() || sceneScale <= 0 || !sceneScreenRect.contains(e.position)) return;

		//the first click of a double click already added the last point
		if (e.getNumberOfClicks() >= 2)
		{
			finishFreeShapeDrawing();
			return;
		}

		if (drawPoints.size() >= 3 && (sceneScreenRect.getTopLeft() + drawPoints[0] * sceneScale).getDistanceFrom(e.position) <= handleSize + 2)
		{
			finishFreeShapeDrawing();
			return;
		}

		drawPoints.add(toMeters(e.position));
		repaint();
		return;
	}

	//click on a zone of the column or the rail : select it (shown in the Inspector)
	if (columnRect.contains(e.getPosition()))
	{
		for (int i = 0; i < zones.size() && i < zoneRowRects.size(); i++)
		{
			if (!zoneRowRects[i].contains(e.getPosition())) continue;
			SpatialZone* z = dynamic_cast<SpatialZone*>(zones[i].zone.get());
			if (e.mods.isRightButtonDown())
			{
				if (z != nullptr) showContextMenu(z, e.getPosition(), false); //same menu as in the top view
				return;
			}
			if (z != nullptr) z->selectThis();
			timerCallback();
			return;
		}
		return;
	}

	ZoneHit hit = hitTest(e.position);
	SpatialZone* zone = hit.zoneIndex >= 0 ? dynamic_cast<SpatialZone*>(zones[hit.zoneIndex].zone.get()) : nullptr;

	if (e.mods.isRightButtonDown())
	{
		if (zone != nullptr && hit.pointIndex >= 0 && hit.handle == 0) showPointMenu(zone, hit.pointIndex, e.getPosition());
		else showContextMenu(zone, e.getPosition(), sceneScale > 0 && sceneScreenRect.contains(e.position));
		return;
	}

	if (zone == nullptr)
	{
		//click in the view outside any zone : deselect everything
		if (topViewRect.contains(e.getPosition())) InspectableSelectionManager::mainSelectionManager->clearSelection();
		return;
	}

	zone->selectThis();

	dragZone = zone;
	dragCorner = hit.corner;
	dragStartCenter = zone->center->getPoint();
	dragStartSize = zone->size->getPoint();
	dragStartBounds = Rectangle<float>(dragStartSize.x, dragStartSize.y).withCentre(dragStartCenter);

	dragPoint = hit.pointIndex;
	dragHandle = hit.handle;
	dragStartPointsMeters = zone->getFreePointsMeters();
	dragStartPoints.clearQuick();
	dragStartHandlesIn.clearQuick();
	dragStartHandlesOut.clearQuick();
	for (auto& fp : zone->freePoints)
	{
		dragStartPoints.add(fp->position->getPoint());
		dragStartHandlesIn.add(fp->handleIn->getPoint());
		dragStartHandlesOut.add(fp->handleOut->getPoint());
	}
}

void TrackingMonitorPanel::mouseDrag(const MouseEvent& e)
{
	if (drawingBoxShape >= 0)
	{
		if (!boxDragging) return;
		boxEnd = toMeters(e.position);
		drawMouse = e.position;
		repaint();
		return;
	}

	SpatialZone* zone = dynamic_cast<SpatialZone*>(dragZone.get());
	if (zone == nullptr || sceneScale <= 0) return;

	Point<float> delta = e.getOffsetFromDragStart().toFloat() / sceneScale;

	if (dragPoint >= 0 && dragHandle > 0)
	{
		//curve handle : offset from its point, mirrored on the out handle for a Quadratic point
		if (dragPoint >= zone->freePoints.size() || dragPoint >= dragStartPointsMeters.size()) return;
		ZonePoint* fp = zone->freePoints[dragPoint];
		Point<float> start = dragHandle == 1 ? -dragStartHandlesOut[dragPoint] : dragStartHandlesOut[dragPoint];
		if (dragHandle == 1 && fp->getInterpolation() == ZonePoint::CUBIC) start = dragStartHandlesIn[dragPoint];
		Point<float> offset = start + delta;

		if (dragHandle == 2) fp->handleOut->setPoint(offset);
		else if (fp->getInterpolation() == ZonePoint::CUBIC) fp->handleIn->setPoint(offset);
		else fp->handleOut->setPoint(-offset);
	}
	else if (dragPoint >= 0)
	{
		//the box (Center + Size) follows the points
		Array<Point<float>> points = dragStartPointsMeters;
		if (dragPoint >= points.size()) return;
		Rectangle<float> scene(snapshot.sceneSize.x, snapshot.sceneSize.y);
		points.set(dragPoint, scene.getConstrainedPoint(points[dragPoint] + delta));
		zone->setFreePointsMeters(points);
	}
	else if (dragCorner < 0)
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

		//free shape : the curves are scaled with the box
		Point<float> scale(dragStartBounds.getWidth() > 0 ? r.getWidth() / dragStartBounds.getWidth() : 1,
			dragStartBounds.getHeight() > 0 ? r.getHeight() / dragStartBounds.getHeight() : 1);
		for (int i = 0; i < zone->freePoints.size() && i < dragStartHandlesIn.size(); i++)
		{
			zone->freePoints[i]->handleIn->setPoint(dragStartHandlesIn[i] * scale);
			zone->freePoints[i]->handleOut->setPoint(dragStartHandlesOut[i] * scale);
		}
	}

	timerCallback(); //refresh right away, not at the next 10 Hz tick
}

void TrackingMonitorPanel::mouseUp(const MouseEvent& e)
{
	if (drawingBoxShape >= 0)
	{
		if (boxDragging) finishBoxDrawing();
		return;
	}

	SpatialZone* zone = dynamic_cast<SpatialZone*>(dragZone.get());
	dragZone = nullptr;
	if (zone == nullptr) return;

	//one undo step for the whole drag
	Array<UndoableAction*> actions;
	if (zone->center->getPoint() != dragStartCenter) actions.add(zone->center->setUndoablePoint(dragStartCenter, zone->center->getPoint(), true));
	if (zone->size->getPoint() != dragStartSize) actions.add(zone->size->setUndoablePoint(dragStartSize, zone->size->getPoint(), true));
	if (dragStartPoints.size() == zone->freePoints.size())
	{
		for (int i = 0; i < dragStartPoints.size(); i++)
		{
			ZonePoint* fp = zone->freePoints[i];
			if (fp->position->getPoint() != dragStartPoints[i]) actions.add(fp->position->setUndoablePoint(dragStartPoints[i], fp->position->getPoint(), true));
			if (fp->handleIn->getPoint() != dragStartHandlesIn[i]) actions.add(fp->handleIn->setUndoablePoint(dragStartHandlesIn[i], fp->handleIn->getPoint(), true));
			if (fp->handleOut->getPoint() != dragStartHandlesOut[i]) actions.add(fp->handleOut->setUndoablePoint(dragStartHandlesOut[i], fp->handleOut->getPoint(), true));
		}
	}
	dragPoint = -1;
	dragHandle = 0;
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

void TrackingMonitorPanel::createZone()
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;

	sm->zoneManager.addItem(); //a new zone starts at the center of the scene
	timerCallback();
}

void TrackingMonitorPanel::showZoneColorEditor(SpatialZone* zone, Point<int> position)
{
	if (zone == nullptr) return;

	//one picker at a time : a click outside closes the open one first
	if (zoneColorUI != nullptr && zoneColorUI->colorEditor != nullptr) return;

	//the picker edits the selected items, so select this zone only
	zone->selectThis();

	//invisible ColorParameterUI at the click position : it opens the picker there
	//and makes the whole edit a single undo step when the picker closes
	zoneColorUI.reset(zone->itemColor->createColorParamUI());
	addChildComponent(zoneColorUI.get());
	zoneColorUI->setBounds(Rectangle<int>(16, 16).withCentre(position));
	zoneColorUI->showEditWindow();
}

void TrackingMonitorPanel::mouseDoubleClick(const MouseEvent& e)
{
	if (drawingFreeShape || drawingBoxShape >= 0 || Time::getMillisecondCounter() - drawFinishedMs < 500) return;

	//double click on a zone name in the column (not the rail, which shows no names)
	if (columnRect.contains(e.getPosition()))
	{
		if (columnCollapsed) return;
		for (int i = 0; i < zones.size() && i < zoneRowRects.size(); i++)
		{
			if (!zoneRowRects[i].contains(e.getPosition())) continue;
			showRenameEditor(dynamic_cast<SpatialZone*>(zones[i].zone.get()), zoneRowRects[i].withTrimmedLeft(22).withTrimmedRight(36).reduced(0, 3));
			return;
		}
		return;
	}

	//double click on the name drawn in the top left corner of a zone, same place as in paintTopView()
	ZoneHit hit = hitTest(e.position);
	if (hit.zoneIndex < 0) return;

	Rectangle<float> label = toScreen(zones[hit.zoneIndex].bounds).getIntersection(sceneScreenRect.expanded(2)).reduced(5, 3).withHeight(16);
	if (!label.expanded(0, 3).contains(e.position)) return;

	Rectangle<int> r = label.withWidth(jmax(label.getWidth(), 140.0f)).expanded(2, 3).toNearestInt();
	showRenameEditor(dynamic_cast<SpatialZone*>(zones[hit.zoneIndex].zone.get()), r.constrainedWithin(topViewRect));
}

void TrackingMonitorPanel::showRenameEditor(SpatialZone* zone, Rectangle<int> bounds)
{
	if (zone == nullptr) return;

	renameZone = zone;
	renameEditor.setFont(Font(13.0f));
	renameEditor.setText(zone->niceName, false);
	renameEditor.setBounds(bounds);
	renameEditor.setVisible(true);
	renameEditor.grabKeyboardFocus();
	renameEditor.selectAll();
}

void TrackingMonitorPanel::closeRenameEditor(bool apply)
{
	//hiding the editor makes it lose focus, which calls this again
	if (!renameEditor.isVisible()) return;
	renameEditor.setVisible(false);

	SpatialZone* zone = dynamic_cast<SpatialZone*>(renameZone.get());
	renameZone = nullptr;
	String newName = renameEditor.getText().trim();
	if (!apply || zone == nullptr || newName.isEmpty() || newName == zone->niceName) return;

	zone->setUndoableNiceName(newName);
	timerCallback();
}

void TrackingMonitorPanel::showContextMenu(SpatialZone* zone, Point<int> clickPosition, bool canCreate)
{
	PopupMenu m;
	if (zone != nullptr)
	{
		bool hidden = zone->hiddenInMonitor->boolValue(); //only possible from the column
		m.addItem(1, "Duplicate");
		m.addItem(2, "Delete");
		m.addItem(hidden ? 6 : 3, hidden ? "Show" : "Hide");
		m.addItem(5, "Set color...");
	}
	else if (canCreate)
	{
		m.addItem(4, "Create zone");
		m.addSeparator();
		m.addItem(8, "Draw rectangle");
		m.addItem(9, "Draw ellipse");
		m.addItem(7, "Draw free shape");
	}

	if (m.getNumItems() == 0) return;

	Point<float> clickMeters = sceneScale > 0 ? (clickPosition.toFloat() - sceneScreenRect.getTopLeft()) / sceneScale : Point<float>();
	WeakReference<ControllableContainer> zoneRef(zone);
	WeakReference<Component> panelRef(this);
	m.showMenuAsync(PopupMenu::Options(), [zoneRef, panelRef, clickMeters, clickPosition](int result)
		{
			SpatialZone* z = dynamic_cast<SpatialZone*>(zoneRef.get());
			TrackingMonitorPanel* p = dynamic_cast<TrackingMonitorPanel*>(panelRef.get());
			switch (result)
			{
			case 1: if (z != nullptr) z->duplicate(); break;
			case 2: if (z != nullptr) z->remove(); break;
			case 3: if (z != nullptr) z->hiddenInMonitor->setUndoableValue(false, true); break;
			case 4: if (p != nullptr) p->createZoneAt(clickMeters); break;
			case 5: if (z != nullptr && p != nullptr) p->showZoneColorEditor(z, clickPosition); break;
			case 6: if (z != nullptr) z->hiddenInMonitor->setUndoableValue(true, false); break;
			case 7: if (p != nullptr) p->startFreeShapeDrawing(); break;
			case 8: if (p != nullptr) p->startBoxDrawing(SpatialZone::RECTANGLE); break;
			case 9: if (p != nullptr) p->startBoxDrawing(SpatialZone::ELLIPSE); break;
			}
		});
}

void TrackingMonitorPanel::showPointMenu(SpatialZone* zone, int pointIndex, Point<int> clickPosition)
{
	if (zone == nullptr || !isPositiveAndBelow(pointIndex, zone->freePoints.size())) return;
	zone->selectThis();

	ZonePoint::Interpolation current = zone->freePoints[pointIndex]->getInterpolation();
	PopupMenu m;
	m.addSectionHeader(zone->freePoints[pointIndex]->niceName);
	m.addItem(1 + ZonePoint::LINEAR, "Linear", true, current == ZonePoint::LINEAR);
	m.addItem(1 + ZonePoint::QUADRATIC, "Quadratic", true, current == ZonePoint::QUADRATIC);
	m.addItem(1 + ZonePoint::CUBIC, "Cubic", true, current == ZonePoint::CUBIC);

	WeakReference<ControllableContainer> zoneRef(zone);
	WeakReference<Component> panelRef(this);
	m.showMenuAsync(PopupMenu::Options(), [zoneRef, panelRef, pointIndex](int result)
		{
			SpatialZone* z = dynamic_cast<SpatialZone*>(zoneRef.get());
			if (result <= 0 || z == nullptr || !isPositiveAndBelow(pointIndex, z->freePoints.size())) return;

			ZonePoint* fp = z->freePoints[pointIndex];
			int newInterpolation = result - 1;
			if (newInterpolation == (int)fp->getInterpolation()) return;

			Array<UndoableAction*> actions;
			String oldKey = fp->interpolation->getValueKey();
			fp->interpolation->setValueWithData(newInterpolation);
			actions.add(fp->interpolation->setUndoableValue(oldKey, fp->interpolation->getValueKey(), true));

			//first time as a curve : handles along the direction from the previous to the next point
			if (newInterpolation != ZonePoint::LINEAR && fp->handleIn->getPoint().isOrigin() && fp->handleOut->getPoint().isOrigin())
			{
				Array<Point<float>> pts = z->getFreePointsMeters();
				int n = pts.size();
				Point<float> tangent = (pts[(pointIndex + 1) % n] - pts[(pointIndex + n - 1) % n]) * .25f;
				actions.add(fp->handleIn->setUndoablePoint(Point<float>(), -tangent, true));
				actions.add(fp->handleOut->setUndoablePoint(Point<float>(), tangent, true));
			}
			else if (newInterpolation == ZonePoint::QUADRATIC)
			{
				//Quadratic mirrors the out handle : keep the in handle consistent for a later switch to Cubic
				Point<float> mirrored = -fp->handleOut->getPoint();
				if (fp->handleIn->getPoint() != mirrored) actions.add(fp->handleIn->setUndoablePoint(fp->handleIn->getPoint(), mirrored, true));
			}

			actions.removeAllInstancesOf(nullptr); //unchanged handles give no action
			UndoMaster::getInstance()->performActions("Edit " + fp->niceName + " interpolation", actions);
			if (TrackingMonitorPanel* p = dynamic_cast<TrackingMonitorPanel*>(panelRef.get())) p->timerCallback();
		});
}

Point<float> TrackingMonitorPanel::toMeters(Point<float> screen) const
{
	if (sceneScale <= 0) return Point<float>();
	return Rectangle<float>(snapshot.sceneSize.x, snapshot.sceneSize.y).getConstrainedPoint((screen - sceneScreenRect.getTopLeft()) / sceneScale);
}

void TrackingMonitorPanel::startFreeShapeDrawing()
{
	drawingBoxShape = -1;
	drawingFreeShape = true;
	drawPoints.clear();
	drawMouse = getMouseXYRelative().toFloat();
	InspectableSelectionManager::mainSelectionManager->clearSelection();
	grabKeyboardFocus();
	repaint();
}

void TrackingMonitorPanel::finishFreeShapeDrawing()
{
	drawingFreeShape = false;
	drawFinishedMs = Time::getMillisecondCounter();
	setMouseCursor(MouseCursor::NormalCursor);

	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr || drawPoints.size() < 3)
	{
		repaint();
		return;
	}

	//shape and points set before adding, so the undo / redo of the creation keeps them
	SpatialZone* z = new SpatialZone();
	z->shape->setValueWithData(SpatialZone::FREE_SHAPE);
	z->setFreePointsMeters(drawPoints);
	sm->zoneManager.addItem(z);
	z->selectThis();

	drawPoints.clear();
	timerCallback();
}

void TrackingMonitorPanel::cancelFreeShapeDrawing()
{
	drawingFreeShape = false;
	drawPoints.clear();
	setMouseCursor(MouseCursor::NormalCursor);
	repaint();
}

void TrackingMonitorPanel::startBoxDrawing(int boxShape)
{
	drawingFreeShape = false;
	drawingBoxShape = boxShape;
	boxDragging = false;
	drawMouse = getMouseXYRelative().toFloat();
	InspectableSelectionManager::mainSelectionManager->clearSelection();
	grabKeyboardFocus();
	repaint();
}

void TrackingMonitorPanel::finishBoxDrawing()
{
	int boxShape = drawingBoxShape;
	Rectangle<float> r(boxStart, boxEnd);
	drawingBoxShape = -1;
	boxDragging = false;
	drawFinishedMs = Time::getMillisecondCounter();
	setMouseCursor(MouseCursor::NormalCursor);

	//a simple click (no real drag) creates nothing
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr || r.getWidth() < .05f || r.getHeight() < .05f)
	{
		repaint();
		return;
	}

	SpatialZone* z = new SpatialZone();
	z->shape->setValueWithData(boxShape);
	z->size->setPoint(r.getWidth(), r.getHeight());
	z->center->setPoint(r.getCentre());
	sm->zoneManager.addItem(z);
	z->selectThis();

	timerCallback();
}

void TrackingMonitorPanel::cancelBoxDrawing()
{
	drawingBoxShape = -1;
	boxDragging = false;
	setMouseCursor(MouseCursor::NormalCursor);
	repaint();
}

void TrackingMonitorPanel::paintBoxDrawing(Graphics& g)
{
	if (boxDragging)
	{
		Graphics::ScopedSaveState clipState(g);
		g.reduceClipRegion(sceneScreenRect.expanded(2).getSmallestIntegerContainer());

		Rectangle<float> r = toScreen(Rectangle<float>(boxStart, boxEnd));
		g.setColour(HIGHLIGHT_COLOR.withAlpha(.15f));
		if (drawingBoxShape == SpatialZone::ELLIPSE) g.fillEllipse(r);
		else g.fillRect(r);
		g.setColour(HIGHLIGHT_COLOR);
		if (drawingBoxShape == SpatialZone::ELLIPSE)
		{
			g.drawEllipse(r, 2.0f);
			g.setColour(HIGHLIGHT_COLOR.withAlpha(.5f));
			g.drawRect(r, 1.0f);
		}
		else g.drawRect(r, 2.0f);
	}

	String what = drawingBoxShape == SpatialZone::ELLIPSE ? "ellipse" : "rectangle";
	paintDrawingHelp(g, "Press and drag to draw the " + what + TrackingMonitorStyle::sep + "Esc : cancel");
}

void TrackingMonitorPanel::paintDrawingHelp(Graphics& g, const String& text)
{
	using namespace TrackingMonitorStyle;

	Rectangle<float> helpR = sceneScreenRect.withHeight(22).reduced(4, 2);
	g.setColour(panelBg.withAlpha(.85f));
	g.fillRoundedRectangle(helpR, 3);
	g.setColour(TEXT_COLOR);
	g.setFont(Font(12.0f));
	g.drawText(text, helpR, Justification::centred, true);
}

bool TrackingMonitorPanel::keyPressed(const KeyPress& key)
{
	if (drawingBoxShape >= 0)
	{
		if (key != KeyPress::escapeKey) return false;
		cancelBoxDrawing();
		return true;
	}

	if (!drawingFreeShape) return false;

	if (key == KeyPress::escapeKey) cancelFreeShapeDrawing();
	else if (key == KeyPress::returnKey) finishFreeShapeDrawing();
	else if (key == KeyPress::backspaceKey || key == KeyPress::deleteKey)
	{
		if (!drawPoints.isEmpty()) drawPoints.removeLast();
		repaint();
	}
	else return false;

	return true;
}

void TrackingMonitorPanel::paintFreeShapeDrawing(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	Graphics::ScopedSaveState clipState(g);
	g.reduceClipRegion(sceneScreenRect.expanded(handleSize).getSmallestIntegerContainer());

	Array<Point<float>> screen;
	for (auto& pm : drawPoints) screen.add(sceneScreenRect.getTopLeft() + pm * sceneScale);

	bool mouseInScene = sceneScreenRect.contains(drawMouse);
	bool canClose = screen.size() >= 3 && screen[0].getDistanceFrom(drawMouse) <= handleSize + 2;

	//placed segments, then the next one following the mouse
	if (!screen.isEmpty())
	{
		Path p;
		p.startNewSubPath(screen[0]);
		for (int i = 1; i < screen.size(); i++) p.lineTo(screen[i]);
		g.setColour(HIGHLIGHT_COLOR);
		g.strokePath(p, PathStrokeType(2.0f));

		if (mouseInScene)
		{
			g.setColour(HIGHLIGHT_COLOR.withAlpha(.6f));
			g.drawLine(Line<float>(screen.getLast(), canClose ? screen[0] : drawMouse), 1.0f);
		}
	}

	for (int i = 0; i < screen.size(); i++)
	{
		Rectangle<float> ph = Rectangle<float>(handleSize + 2, handleSize + 2).withCentre(screen[i]);
		if (i == 0 && screen.size() >= 3) ph = ph.expanded(canClose ? 4.0f : 2.0f); //the first point closes the shape
		g.setColour(i == 0 && canClose ? HIGHLIGHT_COLOR : sceneBg);
		g.fillEllipse(ph);
		g.setColour(HIGHLIGHT_COLOR);
		g.drawEllipse(ph, 2.0f);
	}

	//help line on top of the scene
	String help = drawPoints.size() < 3 ? "Click to add points (at least 3)" : "Click the first point, double click or press Enter to close";
	help += sep + "Backspace : remove the last point" + sep + "Esc : cancel";
	paintDrawingHelp(g, help);
}
