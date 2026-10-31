/*
  ==============================================================================

	TopViewRenderer.cpp
	Created: 28 Oct 2026
	Author:  DrawZone

  ==============================================================================
*/

void TopViewModel::update(SpatialModule* sm)
{
	hasModule = sm != nullptr;

	sources.clearQuick();
	zones.clearQuick();
	slots.clearQuick();
	personZones.clearQuick();
	personRelativePositions.clearQuick();

	if (hasModule)
	{
		moduleName = sm->niceName;
		localPort = sm->localPort != nullptr ? sm->localPort->intValue() : 0;
		snapshot = sm->trackingData.getSnapshot();

		{
			const ScopedLock sl(sm->sourceManager.items.getLock());
			for (auto& s : sm->sourceManager.items)
			{
				SourceView sv;
				sv.source = s;
				sv.id = s;
				sv.name = s->niceName;
				if (AugmentaSource* a = dynamic_cast<AugmentaSource*>(s)) { if (a->isUsed) sv.name += " :" + String(localPort); }
				sv.enabled = s->enabled->boolValue();
				sv.reception = sv.enabled ? s->getReceptionText(sm) : String("Disabled");
				sv.color = s->itemColor->getColor();
				sv.personCount = sm->trackingData.getNumPersons(s);
				sources.add(sv);
			}
		}

		for (auto& z : sm->zoneManager.items)
		{
			ZoneView zv;
			zv.zone = z;
			zv.name = z->niceName;
			zv.bounds = z->getBounds();
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
			zv.capacity = z->capacity->intValue();
			zv.occupancy = z->occupancy->floatValue();
			zv.presenceTime = z->presenceTime->floatValue();
			zv.lastEnteredId = z->lastEnteredId->intValue();
			zv.averagePosition = z->averagePosition->getPoint();
			zv.averageDistance = z->averageDistanceToCenter->floatValue();
			zones.add(zv);
		}

		selectedPersonId = -1;
		for (auto& pp : sm->people) if (pp.second->isSelected) selectedPersonId = pp.first;

		PlayerSlots& ps = sm->playerSlots;
		slotsEnabled = ps.enabled->boolValue();
		slotReleaseMode = (int)ps.releaseMode->getValueDataAsEnum<PlayerSlots::ReleaseMode>();
		freeSlots = ps.freeSlots->intValue();
		reclaimRadius = ps.reclaimRadius->floatValue();
		holdTime = ps.holdTime->floatValue();
		for (auto& s : ps.slots)
		{
			SlotView sv;
			sv.slot = s;
			sv.name = s->niceName;
			sv.color = s->color->getColor();
			sv.occupied = s->occupied->boolValue();
			sv.held = s->isHeld();
			sv.personId = s->personId->intValue();
			sv.position = s->position->getPoint();
			sv.holdRemaining = s->holdRemaining->floatValue();
			slots.add(sv);
		}

		//zone and relative position computed by the module (Values > People) ; a person not there yet : in no zone
		for (auto& p : snapshot.persons)
		{
			auto it = sm->people.find(p.id);
			bool known = it != sm->people.end();
			personZones.add(known ? it->second->firstZoneIndex : -1);
			personRelativePositions.add(known ? it->second->relativePosition->getPoint() : Point<float>(-1, -1));
		}
	}
	else
	{
		snapshot = TrackingData::Snapshot();
	}

	updateMotions();
}

int TopViewModel::getSlotIndexForPerson(int id) const
{
	for (int i = 0; i < slots.size(); i++) if (slots[i].occupied && !slots[i].held && slots[i].personId == id) return i;
	return -1;
}

Colour TopViewModel::getSourceColor(const void* id) const
{
	for (auto& s : sources) if (s.id == id) return s.color;
	return Colours::transparentBlack;
}

Colour TopViewModel::getPersonColor(int id)
{
	return Colour::fromHSV((id * .13f) - (int)(id * .13f), .7f, 1, 1);
}

void TopViewModel::updateMotions()
{
	uint32 now = Time::getMillisecondCounter();
	std::map<int, Motion> updated; //people who left are dropped

	for (auto& p : snapshot.persons)
	{
		Point<float> meters = p.position;

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
			if (dt >= .05f) //the panel also refreshes while dragging a zone, keep a meaningful time step
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

void TopViewRenderer::paint(Graphics& g, const TopViewModel& m, Rectangle<float> scene, float scale, Rectangle<float> clip, const Options& o,
	std::function<void(int zoneIndex, Rectangle<float> zoneRect)> zoneOverlay,
	std::function<void()> afterZones)
{
	using namespace TrackingMonitorStyle;

	Point<float> size = m.snapshot.sceneSize;
	auto toScreen = [&](Rectangle<float> meters) { return Rectangle<float>(scene.getX() + meters.getX() * scale, scene.getY() + meters.getY() * scale, meters.getWidth() * scale, meters.getHeight() * scale); };

	g.setColour(o.background);
	g.fillRect(scene);

	//1 m grid
	if (o.grid)
	{
		g.setColour(NORMAL_COLOR.withAlpha(.18f));
		for (int i = 0; i <= (int)size.x; i++) g.drawVerticalLine((int)(scene.getX() + i * scale), scene.getY(), scene.getBottom());
		for (int i = 0; i <= (int)size.y; i++) g.drawHorizontalLine((int)(scene.getY() + i * scale), scene.getX(), scene.getRight());
		g.setColour(borderColor);
		g.drawRect(scene, 1);
	}

	//Zones, under the people, glowing when someone is inside
	for (int i = 0; i < m.zones.size(); i++)
	{
		auto& z = m.zones.getReference(i);
		if (z.hidden) continue;

		//only draw the part inside the scene : huge coordinates crash the renderer
		Rectangle<float> zr = toScreen(z.bounds).getIntersection(clip);
		if (zr.isEmpty()) continue;

		//an ellipse is drawn whole (its bounds stay near the scene), clipped to the scene
		Rectangle<float> er = toScreen(z.bounds);
		Path sp = z.outline; //free shape, meters to screen
		sp.applyTransform(AffineTransform::scale(scale).translated(scene.getX(), scene.getY()));
		Graphics::ScopedSaveState clipState(g);
		g.reduceClipRegion(clip.getSmallestIntegerContainer());

		Colour c = z.enabled ? z.color : NORMAL_COLOR;
		Colour light = c.interpolatedWith(Colours::white, .5f);
		bool selected = o.selection && z.selected;

		if (z.presence)
		{
			if (z.freeShape) drawGlow(g, sp, c);
			else drawGlow(g, z.ellipse ? er : zr, c, z.ellipse);
			g.setColour(c.withAlpha(.22f));
			if (z.freeShape) g.fillPath(sp);
			else if (z.ellipse) g.fillEllipse(er);
			else g.fillRect(zr);
		}

		g.setColour(selected ? HIGHLIGHT_COLOR : c.withAlpha(z.presence ? 1 : .6f));
		float lineWidth = z.presence || selected ? 2.0f : 1.0f;
		if (z.freeShape) g.strokePath(sp, PathStrokeType(lineWidth, PathStrokeType::curved));
		else if (z.ellipse) g.drawEllipse(er, lineWidth);
		else g.drawRect(zr, lineWidth);

		if (o.zoneNames)
		{
			g.setColour(z.enabled ? light : NORMAL_COLOR);
			g.setFont(Font(12.0f));
			g.drawText(z.name, zr.reduced(5, 3).withHeight(16), Justification::topLeft, true);
		}

		if (zoneOverlay != nullptr) zoneOverlay(i, zr);
	}

	if (afterZones != nullptr) afterZones();

	//Held slots : dashed circle where the person was lost, anyone without a slot walking in takes it back
	if (o.heldSpots)
	{
		for (auto& s : m.slots)
		{
			if (!s.held) continue;

			Point<float> centre = scene.getTopLeft() + s.position * scale;
			float radius = jmax(10.0f, m.reclaimRadius * scale);
			Rectangle<float> circle = Rectangle<float>(radius * 2, radius * 2).withCentre(centre);

			g.setColour(s.color.withAlpha(.1f));
			g.fillEllipse(circle);

			Path ring;
			ring.addEllipse(circle);
			strokeDashed(g, ring, 1.5f, 5, 4, s.color.withAlpha(.8f));

			//time left : an arc that empties, full circle when held until reclaimed
			float ratio = s.holdRemaining < 0 || m.holdTime <= 0 ? 1.0f : jlimit(0.0f, 1.0f, s.holdRemaining / m.holdTime);
			Path arc;
			arc.addCentredArc(centre.x, centre.y, radius + 4, radius + 4, 0, 0, MathConstants<float>::twoPi * ratio, true);
			g.setColour(s.color);
			g.strokePath(arc, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));

			String timeText = s.holdRemaining < 0 ? String(CharPointer_UTF8("\xe2\x88\x9e")) : formatTime(s.holdRemaining);
			g.setFont(Font(12.0f, Font::bold));
			g.drawText(s.name, Rectangle<float>(160, 16).withCentre(centre.translated(0, -7)), Justification::centred);
			g.setFont(Font(11.0f));
			g.setColour(s.color.interpolatedWith(Colours::white, .4f));
			g.drawText(timeText, Rectangle<float>(160, 14).withCentre(centre.translated(0, 8)), Justification::centred);
		}
	}

	//Trails : last 2 s of each person, fading toward the oldest point
	if (o.trails)
	{
		for (auto& p : m.snapshot.persons)
		{
			auto it = m.motions.find(p.id);
			if (it == m.motions.end() || it->second.trail.size() < 2) continue;

			auto& trail = it->second.trail;
			Colour c = TopViewModel::getPersonColor(p.id);
			for (int i = 1; i < trail.size(); i++)
			{
				Point<float> a = scene.getTopLeft() + trail[i - 1] * scale;
				Point<float> b = scene.getTopLeft() + trail[i] * scale;
				float age = (float)i / trail.size(); //0 = oldest, 1 = newest
				g.setColour(c.withAlpha(.05f + .45f * age));
				g.drawLine(Line<float>(a, b), 1.0f + 2.0f * age);
			}
		}
	}

	//People : dot with a halo and the id, arrow toward where they go when moving
	for (auto& p : m.snapshot.persons)
	{
		Point<float> pos = scene.getTopLeft() + p.position * scale;
		Colour c = TopViewModel::getPersonColor(p.id);

		auto it = m.motions.find(p.id);
		if (it != m.motions.end())
		{
			Point<float> v = it->second.velocity;
			float speed = v.getDistanceFromOrigin();
			if (speed > TopViewModel::minArrowSpeed)
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
		if (o.sourceRings)
		{
			g.setColour(m.getSourceColor(p.source));
			g.drawEllipse(Rectangle<float>(20, 20).withCentre(pos), 2);
			g.setColour(c);
		}
		if (o.selection && p.id == m.selectedPersonId)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawEllipse(Rectangle<float>(32, 32).withCentre(pos), 1.5f);
			g.setColour(c);
		}

		//player slot : label with the slot name in its color, the id small next to it
		int slotIndex = o.slots ? m.getSlotIndexForPerson(p.id) : -1;
		if (slotIndex < 0)
		{
			if (!o.personIds) continue;
			g.setFont(Font(13.0f, Font::bold));
			g.drawText(String(p.id), Rectangle<float>(pos.x + 10, pos.y - 20, 40, 16), Justification::centredLeft);
			continue;
		}

		auto& s = m.slots.getReference(slotIndex);
		Font nameFont(12.0f, Font::bold);
		Font idFont(10.0f);
		String idText = o.personIds ? "#" + String(p.id) : String();
		float nameW = nameFont.getStringWidthFloat(s.name);
		float idW = o.personIds ? idFont.getStringWidthFloat(idText) : -4.0f;
		Rectangle<float> label(pos.x + 10, pos.y - 26, nameW + idW + 18, 18);

		g.setColour(s.color.darker(.6f).withAlpha(.85f));
		g.fillRoundedRectangle(label, 9);
		g.setColour(s.color);
		g.drawRoundedRectangle(label, 9, 1.2f);
		g.setFont(nameFont);
		g.setColour(Colours::white);
		g.drawText(s.name, label.withTrimmedLeft(7).withWidth(nameW + 2), Justification::centredLeft, false);
		if (!o.personIds) continue;
		g.setFont(idFont);
		g.setColour(Colours::white.withAlpha(.6f));
		g.drawText(idText, label.withTrimmedLeft(nameW + 11).withWidth(idW + 2), Justification::centredLeft, false);
	}
}
