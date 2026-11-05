/*
  ==============================================================================

	Viewport3DRenderer.cpp
	Created: 5 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

void Viewport3DRenderer::Camera::fit(Point<float> sceneSize)
{
	target = Vec3(sceneSize.x / 2, sceneSize.y / 2, 0);
	yaw = 0;
	pitch = .9f;
	float diagonal = jmax(1.0f, sceneSize.getDistanceFromOrigin());
	distance = diagonal * .5f / std::tan(fov / 2) * 1.25f;
	fitted = true;
}

void Viewport3DRenderer::Camera::orbit(float dx, float dy)
{
	yaw -= dx * .008f;
	pitch = jlimit(minPitch, maxPitch, pitch + dy * .008f);
}

void Viewport3DRenderer::Camera::zoom(float factor)
{
	distance = jlimit(.5f, 1000.0f, distance / factor);
}

namespace Viewport3DDetail
{
	using Vec3 = Viewport3DRenderer::Vec3;

	//camera space : r right, u up, f forward (depth)
	struct View
	{
		Vec3 eye, r, u, f;
		float focal = 1;
		Point<float> centre;
		static constexpr float nearDepth = .05f;

		View(const Viewport3DRenderer::Camera& c, Rectangle<float> area)
		{
			Vec3 offset(std::sin(c.yaw) * std::cos(c.pitch), std::cos(c.yaw) * std::cos(c.pitch), std::sin(c.pitch));
			eye = c.target + offset * c.distance;
			f = (offset * -1.0f).normalised();
			Vec3 up(0, 0, 1);
			r = up.cross(f).normalised();
			u = f.cross(r);
			focal = area.getHeight() / 2 / std::tan(Viewport3DRenderer::Camera::fov / 2);
			centre = area.getCentre();
		}

		Vec3 toCamera(Vec3 p) const { Vec3 d = p - eye; return { d.dot(r), d.dot(u), d.dot(f) }; }
		Point<float> projectCamera(Vec3 c) const { return { centre.x + focal * c.x / c.z, centre.y - focal * c.y / c.z }; }

		bool project(Vec3 p, Point<float>& screen, float& depth) const
		{
			Vec3 c = toCamera(p);
			depth = c.z;
			if (c.z < nearDepth) return false;
			screen = projectCamera(c);
			return true;
		}

		float pixelsPerMeter(float depth) const { return focal / jmax(nearDepth, depth); }

		//polygon in world meters, cut at the near plane, as a screen path (empty when behind the camera)
		Path projectPolygon(const Array<Vec3>& points) const
		{
			Array<Vec3> cam;
			for (auto& p : points) cam.add(toCamera(p));

			Path path;
			bool started = false;
			auto addPoint = [&](Vec3 c)
			{
				Point<float> s = projectCamera(c);
				if (!started) path.startNewSubPath(s);
				else path.lineTo(s);
				started = true;
			};

			for (int i = 0; i < cam.size(); i++)
			{
				Vec3 a = cam[i], b = cam[(i + 1) % cam.size()];
				bool aIn = a.z >= nearDepth, bIn = b.z >= nearDepth;
				if (aIn) addPoint(a);
				if (aIn != bIn)
				{
					float t = (nearDepth - a.z) / (b.z - a.z);
					addPoint(a + (b - a) * t);
				}
			}
			if (started) path.closeSubPath();
			return path;
		}

		Path projectLine(Vec3 a, Vec3 b) const
		{
			Vec3 ca = toCamera(a), cb = toCamera(b);
			Path path;
			if (ca.z < nearDepth && cb.z < nearDepth) return path;
			if (ca.z < nearDepth) ca = ca + (cb - ca) * ((nearDepth - ca.z) / (cb.z - ca.z));
			if (cb.z < nearDepth) cb = cb + (ca - cb) * ((nearDepth - cb.z) / (ca.z - cb.z));
			path.startNewSubPath(projectCamera(ca));
			path.lineTo(projectCamera(cb));
			return path;
		}
	};

	Array<Vec3> getFloorOutline(const TopViewModel::ZoneView& z)
	{
		Array<Vec3> points;
		Rectangle<float> b = z.bounds;

		if (z.freeShape)
		{
			PathFlatteningIterator it(z.outline, AffineTransform(), .02f);
			while (it.next()) points.add(Vec3(it.x1, it.y1, 0));
			return points;
		}

		if (z.ellipse)
		{
			const int steps = 64;
			for (int i = 0; i < steps; i++)
			{
				float a = MathConstants<float>::twoPi * i / steps;
				points.add(Vec3(b.getCentreX() + std::cos(a) * b.getWidth() / 2, b.getCentreY() + std::sin(a) * b.getHeight() / 2, 0));
			}
			return points;
		}

		points.add(Vec3(b.getX(), b.getY(), 0));
		points.add(Vec3(b.getRight(), b.getY(), 0));
		points.add(Vec3(b.getRight(), b.getBottom(), 0));
		points.add(Vec3(b.getX(), b.getBottom(), 0));
		return points;
	}

	Array<Vec3> getFloorCircle(Point<float> centre, float radius)
	{
		Array<Vec3> points;
		const int steps = 24;
		for (int i = 0; i < steps; i++)
		{
			float a = MathConstants<float>::twoPi * i / steps;
			points.add(Vec3(centre.x + std::cos(a) * radius, centre.y + std::sin(a) * radius, 0));
		}
		return points;
	}
}

void Viewport3DRenderer::Camera::pan(float dx, float dy, float viewHeight)
{
	//the floor point under the target follows the mouse : right / forward on the floor, at the target depth
	float metersPerPixel = distance * 2 * std::tan(fov / 2) / jmax(1.0f, viewHeight);
	Vec3 right(std::cos(yaw), -std::sin(yaw), 0);
	Vec3 forward(-std::sin(yaw), -std::cos(yaw), 0);	// away from the camera, on the floor
	float forwardScale = 1.0f / jmax(.2f, std::sin(pitch));
	target = target - right * (dx * metersPerPixel) + forward * (dy * metersPerPixel * forwardScale);
}

void Viewport3DRenderer::paint(Graphics& g, const TopViewModel& m, Rectangle<float> area, Camera& camera)
{
	using namespace Viewport3DDetail;
	using namespace TrackingMonitorStyle;

	Graphics::ScopedSaveState state(g);
	g.reduceClipRegion(area.toNearestInt());
	g.setColour(sceneBg);
	g.fillRect(area);

	Point<float> size = m.snapshot.sceneSize;
	if (size.x <= 0 || size.y <= 0)
	{
		g.setColour(TEXT_COLOR.withAlpha(.5f));
		g.drawFittedText(SpatialModule::getNoSceneSizeText(), area.toNearestInt(), Justification::centred, 1);
		return;
	}

	if (!camera.fitted) camera.fit(size);
	View v(camera, area);

	//floor of the scene and its 1 m grid (thinned out on a big scene)
	Array<Vec3> floor = { Vec3(0, 0, 0), Vec3(size.x, 0, 0), Vec3(size.x, size.y, 0), Vec3(0, size.y, 0) };
	Path floorPath = v.projectPolygon(floor);
	g.setColour(Colour(0xff181818));
	g.fillPath(floorPath);

	float step = 1;
	while (jmax(size.x, size.y) / step > 40) step *= 2;
	g.setColour(Colours::white.withAlpha(.07f));
	for (float x = step; x < size.x; x += step) g.strokePath(v.projectLine(Vec3(x, 0, 0), Vec3(x, size.y, 0)), PathStrokeType(1));
	for (float y = step; y < size.y; y += step) g.strokePath(v.projectLine(Vec3(0, y, 0), Vec3(size.x, y, 0)), PathStrokeType(1));

	g.setColour(Colours::white.withAlpha(.35f));
	g.strokePath(floorPath, PathStrokeType(1.5f));

	//axes at the origin (top left corner of the top view) : x red, y green, z blue, 1 m
	const Vec3 origin;
	g.setColour(Colour(0xffe05a5a));
	g.strokePath(v.projectLine(origin, Vec3(1, 0, 0)), PathStrokeType(2));
	g.setColour(Colour(0xff6cc58b));
	g.strokePath(v.projectLine(origin, Vec3(0, 1, 0)), PathStrokeType(2));
	g.setColour(Colour(0xff5a8ee0));
	g.strokePath(v.projectLine(origin, Vec3(0, 0, 1)), PathStrokeType(2));

	//zones on the floor
	g.setFont(Font(12.0f));
	for (auto& z : m.zones)
	{
		if (z.hidden) continue;
		Colour c = z.enabled ? z.color : NORMAL_COLOR;
		Path p = v.projectPolygon(getFloorOutline(z));
		if (p.isEmpty()) continue;

		g.setColour(c.withAlpha(z.presence ? .4f : .18f));
		g.fillPath(p);
		g.setColour(z.selected ? HIGHLIGHT_COLOR : c.withAlpha(z.enabled ? .9f : .5f));
		g.strokePath(p, PathStrokeType(z.selected || z.presence ? 2.0f : 1.2f));

		Point<float> label;
		float depth;
		if (v.project(Vec3(z.bounds.getCentreX(), z.bounds.getCentreY(), 0), label, depth))
		{
			g.setColour(c.interpolatedWith(Colours::white, .5f));
			g.drawText(z.name, Rectangle<float>(160, 16).withCentre(label), Justification::centred, true);
		}
	}

	//people, the farthest first : shadow on the floor, body line, head
	struct Drawn { int index; float depth; };
	Array<Drawn> order;
	for (int i = 0; i < m.snapshot.persons.size(); i++)
	{
		auto& p = m.snapshot.persons.getReference(i);
		order.add({ i, v.toCamera(Vec3(p.position.x, p.position.y, 0)).z });
	}
	std::sort(order.begin(), order.end(), [](const Drawn& a, const Drawn& b) { return a.depth > b.depth; });

	for (auto& d : order)
	{
		auto& p = m.snapshot.persons.getReference(d.index);
		float height = p.height > .1f ? p.height : defaultPersonHeight;
		Vec3 foot(p.position.x, p.position.y, 0), head(p.position.x, p.position.y, height);

		int slotIndex = m.getSlotIndexForPerson(p.id);
		Colour c = slotIndex >= 0 ? m.slots[slotIndex].color : TopViewModel::getPersonColor(p.id);

		g.setColour(Colours::black.withAlpha(.45f));
		g.fillPath(v.projectPolygon(getFloorCircle(p.position, .3f)));
		g.setColour(c.withAlpha(.6f));
		g.strokePath(v.projectPolygon(getFloorCircle(p.position, .3f)), PathStrokeType(1.5f));

		Point<float> headScreen;
		float headDepth;
		if (!v.project(head, headScreen, headDepth)) continue;

		float ppm = v.pixelsPerMeter(headDepth);
		g.setColour(c.withAlpha(.85f));
		g.strokePath(v.projectLine(foot, Vec3(p.position.x, p.position.y, height - .12f)), PathStrokeType(jlimit(1.5f, 12.0f, .12f * ppm), PathStrokeType::curved, PathStrokeType::rounded));

		float headRadius = jlimit(3.0f, 30.0f, .14f * ppm);
		g.setColour(c);
		g.fillEllipse(Rectangle<float>(headRadius * 2, headRadius * 2).withCentre(headScreen));
		if (p.id == m.selectedPersonId)
		{
			g.setColour(HIGHLIGHT_COLOR);
			g.drawEllipse(Rectangle<float>(headRadius * 2 + 6, headRadius * 2 + 6).withCentre(headScreen), 2);
		}

		String label = slotIndex >= 0 ? m.slots[slotIndex].name + " #" + String(p.id) : "#" + String(p.id);
		g.setColour(TEXT_COLOR);
		g.setFont(Font(12.0f));
		g.drawText(label, Rectangle<float>(140, 16).withCentre(headScreen.translated(0, -headRadius - 10)), Justification::centred, false);
	}

	g.setColour(dimText);
	g.setFont(Font(11.0f));
	g.drawText("Drag : orbit" + sep + "Right drag : pan" + sep + "Wheel : zoom" + sep + "Double click : reset",
		area.reduced(8, 4).removeFromBottom(16), Justification::bottomLeft, true);
}
