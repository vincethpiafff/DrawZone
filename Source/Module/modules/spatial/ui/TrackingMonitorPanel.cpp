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

	slotMinusButton.setButtonText("-");
	slotMinusButton.setTooltip("Remove the last player slot");
	slotMinusButton.onClick = [this]() { changeSlotCount(-1); };
	addChildComponent(slotMinusButton);

	slotPlusButton.setButtonText("+");
	slotPlusButton.setTooltip("Add a player slot");
	slotPlusButton.onClick = [this]() { changeSlotCount(1); };
	addChildComponent(slotPlusButton);

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
	shortcutsRect = headerRect.removeFromRight(104).reduced(0, 1);
	headerRect.removeFromRight(8);
	ndiRect = headerRect.removeFromRight(106).reduced(0, 1);
	headerRect.removeFromRight(8);
	spoutRect = headerRect.removeFromRight(118).reduced(0, 1);
	headerRect.removeFromRight(10);
	r.removeFromTop(8);

	slotBarRect = r.removeFromBottom(slotBarHeight);
	r.removeFromBottom(8);

	//not enough room : no top view, the column still shows zones and people
	topViewRect = r.getWidth() > 80 && r.getHeight() > 80 ? r : Rectangle<int>();

	layoutColumn();
	layoutSlotBar();
}

void TrackingMonitorPanel::layoutSlotBar()
{
	slotChipRects.clearQuick();

	Rectangle<int> b = slotBarRect.reduced(6, 6);
	slotModeRect = b.removeFromLeft(126);
	b.removeFromLeft(10);
	slotPlusButton.setBounds(b.removeFromRight(24));
	b.removeFromRight(3);
	slotMinusButton.setBounds(b.removeFromRight(24));
	b.removeFromRight(8);
	slotBarLabelRect = b.removeFromLeft(96);
	b.removeFromLeft(4);

	//chips share the width, never narrower than 72 px : the ones that do not fit are not shown
	const int gap = 6;
	int n = slots.size();
	int w = n > 0 ? jlimit(72, 170, (b.getWidth() - gap * (n - 1)) / n) : 0;
	for (int i = 0; i < n; i++)
	{
		if (b.getWidth() < w)
		{
			slotChipRects.add(Rectangle<int>());
			continue;
		}
		slotChipRects.add(b.removeFromLeft(w));
		b.removeFromLeft(gap);
	}

	slotMinusButton.setVisible(hasModule);
	slotPlusButton.setVisible(hasModule);
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
			//one card per zone, taller when unfolded ; keep room for the buttons under the list
			int h = cardHeight + (isExpanded(zones[i].zone.get()) ? numDetailLines * detailLineHeight + 6 : 0);
			if (c.getHeight() < h + 4 + 26)
			{
				zoneRowRects.add(Rectangle<int>());
				continue;
			}
			zoneRowRects.add(c.removeFromTop(h));
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

bool TrackingMonitorPanel::isExpanded(ControllableContainer* zone) const
{
	if (zone == nullptr) return false;
	for (auto& z : expandedZones) if (z.get() == zone) return true;
	return false;
}

Rectangle<int> TrackingMonitorPanel::getCardArrowRect(Rectangle<int> card)
{
	return Rectangle<int>(card.getX() + 4, card.getY(), 18, 28);
}

void TrackingMonitorPanel::timerCallback()
{
	SpatialModule* sm = getSpatialModule();
	model.update(sm);
	spoutOn = sm != nullptr && sm->videoOutput.spout->boolValue();
	ndiOn = sm != nullptr && sm->videoOutput.ndi->boolValue();

	numHidden = 0;
	for (auto& z : zones) if (z.hidden) numHidden++;

	layoutColumn();
	layoutSlotBar();
	repaint();
}

void TrackingMonitorPanel::paint(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	g.fillAll(panelBg);
	sceneScale = 0; //set again by paintTopView if the top view is visible
	personRowRects.clearQuick();
	personRowIds.clearQuick();

	if (!hasModule)
	{
		g.setColour(TEXT_COLOR.withAlpha(.6f));
		g.setFont(Font(14.0f));
		g.drawFittedText("No Spatial module. Add one from Modules > DrawZone > Spatial.", getLocalBounds().reduced(8), Justification::centred, 3);
		return;
	}

	paintHeader(g);
	if (!topViewRect.isEmpty()) paintTopView(g, topViewRect);
	paintSlotBar(g);

	g.setColour(columnBg);
	g.fillRect(columnRect);
	g.setColour(Colour(0xff2c2c2c));
	g.drawVerticalLine(columnRect.getX(), (float)columnRect.getY(), (float)columnRect.getBottom());

	if (columnCollapsed) paintRail(g);
	else paintColumn(g);

	if (slotDragging) paintSlotDrag(g);
	paintOutputChip(g, spoutRect, spoutOn, spoutHovered, "Spout Output");
	paintOutputChip(g, ndiRect, ndiOn, ndiHovered, "NDI Output");
	paintShortcuts(g); //last : the unfolded list goes over everything
}

Rectangle<int> TrackingMonitorPanel::getOutputBoxRect(Rectangle<int> chip)
{
	return Rectangle<int>(14, 14).withCentre(Point<int>(chip.getX() + 13, chip.getCentreY()));
}

void TrackingMonitorPanel::paintOutputChip(Graphics& g, Rectangle<int> chip, bool on, bool hovered, const String& label)
{
	using namespace TrackingMonitorStyle;

	Rectangle<float> cf = chip.toFloat();
	g.setColour(hovered ? rowBg.brighter(.15f) : rowBg);
	g.fillRoundedRectangle(cf, 4);
	g.setColour(on ? HIGHLIGHT_COLOR : (hovered ? TEXT_COLOR.withAlpha(.5f) : borderColor));
	g.drawRoundedRectangle(cf.reduced(.5f), 4, 1);

	//check box : filled with a tick when the output is on
	Rectangle<float> box = getOutputBoxRect(chip).toFloat();
	if (on)
	{
		g.setColour(HIGHLIGHT_COLOR);
		g.fillRoundedRectangle(box, 3);
		Path tick;
		tick.startNewSubPath(box.getX() + 3.5f, box.getCentreY());
		tick.lineTo(box.getX() + 6, box.getBottom() - 4);
		tick.lineTo(box.getRight() - 3.5f, box.getY() + 4);
		g.setColour(Colours::black.withAlpha(.8f));
		g.strokePath(tick, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
	}
	else
	{
		g.setColour(dimText);
		g.drawRoundedRectangle(box.reduced(.5f), 3, 1.2f);
	}

	g.setFont(Font(12.0f));
	g.setColour(on ? TEXT_COLOR : dimText);
	g.drawText(label, cf.withTrimmedLeft(26).withTrimmedRight(4), Justification::centredLeft, true);
}

bool TrackingMonitorPanel::handleOutputChipClick(Point<int> pos, Rectangle<int> chip, BoolParameter* output)
{
	if (!chip.contains(pos)) return false;

	if (getOutputBoxRect(chip).expanded(4).contains(pos)) output->setUndoableValue(output->boolValue(), !output->boolValue());
	else if (SpatialModule* sm = getSpatialModule()) sm->videoOutput.selectThis();
	timerCallback();
	return true;
}

Point<float> TrackingMonitorPanel::getPersonScreenPosition(int id) const
{
	if (sceneScale > 0)
	{
		for (auto& p : snapshot.persons)
			if (p.id == id) return Point<float>(sceneScreenRect.getX() + p.position.x * sceneScreenRect.getWidth(), sceneScreenRect.getY() + p.position.y * sceneScreenRect.getHeight());
	}
	for (int i = 0; i < personRowIds.size(); i++) if (personRowIds[i] == id) return personRowRects[i].getCentre().toFloat();
	return slotDragStart;
}

void TrackingMonitorPanel::paintSlotDrag(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	//drop target under the mouse : a slot chip when dragging a person, a person when dragging a chip
	int targetSlot = slotDragSlot >= 0 ? slotDragSlot : getSlotChipAt(slotDragMouse.toInt());
	int targetPerson = slotDragPersonId >= 0 ? slotDragPersonId : getPersonAt(slotDragMouse);
	bool valid = targetSlot >= 0 && targetPerson >= 0;

	Colour c = slotDragSlot >= 0 ? slots[slotDragSlot].color : (targetSlot >= 0 ? slots[targetSlot].color : getPersonColor(slotDragPersonId));
	Point<float> from = slotDragSlot >= 0 ? slotChipRects[slotDragSlot].getCentre().toFloat() : getPersonScreenPosition(slotDragPersonId);

	//link from the dragged item to the mouse
	Path link, dashed;
	link.startNewSubPath(from);
	link.lineTo(slotDragMouse);
	const float dashes[] = { 6, 4 };
	PathStrokeType(2.0f).createDashedStroke(dashed, link, dashes, 2);
	g.setColour(c.withAlpha(valid ? 1.0f : .6f));
	g.fillPath(dashed);
	g.fillEllipse(Rectangle<float>(8, 8).withCentre(slotDragMouse));

	if (!valid) return;

	//highlight the drop target
	if (slotDragSlot < 0)
	{
		g.setColour(c);
		g.drawRoundedRectangle(slotChipRects[targetSlot].toFloat().expanded(2), 5, 2);
	}
	else
	{
		g.setColour(c);
		g.drawEllipse(Rectangle<float>(34, 34).withCentre(getPersonScreenPosition(targetPerson)), 2.5f);
	}

	//what the drop will do
	String text = "Person " + String(targetPerson) + "  ->  " + slots[targetSlot].name;
	Font f(12.0f, Font::bold);
	Rectangle<float> tag = Rectangle<float>(f.getStringWidthFloat(text) + 16, 20).withPosition(slotDragMouse.translated(12, -26));
	g.setColour(c.darker(.6f).withAlpha(.9f));
	g.fillRoundedRectangle(tag, 10);
	g.setColour(c);
	g.drawRoundedRectangle(tag, 10, 1.2f);
	g.setFont(f);
	g.setColour(Colours::white);
	g.drawText(text, tag, Justification::centred, false);
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

	//meter labels around the scene, thinned out when the meters get too small on screen
	int labelStep = 1;
	while (labelStep * scale < 24) labelStep *= 2;

	g.setFont(Font(11.0f));
	g.setColour(TEXT_COLOR.withAlpha(.45f));
	for (int i = 0; i <= (int)size.x; i += labelStep)
		g.drawText(String(i), Rectangle<float>(scene.getX() + i * scale - 15, scene.getY() - labelSize, 30, (float)labelSize), Justification::centred);
	for (int i = 0; i <= (int)size.y; i += labelStep)
		g.drawText(String(i), Rectangle<float>(scene.getX() - labelSize * 2, scene.getY() + i * scale - 8, (float)labelSize * 2 - 4, 16), Justification::centredRight);

	//same drawing as the video output, plus the selection and the editing handles
	TopViewRenderer::Options o;
	o.selection = true;

	auto zoneHandles = [&](int i, Rectangle<float> zr)
	{
		auto& z = zones.getReference(i);
		Colour c = z.enabled ? z.color : NORMAL_COLOR;

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
	};

	auto drawingPreviews = [&]()
	{
		if (drawingFreeShape) paintFreeShapeDrawing(g);
		if (drawingBoxShape >= 0) paintBoxDrawing(g);
	};

	TopViewRenderer::paint(g, model, scene, scale, scene.expanded(2), o, zoneHandles, drawingPreviews);
}

void TrackingMonitorPanel::paintSlotBar(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	if (slotBarRect.isEmpty()) return;

	g.setColour(columnBg);
	g.fillRoundedRectangle(slotBarRect.toFloat(), 4);

	//SwapSlots mode chip : icon, short name, chevron
	if (!slotModeRect.isEmpty())
	{
		Rectangle<float> mf = slotModeRect.toFloat();
		g.setColour(slotModeHovered ? rowBg.brighter(.15f) : rowBg);
		g.fillRoundedRectangle(mf, 4);
		g.setColour(slotModeHovered ? TEXT_COLOR.withAlpha(.5f) : borderColor);
		g.drawRoundedRectangle(mf.reduced(.5f), 4, 1);

		Colour iconColor = slotsEnabled ? HIGHLIGHT_COLOR : NORMAL_COLOR;
		drawSlotModeIcon(g, Rectangle<float>(14, 14).withCentre(Point<float>(mf.getX() + 15, mf.getCentreY())), slotReleaseMode, iconColor);

		Rectangle<float> chevron = Rectangle<float>(8, 5).withCentre(Point<float>(mf.getRight() - 12, mf.getCentreY()));
		Path p;
		p.addTriangle(chevron.getX(), chevron.getY(), chevron.getRight(), chevron.getY(), chevron.getCentreX(), chevron.getBottom());
		g.setColour(dimText);
		g.fillPath(p);

		g.setFont(Font(12.0f, Font::bold));
		g.setColour(TEXT_COLOR);
		g.drawText(slotReleaseMode == PlayerSlots::HOLD_SPOT ? "Hold spot" : "Next person", mf.withTrimmedLeft(28).withTrimmedRight(20), Justification::centredLeft, true);
	}

	g.setFont(Font(11.0f));
	g.setColour(dimText);
	g.drawText("PLAYER SLOTS", slotBarLabelRect.withHeight(slotBarLabelRect.getHeight() / 2 + 2), Justification::bottomLeft);
	g.drawText(slotsEnabled ? String(freeSlots) + " / " + String(slots.size()) + " free" : String("Disabled"),
		slotBarLabelRect.withTrimmedTop(slotBarLabelRect.getHeight() / 2 + 2), Justification::topLeft);

	for (int i = 0; i < slots.size() && i < slotChipRects.size(); i++)
	{
		Rectangle<int> chip = slotChipRects[i];
		if (chip.isEmpty()) continue;

		auto& s = slots.getReference(i);
		bool selected = s.slot != nullptr && s.slot->isSelected;
		Colour c = slotsEnabled ? s.color : NORMAL_COLOR;
		Rectangle<float> cf = chip.toFloat();

		//taken : tinted with the slot color ; held : dashed border ; free : plain
		g.setColour(s.occupied && !s.held ? rowBg.overlaidWith(c.withAlpha(.22f)) : rowBg);
		g.fillRoundedRectangle(cf, 4);
		if (selected)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawRoundedRectangle(cf.reduced(.5f), 4, 1);
		}
		else if (s.held)
		{
			Path border, dashed;
			border.addRoundedRectangle(cf.reduced(.5f), 4);
			const float dashes[] = { 4, 3 };
			PathStrokeType(1.0f).createDashedStroke(dashed, border, dashes, 2);
			g.setColour(c.withAlpha(.8f));
			g.fillPath(dashed);
		}
		else if (s.occupied)
		{
			g.setColour(c.withAlpha(.5f));
			g.drawRoundedRectangle(cf.reduced(.5f), 4, 1);
		}

		//dot : full when taken, ring when free or held
		Rectangle<float> dot = Rectangle<float>(10, 10).withCentre(Point<float>(cf.getX() + 12, cf.getCentreY()));
		g.setColour(c);
		if (s.occupied && !s.held) g.fillEllipse(dot);
		else g.drawEllipse(dot.reduced(.5f), 1.5f);

		String state = !s.occupied ? String(CharPointer_UTF8("\xe2\x80\x94"))
			: s.held ? (s.holdRemaining < 0 ? String(CharPointer_UTF8("held \xe2\x88\x9e")) : "held " + formatTime(s.holdRemaining))
			: "#" + String(s.personId);

		Rectangle<int> text = chip.withTrimmedLeft(22).withTrimmedRight(6);
		g.setFont(Font(11.0f));
		g.setColour(s.occupied ? TEXT_COLOR : dimText);
		int stateW = jmin(text.getWidth() / 2, (int)Font(11.0f).getStringWidthFloat(state) + 4);
		g.drawText(state, text.removeFromRight(stateW), Justification::centredRight, false);
		g.setFont(Font(12.0f, Font::bold));
		g.setColour(s.occupied ? Colours::white : TEXT_COLOR);
		g.drawText(s.name, text, Justification::centredLeft, true);
	}
}

int TrackingMonitorPanel::getPersonAt(Point<float> pos) const
{
	for (int i = 0; i < personRowRects.size(); i++) if (personRowRects[i].contains(pos.toInt())) return personRowIds[i];

	if (sceneScale <= 0 || !sceneScreenRect.contains(pos)) return -1;

	//top view : the dot, a bit larger than drawn ; the last drawn (on top) first
	for (int i = snapshot.persons.size() - 1; i >= 0; i--)
	{
		auto& p = snapshot.persons.getReference(i);
		Point<float> screen(sceneScreenRect.getX() + p.position.x * sceneScreenRect.getWidth(), sceneScreenRect.getY() + p.position.y * sceneScreenRect.getHeight());
		if (screen.getDistanceFrom(pos) <= 10) return p.id;
	}
	return -1;
}

void TrackingMonitorPanel::selectPerson(int id)
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;
	auto it = sm->people.find(id);
	if (it != sm->people.end()) it->second->selectThis();
	grabKeyboardFocus(); //keys 1-9 / 0 assign its slot
	timerCallback();
}

void TrackingMonitorPanel::assignSlot(int personId, int slotIndex)
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;

	if (slotIndex < 0) sm->playerSlots.releaseByHand(personId);
	else
	{
		Point<float> meters;
		for (auto& p : snapshot.persons) if (p.id == personId) meters = Point<float>(p.position.x * snapshot.sceneSize.x, p.position.y * snapshot.sceneSize.y);
		sm->playerSlots.assignByHand(personId, meters, slotIndex);
	}
	timerCallback();
}

void TrackingMonitorPanel::showPersonMenu(int personId, Point<int> clickPosition)
{
	int current = getSlotIndexForPerson(personId);

	PopupMenu slotsMenu;
	for (int i = 0; i < slots.size(); i++)
	{
		auto& s = slots.getReference(i);
		String state = !s.occupied ? "free" : (s.held ? "held" : "#" + String(s.personId));
		String key = i < 9 ? "   [" + String(i + 1) + "]" : String();
		slotsMenu.addColouredItem(100 + i, s.name + "   " + state + key, s.color, slotsEnabled && i != current, i == current);
	}

	PopupMenu m;
	m.addSectionHeader("Person " + String(personId));
	m.addSubMenu("Assign slot", slotsMenu, slotsEnabled && !slots.isEmpty());
	m.addItem(1, "Release slot   [0]", current >= 0);

	WeakReference<Component> panelRef(this);
	m.showMenuAsync(PopupMenu::Options().withTargetScreenArea(Rectangle<int>(1, 1).withPosition(localPointToGlobal(clickPosition))), [panelRef, personId](int result)
		{
			TrackingMonitorPanel* p = dynamic_cast<TrackingMonitorPanel*>(panelRef.get());
			if (p == nullptr || result <= 0) return;
			p->assignSlot(personId, result == 1 ? -1 : result - 100);
		});
}

Rectangle<int> TrackingMonitorPanel::getShortcutsPanelRect() const
{
	return Rectangle<int>(shortcutsRect.getRight() - 340, shortcutsRect.getBottom() + 6, 340, 292);
}

void TrackingMonitorPanel::mouseExit(const MouseEvent&)
{
	if (!shortcutsHovered && !slotModeHovered && !spoutHovered && !ndiHovered) return;
	shortcutsHovered = slotModeHovered = spoutHovered = ndiHovered = false;
	repaint();
}

void TrackingMonitorPanel::paintShortcuts(Graphics& g)
{
	using namespace TrackingMonitorStyle;

	//chip : keyboard icon, label, chevron
	bool open = shortcutsPinned || shortcutsHovered;
	Rectangle<float> cf = shortcutsRect.toFloat();
	g.setColour(open ? rowBg.brighter(.15f) : rowBg);
	g.fillRoundedRectangle(cf, 4);
	g.setColour(shortcutsPinned ? HIGHLIGHT_COLOR : (open ? TEXT_COLOR.withAlpha(.5f) : borderColor));
	g.drawRoundedRectangle(cf.reduced(.5f), 4, 1);

	Rectangle<float> kb = Rectangle<float>(16, 11).withCentre(Point<float>(cf.getX() + 14, cf.getCentreY()));
	g.setColour(dimText);
	g.drawRoundedRectangle(kb, 2, 1.2f);
	for (int i = 0; i < 3; i++) g.fillRect(Rectangle<float>(kb.getX() + 3 + i * 4, kb.getY() + 3, 2, 1.5f));
	g.fillRect(Rectangle<float>(kb.getX() + 4, kb.getY() + 7, 8, 1.5f));

	g.setFont(Font(12.0f));
	g.setColour(TEXT_COLOR);
	g.drawText("Shortcuts", cf.withTrimmedLeft(28).withTrimmedRight(16), Justification::centredLeft, true);
	Rectangle<float> chevron = Rectangle<float>(8, 5).withCentre(Point<float>(cf.getRight() - 10, cf.getCentreY()));
	Path cp;
	if (open) cp.addTriangle(chevron.getX(), chevron.getBottom(), chevron.getRight(), chevron.getBottom(), chevron.getCentreX(), chevron.getY());
	else cp.addTriangle(chevron.getX(), chevron.getY(), chevron.getRight(), chevron.getY(), chevron.getCentreX(), chevron.getBottom());
	g.setColour(dimText);
	g.fillPath(cp);

	if (!open) return;

	//unfolded list, over the top view
	struct Line { const char* keys; const char* action; };
	const char* sections[] = { "PLAYER SLOTS", "ZONES", "FREE SHAPE DRAWING" };
	const Line slotLines[] = { { "Click a person", "select it (Inspector)" }, { "1 - 9  (numpad too)", "assign slot 1 - 9" }, { "0  /  Del", "release its slot" }, { "Right click a person", "assign / release slot" } };
	const Line zoneLines[] = { { "Double click a name", "rename (zone or slot)" }, { "Right click the scene", "create / draw a zone" }, { "Right click a zone", "duplicate, delete, color..." } };
	const Line drawLines[] = { { "Enter / double click", "close the shape" }, { "Backspace", "remove the last point" }, { "Esc", "cancel" } };

	Rectangle<int> panel = getShortcutsPanelRect();
	g.setColour(Colours::black.withAlpha(.35f));
	g.fillRoundedRectangle(panel.toFloat().translated(0, 3), 6);
	g.setColour(columnBg);
	g.fillRoundedRectangle(panel.toFloat(), 6);
	g.setColour(borderColor);
	g.drawRoundedRectangle(panel.toFloat().reduced(.5f), 6, 1);

	Rectangle<int> r = panel.reduced(12, 8);
	auto drawSection = [&](const char* title, const Line* lines, int numLines)
	{
		g.setFont(Font(10.0f));
		g.setColour(dimText);
		g.drawText(title, r.removeFromTop(18), Justification::centredLeft);
		for (int i = 0; i < numLines; i++)
		{
			Rectangle<int> row = r.removeFromTop(20);
			Rectangle<int> keyR = row.removeFromLeft(150);
			Font keyFont(11.0f, Font::bold);
			g.setFont(keyFont);
			Rectangle<float> keyChip = keyR.toFloat().withWidth(keyFont.getStringWidthFloat(lines[i].keys) + 12).reduced(0, 2);
			g.setColour(rowBg);
			g.fillRoundedRectangle(keyChip, 3);
			g.setColour(TEXT_COLOR);
			g.drawText(lines[i].keys, keyChip, Justification::centred, false);
			g.setFont(Font(12.0f));
			g.setColour(TEXT_COLOR.withAlpha(.8f));
			g.drawText(lines[i].action, row, Justification::centredLeft, true);
		}
		r.removeFromTop(6);
	};

	drawSection(sections[0], slotLines, 4);
	drawSection(sections[1], zoneLines, 3);
	drawSection(sections[2], drawLines, 3);
}

//one SwapSlots mode in the menu : icon, full name and a short explanation
class SlotModeMenuItem :
	public PopupMenu::CustomComponent
{
public:
	SlotModeMenuItem(int mode, bool ticked) : mode(mode), ticked(ticked) {}

	int mode;
	bool ticked;

	void getIdealSize(int& idealWidth, int& idealHeight) override
	{
		idealWidth = 300;
		idealHeight = 44;
	}

	void paint(Graphics& g) override
	{
		using namespace TrackingMonitorStyle;

		Rectangle<float> r = getLocalBounds().toFloat().reduced(3, 2);
		if (isItemHighlighted())
		{
			g.setColour(HIGHLIGHT_COLOR.withAlpha(.15f));
			g.fillRoundedRectangle(r, 4);
		}

		drawSlotModeIcon(g, Rectangle<float>(14, 14).withCentre(Point<float>(r.getX() + 16, r.getY() + 14)), mode, HIGHLIGHT_COLOR);

		Rectangle<float> text = r.withTrimmedLeft(32).withTrimmedRight(24);
		g.setFont(Font(13.0f, Font::bold));
		g.setColour(TEXT_COLOR);
		g.drawText(mode == PlayerSlots::HOLD_SPOT ? "Hold spot to reclaim" : "Next person takes slot", text.withHeight(24), Justification::centredLeft, true);
		g.setFont(Font(11.0f));
		g.setColour(dimText);
		g.drawText(mode == PlayerSlots::HOLD_SPOT ? "Kept where lost, walk in to take it back" : "Freed at once for the last person who arrived",
			text.withTrimmedTop(22), Justification::topLeft, true);

		if (ticked)
		{
			g.setColour(HIGHLIGHT_COLOR);
			Path tick;
			Point<float> c(r.getRight() - 12, r.getY() + 14);
			tick.startNewSubPath(c.x - 5, c.y);
			tick.lineTo(c.x - 1.5f, c.y + 4);
			tick.lineTo(c.x + 5, c.y - 4);
			g.strokePath(tick, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
		}
	}
};

void TrackingMonitorPanel::showSlotModeMenu()
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;

	PopupMenu m;
	m.addCustomItem(PlayerSlots::NEXT_PERSON + 1, std::make_unique<SlotModeMenuItem>(PlayerSlots::NEXT_PERSON, slotReleaseMode == PlayerSlots::NEXT_PERSON));
	m.addCustomItem(PlayerSlots::HOLD_SPOT + 1, std::make_unique<SlotModeMenuItem>(PlayerSlots::HOLD_SPOT, slotReleaseMode == PlayerSlots::HOLD_SPOT));

	WeakReference<ControllableContainer> slotsRef(&sm->playerSlots);
	WeakReference<Component> panelRef(this);
	m.showMenuAsync(PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(slotModeRect)), [slotsRef, panelRef](int result)
		{
			PlayerSlots* p = dynamic_cast<PlayerSlots*>(slotsRef.get());
			if (p == nullptr || result <= 0) return;
			EnumParameter* mode = p->releaseMode;
			String key = result - 1 == PlayerSlots::HOLD_SPOT ? "Hold Spot To Reclaim" : "Next Person Takes Slot";
			if (key != mode->getValueKey()) mode->setUndoableValue(mode->getValueKey(), key);
			if (TrackingMonitorPanel* panel = dynamic_cast<TrackingMonitorPanel*>(panelRef.get())) panel->repaint();
		});
}

int TrackingMonitorPanel::getSlotChipAt(Point<int> pos) const
{
	for (int i = 0; i < slotChipRects.size(); i++) if (slotChipRects[i].contains(pos)) return i;
	return -1;
}

void TrackingMonitorPanel::changeSlotCount(int delta)
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;
	IntParameter* count = sm->playerSlots.slotCount;
	int newCount = jlimit((int)count->minimumValue, (int)count->maximumValue, count->intValue() + delta);
	if (newCount != count->intValue()) count->setUndoableValue(count->intValue(), newCount);
	timerCallback();
}

void TrackingMonitorPanel::showSlotMenu(int slotIndex, Point<int> clickPosition)
{
	SpatialModule* sm = getSpatialModule();
	if (sm == nullptr) return;
	PlayerSlots& ps = sm->playerSlots;
	PlayerSlot* slot = slotIndex >= 0 && slotIndex < ps.slots.size() ? ps.slots[slotIndex] : nullptr;

	PopupMenu m;
	if (slot != nullptr)
	{
		m.addItem(1, "Rename");
		m.addItem(2, "Set color...");
		m.addSeparator();
	}
	m.addItem(3, "Enabled", true, ps.enabled->boolValue());

	WeakReference<ControllableContainer> slotRef(slot);
	WeakReference<ControllableContainer> slotsRef(&ps);
	WeakReference<Component> panelRef(this);
	Rectangle<int> chip = slotIndex >= 0 && slotIndex < slotChipRects.size() ? slotChipRects[slotIndex] : Rectangle<int>();
	m.showMenuAsync(PopupMenu::Options(), [slotRef, slotsRef, panelRef, clickPosition, chip](int result)
		{
			PlayerSlot* s = dynamic_cast<PlayerSlot*>(slotRef.get());
			PlayerSlots* p = dynamic_cast<PlayerSlots*>(slotsRef.get());
			TrackingMonitorPanel* panel = dynamic_cast<TrackingMonitorPanel*>(panelRef.get());
			if (p == nullptr) return;
			switch (result)
			{
			case 1: if (s != nullptr && panel != nullptr) panel->showRenameEditor(s, chip.withTrimmedLeft(20).withSizeKeepingCentre(jmax(chip.getWidth() - 20, 120), 24)); break;
			case 2: if (s != nullptr && panel != nullptr) panel->showColorEditor(s, s->color, clickPosition); break;
			case 3: p->enabled->setUndoableValue(p->enabled->boolValue(), !p->enabled->boolValue()); break;
			}
		});
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
		Colour light = c.interpolatedWith(Colours::white, .5f);
		Rectangle<float> rf = row.toFloat();
		bool expanded = isExpanded(z.zone.get());

		//card background, tinted with the zone color when someone is inside
		if (!z.hidden)
		{
			g.setColour(z.selected ? HIGHLIGHT_COLOR.withAlpha(.12f) : (z.presence ? rowBg.overlaidWith(c.withAlpha(.12f)) : rowBg));
			g.fillRoundedRectangle(rf, 4);
			if (z.presence && !z.selected)
			{
				g.setColour(c.withAlpha(.5f));
				g.drawRoundedRectangle(rf.reduced(.5f), 4, 1);
			}
		}
		if (z.selected)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawRoundedRectangle(rf.reduced(.5f), 4, 1);
		}

		Rectangle<int> card = row.reduced(8, 0);
		Rectangle<int> line1 = card.removeFromTop(28);
		Rectangle<int> barLine = card.removeFromTop(cardHeight - 28 - 4);

		//unfold arrow
		Rectangle<int> arrowR = getCardArrowRect(row);
		Path arrow;
		arrow.addTriangle(0, 0, 8, 4.5f, 0, 9);
		arrow.applyTransform(AffineTransform::translation(-4, -4.5f).rotated(expanded ? MathConstants<float>::halfPi : 0).translated(arrowR.toFloat().getCentre()));
		g.setColour(dimText);
		g.fillPath(arrow);
		line1.setLeft(arrowR.getRight());

		//filled glowing dot when occupied, ring when empty
		Rectangle<float> dot = Rectangle<float>(10, 10).withCentre(Point<float>(line1.getX() + 7.0f, (float)line1.getCentreY()));
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
		line1.removeFromLeft(20);

		//count / capacity and presence time on the right, name on the left
		String countText = String(z.personCount) + " / " + String(z.capacity);
		String timeText = z.presence ? sep + formatTime(z.presenceTime) : String();
		Font countFont(15.0f, Font::bold), timeFont(13.0f);
		int countW = (int)countFont.getStringWidthFloat(countText) + 2;
		int timeW = (int)timeFont.getStringWidthFloat(timeText) + 2;

		g.setFont(timeFont);
		g.setColour(TEXT_COLOR);
		g.drawText(timeText, line1.removeFromRight(timeW), Justification::centredRight);
		g.setFont(countFont);
		g.setColour(z.presence ? light : dimText);
		g.drawText(countText, line1.removeFromRight(countW), Justification::centredRight);

		g.setFont(Font(14.0f, z.hidden ? Font::italic : Font::plain));
		g.setColour(z.hidden || !z.enabled ? dimText : TEXT_COLOR);
		g.drawText(z.hidden ? z.name + " (hidden)" : z.name, line1.withTrimmedRight(8), Justification::centredLeft, true);

		//full width occupancy bar and percentage
		Rectangle<int> pctR = barLine.removeFromRight(44);
		g.setFont(Font(12.0f));
		g.setColour(z.presence ? TEXT_COLOR : dimText);
		g.drawText(String(roundToInt(z.occupancy * 100)) + " %", pctR, Justification::centredRight);
		drawBar(g, barLine.withTrimmedLeft(arrowR.getWidth()).withTrimmedRight(6).toFloat().withSizeKeepingCentre(barLine.getWidth() - arrowR.getWidth() - 6.0f, 6), z.occupancy, c);

		//detail : one value per line, label on the left, value aligned on the right
		if (expanded)
		{
			Rectangle<int> detail = card.withTrimmedTop(2).withTrimmedLeft(arrowR.getWidth());
			g.setColour(borderColor);
			g.drawHorizontalLine(detail.getY(), (float)detail.getX(), (float)detail.getRight());
			detail.removeFromTop(4);

			auto drawDetail = [&](const String& label, const String& value, Colour valueColor)
			{
				Rectangle<int> l = detail.removeFromTop(detailLineHeight);
				g.setFont(Font(12.0f));
				g.setColour(dimText);
				g.drawText(label, l, Justification::centredLeft);
				g.setFont(Font(13.0f));
				g.setColour(valueColor);
				g.drawText(value, l, Justification::centredRight);
			};

			drawDetail("Occupancy", countText + sep + String(roundToInt(z.occupancy * 100)) + " %", TEXT_COLOR);
			drawDetail("Presence time", z.presence ? formatTime(z.presenceTime) : String("-"), z.presence ? TEXT_COLOR : dimText);
			drawDetail("Last entered", z.lastEnteredId >= 0 ? "#" + String(z.lastEnteredId) : String("-"), z.lastEnteredId >= 0 ? getPersonColor(z.lastEnteredId) : dimText);
			drawDetail("Avg position", String(z.averagePosition.x, 2) + " , " + String(z.averagePosition.y, 2), TEXT_COLOR);
			drawDetail("Avg distance", String(z.averageDistance, 2) + " m", TEXT_COLOR);
		}
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
		personRowRects.add(row);
		personRowIds.add(p.id);
		g.setColour(p.id == selectedPersonId ? HIGHLIGHT_COLOR.withAlpha(.12f) : rowBg);
		g.fillRoundedRectangle(row.toFloat(), 3);
		if (p.id == selectedPersonId)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawRoundedRectangle(row.toFloat().reduced(.5f), 3, 1);
		}

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
		personRowRects.add(cell.toNearestInt());
		personRowIds.add(p.id);
		if (p.id == selectedPersonId)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawEllipse(cell.expanded(5), 1.5f);
		}

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

	bool overSpout = spoutRect.contains(e.getPosition());
	if (overSpout != spoutHovered)
	{
		spoutHovered = overSpout;
		repaint(spoutRect);
	}

	bool overNDI = ndiRect.contains(e.getPosition());
	if (overNDI != ndiHovered)
	{
		ndiHovered = overNDI;
		repaint(ndiRect);
	}

	bool modeHovered = slotModeRect.contains(e.getPosition());
	if (modeHovered != slotModeHovered)
	{
		slotModeHovered = modeHovered;
		repaint(slotModeRect);
	}

	//shortcuts list : open while the mouse is on the chip or on the list itself
	bool overShortcuts = shortcutsRect.contains(e.getPosition()) || (shortcutsHovered && getShortcutsPanelRect().contains(e.getPosition()));
	if (overShortcuts != shortcutsHovered)
	{
		shortcutsHovered = overShortcuts;
		repaint();
	}
	if (overShortcuts) return;

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

	//click on a person (top view, column or rail) : its parameters and values in the Inspector
	//(right click : Assign slot / Release slot)
	//but first the shortcuts chip : a click keeps the list open ; clicks on the open list do nothing else
	if (shortcutsRect.contains(e.getPosition()))
	{
		shortcutsPinned = !shortcutsPinned;
		repaint();
		return;
	}
	if ((shortcutsPinned || shortcutsHovered) && getShortcutsPanelRect().contains(e.getPosition())) return;

	//Spout / NDI chips : the box turns the output on / off, the rest shows its parameters
	if (SpatialModule* sm = getSpatialModule())
	{
		if (handleOutputChipClick(e.getPosition(), spoutRect, sm->videoOutput.spout)) return;
		if (handleOutputChipClick(e.getPosition(), ndiRect, sm->videoOutput.ndi)) return;
	}

	int personId = getPersonAt(e.position);
	if (personId >= 0)
	{
		selectPerson(personId);
		if (e.mods.isRightButtonDown()) showPersonMenu(personId, e.getPosition());
		else
		{
			//may become a drag onto a slot chip
			slotDragPersonId = personId;
			slotDragStart = slotDragMouse = e.position;
		}
		return;
	}

	//slots strip : click selects the slot (Inspector), right click opens the slots menu
	if (slotBarRect.contains(e.getPosition()))
	{
		if (slotModeRect.contains(e.getPosition()))
		{
			showSlotModeMenu();
			return;
		}

		int chipIndex = getSlotChipAt(e.getPosition());
		if (e.mods.isRightButtonDown()) showSlotMenu(chipIndex, e.getPosition());
		else if (chipIndex >= 0 && slots[chipIndex].slot != nullptr)
		{
			slots[chipIndex].slot->selectThis();
			//may become a drag onto a person
			slotDragSlot = chipIndex;
			slotDragStart = slotDragMouse = e.position;
		}
		timerCallback();
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
			if (getCardArrowRect(zoneRowRects[i]).contains(e.getPosition()) && !columnCollapsed)
			{
				//unfold / fold the detail of the card
				if (isExpanded(z)) expandedZones.removeAllInstancesOf(WeakReference<ControllableContainer>(z));
				else if (z != nullptr) expandedZones.add(z);
				timerCallback();
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
	if (slotDragPersonId >= 0 || slotDragSlot >= 0)
	{
		slotDragMouse = e.position;
		if (!slotDragging && slotDragStart.getDistanceFrom(e.position) > 5) slotDragging = true;
		if (slotDragging) repaint();
		return;
	}

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
	if (slotDragPersonId >= 0 || slotDragSlot >= 0)
	{
		int personId = slotDragPersonId >= 0 ? slotDragPersonId : getPersonAt(e.position);
		int slotIndex = slotDragSlot >= 0 ? slotDragSlot : getSlotChipAt(e.getPosition());
		bool dropped = slotDragging && personId >= 0 && slotIndex >= 0;

		slotDragPersonId = slotDragSlot = -1;
		slotDragging = false;
		if (dropped) assignSlot(personId, slotIndex);
		repaint();
		return;
	}

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
	showColorEditor(zone, zone->itemColor, position);
}

void TrackingMonitorPanel::showColorEditor(Inspectable* owner, ColorParameter* color, Point<int> position)
{
	//one picker at a time : a click outside closes the open one first
	if (zoneColorUI != nullptr && zoneColorUI->colorEditor != nullptr) return;

	//the picker edits the selected items, so select this zone or slot only
	owner->selectThis();

	//invisible ColorParameterUI at the click position : it opens the picker there
	//and makes the whole edit a single undo step when the picker closes
	zoneColorUI.reset(color->createColorParamUI());
	addChildComponent(zoneColorUI.get());
	zoneColorUI->setBounds(Rectangle<int>(16, 16).withCentre(position));
	zoneColorUI->showEditWindow();
}

void TrackingMonitorPanel::mouseDoubleClick(const MouseEvent& e)
{
	if (drawingFreeShape || drawingBoxShape >= 0 || Time::getMillisecondCounter() - drawFinishedMs < 500) return;

	//double click on a slot chip : rename the slot
	int chipIndex = getSlotChipAt(e.getPosition());
	if (chipIndex >= 0)
	{
		Rectangle<int> chip = slotChipRects[chipIndex];
		showRenameEditor(slots[chipIndex].slot.get(), chip.withTrimmedLeft(20).withSizeKeepingCentre(jmax(chip.getWidth() - 20, 120), 24));
		return;
	}

	//double click on a zone name in the column (not the rail, which shows no names)
	if (columnRect.contains(e.getPosition()))
	{
		if (columnCollapsed) return;
		for (int i = 0; i < zones.size() && i < zoneRowRects.size(); i++)
		{
			if (!zoneRowRects[i].contains(e.getPosition())) continue;
			//only on the first line of the card, not on its arrow
			Rectangle<int> nameR = zoneRowRects[i].withHeight(28).withTrimmedLeft(getCardArrowRect(zoneRowRects[i]).getRight() - zoneRowRects[i].getX() + 20).withTrimmedRight(90);
			if (!zoneRowRects[i].withHeight(28).contains(e.getPosition()) || getCardArrowRect(zoneRowRects[i]).contains(e.getPosition())) return;
			showRenameEditor(dynamic_cast<SpatialZone*>(zones[i].zone.get()), nameR.reduced(0, 3));
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

void TrackingMonitorPanel::showRenameEditor(ControllableContainer* zone, Rectangle<int> bounds)
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

	ControllableContainer* zone = renameZone.get();	// a zone or a player slot
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

	if (!drawingFreeShape)
	{
		//selected person : 1-9 (or the numeric keypad) assigns the slot, 0 / Delete removes it
		if (selectedPersonId < 0) return false;
		int k = key.getKeyCode();
		int digit = k >= '0' && k <= '9' ? k - '0' : (k >= KeyPress::numberPad0 && k <= KeyPress::numberPad9 ? k - KeyPress::numberPad0 : -1);
		if (digit >= 1) assignSlot(selectedPersonId, digit - 1);
		else if (digit == 0 || key == KeyPress::deleteKey || key == KeyPress::backspaceKey) assignSlot(selectedPersonId, -1);
		else return false;
		return true;
	}

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
