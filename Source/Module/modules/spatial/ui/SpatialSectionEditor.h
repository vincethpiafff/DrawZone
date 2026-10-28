/*
  ==============================================================================

	SpatialSectionEditor.h
	Created: 28 Oct 2026
	Author:  DrawZone

	Inspector look of the Spatial module blocks (style D) : an icon badge in
	the block colour before the title, state chips on the right of the
	header (green = active), a border in the block colour when the block is
	open. Wraps any organicUI container editor (generic, enabling, manager,
	network) so each block keeps its own buttons and behaviour.

  ==============================================================================
*/

#pragma once

namespace SpatialSection
{
	enum Type { SOURCES, SCENE, ZONES, SLOTS, OUTPUT, NETWORK };

	struct Chip
	{
		String text;
		bool on = false;
		bool operator==(const Chip& o) const { return text == o.text && on == o.on; }
		bool operator!=(const Chip& o) const { return !(*this == o); }
	};

	typedef std::function<Array<Chip>()> SummaryFunc;

	inline Colour getColour(Type t)
	{
		switch (t)
		{
		case SOURCES: return Colour(0xffc98bdb);
		case SCENE: return Colour(0xffb48fd6);
		case ZONES: return Colour(0xffe0b44e);
		case SLOTS: return Colour(0xff6cc58b);
		case OUTPUT: return Colour(0xff4f9de8);
		default: return Colour(0xff8f8f8f);
		}
	}

	//icons drawn on a 16 x 16 grid
	inline Path getIcon(Type t)
	{
		Path p;
		switch (t)
		{
		case SOURCES:
			p.addEllipse(6.4f, 6.4f, 3.2f, 3.2f);
			p.addCentredArc(8, 8, 5, 5, 0, -2.4f, -0.75f, true);
			p.addCentredArc(8, 8, 5, 5, 0, 0.75f, 2.4f, true);
			p.addCentredArc(8, 8, 7.5f, 7.5f, 0, -2.4f, -0.75f, true);
			p.addCentredArc(8, 8, 7.5f, 7.5f, 0, 0.75f, 2.4f, true);
			break;
		case SCENE:
			p.addRoundedRectangle(2, 3, 12, 10, 1);
			p.startNewSubPath(2, 8); p.lineTo(14, 8);
			p.startNewSubPath(8, 3); p.lineTo(8, 13);
			break;
		case ZONES:
			p.addRoundedRectangle(2, 3, 7, 6, 1);
			p.addEllipse(7.8f, 8.4f, 6.4f, 5.2f);
			break;
		case SLOTS:
			p.addEllipse(3.3f, 3.8f, 4.4f, 4.4f);
			p.addEllipse(8.8f, 3.8f, 4.4f, 4.4f);
			p.startNewSubPath(2, 13); p.quadraticTo(5.5f, 8.5f, 9, 13);
			break;
		case OUTPUT:
			p.addRoundedRectangle(1.8f, 3, 12.4f, 8.4f, 1);
			p.startNewSubPath(5.5f, 13.5f); p.lineTo(10.5f, 13.5f);
			p.addTriangle(7, 5.8f, 7, 8.6f, 9.5f, 7.2f);
			break;
		default:
			p.addEllipse(2, 6.5f, 3, 3);
			p.addEllipse(11, 2.5f, 3, 3);
			p.addEllipse(11, 10.5f, 3, 3);
			p.startNewSubPath(5, 7.3f); p.lineTo(11, 4.7f);
			p.startNewSubPath(5, 8.7f); p.lineTo(11, 11.3f);
			break;
		}
		return p;
	}

	inline void paintBadge(Graphics& g, Type t, Rectangle<float> r)
	{
		Colour c = getColour(t);
		g.setColour(c.withAlpha(.22f));
		g.fillRoundedRectangle(r, 4);
		Rectangle<float> ir = r.reduced(r.getWidth() * .17f);
		g.setColour(c);
		g.strokePath(getIcon(t), PathStrokeType(1.3f), AffineTransform::scale(ir.getWidth() / 16, ir.getHeight() / 16).translated(ir.getX(), ir.getY()));
	}

	//chips laid out from the right of r
	inline void paintChips(Graphics& g, const Array<Chip>& chips, Rectangle<float> r)
	{
		Font f(11.0f);
		g.setFont(f);
		float x = r.getRight();
		for (int i = chips.size() - 1; i >= 0; i--)
		{
			const Chip& c = chips.getReference(i);
			float w = f.getStringWidthFloat(c.text) + 10;
			if (x - w < r.getX()) break; //not enough room : the leftmost chips are dropped
			Rectangle<float> cr(x - w, r.getCentreY() - 7, w, 14);
			g.setColour(c.on ? Colour(0xff6cc58b).withAlpha(.25f) : Colours::black.withAlpha(.28f));
			g.fillRoundedRectangle(cr, 3);
			g.setColour(c.on ? Colour(0xffbfe8cc) : Colour(0xff9a9a9a));
			g.drawText(c.text, cr, Justification::centred, false);
			x -= w + 4;
		}
	}
}

template<class Base>
class SpatialSectionEditor :
	public Base
{
public:
	template<typename... Args>
	SpatialSectionEditor(SpatialSection::Type type, SpatialSection::SummaryFunc summary, Args&&... args) :
		Base(std::forward<Args>(args)...),
		sectionType(type),
		summary(summary)
	{
		refresher.onTick = [this]
			{
				Array<SpatialSection::Chip> c = this->summary != nullptr ? this->summary() : Array<SpatialSection::Chip>();
				if (c != chips)
				{
					chips = c;
					this->repaint();
				}
			};
		refresher.onTick();
		refresher.startTimerHz(4);
	}

	~SpatialSectionEditor()
	{
		refresher.stopTimer();
	}

	void resizedInternalHeader(Rectangle<int>& r) override
	{
		badgeRect = r.removeFromLeft(r.getHeight()).reduced(1);
		r.removeFromLeft(4);
		Base::resizedInternalHeader(r);
		chipsRect = r.reduced(4, 0);
	}

	void paint(Graphics& g) override
	{
		Base::paint(g);
		if (this->container == nullptr || this->container.wasObjectDeleted()) return;

		SpatialSection::paintBadge(g, sectionType, badgeRect.toFloat());
		SpatialSection::paintChips(g, chips, chipsRect.toFloat());

		if (!this->container->editorIsCollapsed)
		{
			g.setColour(SpatialSection::getColour(sectionType).withAlpha(.55f));
			g.drawRoundedRectangle(this->getLocalBounds().toFloat().reduced(.75f), 4, 1.5f);
		}
	}

private:
	struct Refresher : public Timer
	{
		std::function<void()> onTick;
		void timerCallback() override { if (onTick) onTick(); }
	};

	SpatialSection::Type sectionType;
	SpatialSection::SummaryFunc summary;
	Array<SpatialSection::Chip> chips;
	Rectangle<int> badgeRect, chipsRect;
	Refresher refresher;
};

//a manager shown with the section look
template<class T>
class SpatialSectionManager :
	public BaseManager<T>
{
public:
	SpatialSectionManager(const String& name, SpatialSection::Type type) :
		BaseManager<T>(name),
		sectionType(type)
	{
	}

	SpatialSection::Type sectionType;
	SpatialSection::SummaryFunc sectionSummary;

	InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables = Array<Inspectable*>()) override
	{
		return new SpatialSectionEditor<GenericManagerEditor<T>>(sectionType, sectionSummary, this, isRoot);
	}
};
