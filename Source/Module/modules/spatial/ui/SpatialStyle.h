/*
  ==============================================================================

	SpatialStyle.h
	Created: 29 Oct 2026
	Author:  DrawZone

	Look shared by the Tracking Monitor, the video output and the Inspector
	sections of the Spatial module : one palette and the small drawing helpers
	(chip, chevron, tick, dashed stroke...).

  ==============================================================================
*/

#pragma once

//dark "show" look of the Tracking Monitor
namespace TrackingMonitorStyle
{
	const Colour panelBg(0xff141414);
	const Colour sceneBg(0xff0c0c0c);
	const Colour columnBg(0xff1a1a1a);
	const Colour rowBg(0xff222222);
	const Colour borderColor(0xff333333);
	const Colour trackBg(0xff2e2e2e);		// empty part of a bar
	const Colour dimText(0xff9a9a9a);
	const Colour activeColor(0xff6cc58b);	// green : receiving, active, on
	const Colour activeText(0xffbfe8cc);	// text on an active chip
	const String sep = String(CharPointer_UTF8(" \xc2\xb7 "));
	const String times = String(CharPointer_UTF8(" \xc3\x97 "));

	//rounded chip of the Monitor : brighter when hovered ; border in the highlight color when highlighted
	inline void drawChip(Graphics& g, Rectangle<float> r, bool hovered, bool highlighted = false)
	{
		g.setColour(hovered ? rowBg.brighter(.15f) : rowBg);
		g.fillRoundedRectangle(r, 4);
		g.setColour(highlighted ? HIGHLIGHT_COLOR : (hovered ? TEXT_COLOR.withAlpha(.5f) : borderColor));
		g.drawRoundedRectangle(r.reduced(.5f), 4, 1);
	}

	//small triangle of a drop-down, pointing down (or up when open)
	inline void drawChevron(Graphics& g, Point<float> centre, bool up, Colour c)
	{
		Rectangle<float> r = Rectangle<float>(8, 5).withCentre(centre);
		Path p;
		if (up) p.addTriangle(r.getX(), r.getBottom(), r.getRight(), r.getBottom(), r.getCentreX(), r.getY());
		else p.addTriangle(r.getX(), r.getY(), r.getRight(), r.getY(), r.getCentreX(), r.getBottom());
		g.setColour(c);
		g.fillPath(p);
	}

	//check mark : from a, down to b, up to c
	inline void drawTick(Graphics& g, Point<float> a, Point<float> b, Point<float> c, Colour colour)
	{
		Path tick;
		tick.startNewSubPath(a);
		tick.lineTo(b);
		tick.lineTo(c);
		g.setColour(colour);
		g.strokePath(tick, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
	}

	//path drawn as dashes (held slot, link while dragging, mode icons)
	inline void strokeDashed(Graphics& g, const Path& p, float thickness, float dash, float gap, Colour c)
	{
		const float dashes[] = { dash, gap };
		Path dashed;
		PathStrokeType(thickness).createDashedStroke(dashed, p, dashes, 2);
		g.setColour(c);
		g.fillPath(dashed);
	}

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

	//"12.3 s" under a minute, "2:05" above
	inline String formatTime(float seconds)
	{
		if (seconds < 60) return String(seconds, 1) + " s";
		int s = (int)seconds;
		return String(s / 60) + ":" + String(s % 60).paddedLeft('0', 2);
	}

	//occupancy bar : dark track, filled part in the zone color
	inline void drawBar(Graphics& g, Rectangle<float> r, float value, Colour c)
	{
		g.setColour(trackBg);
		g.fillRoundedRectangle(r, r.getHeight() / 2);
		if (value <= 0) return;
		g.setColour(c);
		g.fillRoundedRectangle(r.withWidth(jmax(r.getHeight(), r.getWidth() * jmin(1.0f, value))), r.getHeight() / 2);
	}

	//SwapSlots mode icons : dashed arrow (Next Person Takes Slot), dashed circle around a dot (Hold Spot To Reclaim)
	inline void drawSlotModeIcon(Graphics& g, Rectangle<float> r, bool holdSpot, Colour c)
	{
		Path shape;
		if (holdSpot)
		{
			shape.addEllipse(r.reduced(1));
			strokeDashed(g, shape, 1.3f, 2.5f, 2, c);
			g.fillEllipse(Rectangle<float>(4, 4).withCentre(r.getCentre()));
		}
		else
		{
			float y = r.getCentreY();
			shape.startNewSubPath(r.getX(), y);
			shape.lineTo(r.getRight() - 4, y);
			strokeDashed(g, shape, 1.5f, 2.5f, 2, c);
			Path head;
			head.addTriangle(r.getRight(), y, r.getRight() - 5, y - 4, r.getRight() - 5, y + 4);
			g.fillPath(head);
		}
	}
}
