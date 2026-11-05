/*
  ==============================================================================

	Viewport3DRenderer.cpp
	Created: 5 Nov 2026
	Author:  DrawZone

  ==============================================================================
*/

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

namespace Viewport3DDetail
{
	//scene 2D meters (x, y) and height above it -> world : Position + Rotation (X, then Y, then Z)
	struct SceneTransform
	{
		Vec3 origin;
		Vec3 ax, ay, az;	// world directions of the scene x, y and height

		SceneTransform() : ax(1, 0, 0), ay(0, 1, 0), az(0, 0, 1) {}

		explicit SceneTransform(const TopViewModel::SceneView& s)
		{
			origin = Vec3(s.position[0], s.position[1], s.position[2]);
			const float d = MathConstants<float>::pi / 180;
			float cx = std::cos(s.rotation[0] * d), sx = std::sin(s.rotation[0] * d);
			float cy = std::cos(s.rotation[1] * d), sy = std::sin(s.rotation[1] * d);
			float cz = std::cos(s.rotation[2] * d), sz = std::sin(s.rotation[2] * d);
			auto rotate = [&](Vec3 v)
			{
				v = Vec3(v.x, v.y * cx - v.z * sx, v.y * sx + v.z * cx);	// X
				v = Vec3(v.x * cy + v.z * sy, v.y, -v.x * sy + v.z * cy);	// Y
				return Vec3(v.x * cz - v.y * sz, v.x * sz + v.y * cz, v.z);	// Z
			};
			ax = rotate(Vec3(1, 0, 0));
			ay = rotate(Vec3(0, 1, 0));
			az = rotate(Vec3(0, 0, 1));
		}

		Vec3 apply(float x, float y, float h = 0) const { return origin + ax * x + ay * y + az * h; }
		Vec3 apply(Point<float> p, float h = 0) const { return apply(p.x, p.y, h); }

		Array<Vec3> apply(const Array<Point<float>>& points) const
		{
			Array<Vec3> out;
			for (auto& p : points) out.add(apply(p));
			return out;
		}
	};

	Array<Point<float>> getZoneOutline(const TopViewModel::ZoneView& z)
	{
		Array<Point<float>> points;
		Rectangle<float> b = z.bounds;

		if (z.freeShape)
		{
			PathFlatteningIterator it(z.outline, AffineTransform(), .02f);
			while (it.next()) points.add(Point<float>(it.x1, it.y1));
			return points;
		}

		if (z.ellipse)
		{
			const int steps = 64;
			for (int i = 0; i < steps; i++)
			{
				float a = MathConstants<float>::twoPi * i / steps;
				points.add(Point<float>(b.getCentreX() + std::cos(a) * b.getWidth() / 2, b.getCentreY() + std::sin(a) * b.getHeight() / 2));
			}
			return points;
		}

		points.add(b.getTopLeft());
		points.add(b.getTopRight());
		points.add(b.getBottomRight());
		points.add(b.getBottomLeft());
		return points;
	}

	Array<Point<float>> getCircle(Point<float> centre, float radius)
	{
		Array<Point<float>> points;
		const int steps = 24;
		for (int i = 0; i < steps; i++)
		{
			float a = MathConstants<float>::twoPi * i / steps;
			points.add(Point<float>(centre.x + std::cos(a) * radius, centre.y + std::sin(a) * radius));
		}
		return points;
	}

	SceneTransform getTransform(const TopViewModel& m, const void* sceneId)
	{
		for (auto& s : m.scenes) if (s.id == sceneId) return SceneTransform(s);
		return SceneTransform();
	}
}

void Viewport3DRenderer::Camera::fit(Vec3 min, Vec3 max)
{
	target = (min + max) * .5f;
	yaw = 0;
	pitch = .9f;
	float diagonal = jmax(1.0f, (max - min).length());
	distance = diagonal * .5f / std::tan(fov / 2) * 1.25f;
	fitted = true;
}

void Viewport3DRenderer::paint(Graphics& g, const TopViewModel& m, Rectangle<float> area, Camera& camera, int highlightedScene)
{
	using namespace Viewport3DDetail;
	using namespace TrackingMonitorStyle;

	Graphics::ScopedSaveState state(g);
	g.reduceClipRegion(area.toNearestInt());
	g.setColour(sceneBg);
	g.fillRect(area);

	//scenes with a size : framed together at the first paint (or after a double click)
	Vec3 min(1e9f, 1e9f, 1e9f), max(-1e9f, -1e9f, -1e9f);
	int numSized = 0;
	for (auto& s : m.scenes)
	{
		if (s.size.x <= 0 || s.size.y <= 0) continue;
		numSized++;
		SceneTransform t(s);
		for (Vec3 c : { t.apply(0, 0), t.apply(s.size.x, 0), t.apply(s.size.x, s.size.y), t.apply(0, s.size.y) })
		{
			min = Vec3(jmin(min.x, c.x), jmin(min.y, c.y), jmin(min.z, c.z));
			max = Vec3(jmax(max.x, c.x), jmax(max.y, c.y), jmax(max.z, c.z));
		}
	}

	if (numSized == 0)
	{
		g.setColour(TEXT_COLOR.withAlpha(.5f));
		g.drawFittedText(SpatialModule::getNoSceneSizeText(), area.toNearestInt(), Justification::centred, 1);
		return;
	}

	if (!camera.fitted) camera.fit(min, max);
	View v(camera, area);

	//axes at the world origin : x red, y green, z blue, 1 m
	const Vec3 origin;
	g.setColour(Colour(0xffe05a5a));
	g.strokePath(v.projectLine(origin, Vec3(1, 0, 0)), PathStrokeType(2));
	g.setColour(Colour(0xff6cc58b));
	g.strokePath(v.projectLine(origin, Vec3(0, 1, 0)), PathStrokeType(2));
	g.setColour(Colour(0xff5a8ee0));
	g.strokePath(v.projectLine(origin, Vec3(0, 0, 1)), PathStrokeType(2));

	//each scene : its plane, its 1 m grid (thinned out on a big scene), its name, its zones
	for (int si = 0; si < m.scenes.size(); si++)
	{
		auto& s = m.scenes.getReference(si);
		if (s.size.x <= 0 || s.size.y <= 0) continue;
		SceneTransform t(s);
		bool highlighted = si == highlightedScene;

		Array<Point<float>> corners;
		corners.add({ 0, 0 });
		corners.add({ s.size.x, 0 });
		corners.add(s.size);
		corners.add({ 0, s.size.y });
		Path plane = v.projectPolygon(t.apply(corners));
		g.setColour(Colour(0xff181818).interpolatedWith(s.color, highlighted ? .12f : .05f));
		g.fillPath(plane);

		float step = 1;
		while (jmax(s.size.x, s.size.y) / step > 40) step *= 2;
		g.setColour(Colours::white.withAlpha(.07f));
		for (float x = step; x < s.size.x; x += step) g.strokePath(v.projectLine(t.apply(x, 0), t.apply(x, s.size.y)), PathStrokeType(1));
		for (float y = step; y < s.size.y; y += step) g.strokePath(v.projectLine(t.apply(0, y), t.apply(s.size.x, y)), PathStrokeType(1));

		g.setColour(s.color.withAlpha(highlighted ? .9f : .5f));
		g.strokePath(plane, PathStrokeType(highlighted ? 2.0f : 1.5f));

		Point<float> label;
		float depth;
		if (v.project(t.apply(0, 0), label, depth))
		{
			g.setFont(Font(13.0f, Font::bold));
			g.setColour(s.color);
			g.drawText(s.name, Rectangle<float>(label.x + 4, label.y - 18, 200, 16), Justification::centredLeft, true);
		}

		g.setFont(Font(12.0f));
		for (auto& z : m.zones)
		{
			if (z.hidden || z.sceneId != s.id) continue;
			Colour c = z.enabled ? z.color : NORMAL_COLOR;
			Path p = v.projectPolygon(t.apply(getZoneOutline(z)));
			if (p.isEmpty()) continue;

			g.setColour(c.withAlpha(z.presence ? .4f : .18f));
			g.fillPath(p);
			g.setColour(z.selected ? HIGHLIGHT_COLOR : c.withAlpha(z.enabled ? .9f : .5f));
			g.strokePath(p, PathStrokeType(z.selected || z.presence ? 2.0f : 1.2f));

			if (v.project(t.apply(z.bounds.getCentre()), label, depth))
			{
				g.setColour(c.interpolatedWith(Colours::white, .5f));
				g.drawText(z.name, Rectangle<float>(160, 16).withCentre(label), Justification::centred, true);
			}
		}
	}

	//people, the farthest first : flat on their scene, circle and cross on the centroid
	struct Drawn { int index; float depth; };
	Array<Drawn> order;
	for (int i = 0; i < m.snapshot.persons.size(); i++)
	{
		auto& p = m.snapshot.persons.getReference(i);
		order.add({ i, v.toCamera(getTransform(m, p.scene).apply(p.position)).z });
	}
	std::sort(order.begin(), order.end(), [](const Drawn& a, const Drawn& b) { return a.depth > b.depth; });

	for (auto& d : order)
	{
		auto& p = m.snapshot.persons.getReference(d.index);
		SceneTransform t = getTransform(m, p.scene);

		int slotIndex = m.getSlotIndexForPerson(p.id);
		Colour c = slotIndex >= 0 ? m.slots[slotIndex].color : TopViewModel::getPersonColor(p.id);
		bool selected = p.id == m.selectedPersonId;

		//circle on the scene plane, cross on the centroid of the detection
		const float radius = .3f, cross = .15f;
		Path circle = v.projectPolygon(t.apply(getCircle(p.position, radius)));
		g.setColour(c.withAlpha(.25f));
		g.fillPath(circle);
		g.setColour(selected ? HIGHLIGHT_COLOR : c);
		g.strokePath(circle, PathStrokeType(selected ? 3.0f : 2.0f));

		g.setColour(c);
		g.strokePath(v.projectLine(t.apply(p.position.translated(-cross, 0)), t.apply(p.position.translated(cross, 0))), PathStrokeType(2));
		g.strokePath(v.projectLine(t.apply(p.position.translated(0, -cross)), t.apply(p.position.translated(0, cross))), PathStrokeType(2));

		//label just past the circle, along the scene height
		Point<float> labelScreen;
		float labelDepth;
		if (!v.project(t.apply(p.position, radius), labelScreen, labelDepth)) continue;

		String label = slotIndex >= 0 ? m.slots[slotIndex].name + " #" + String(p.id) : "#" + String(p.id);
		g.setColour(TEXT_COLOR);
		g.setFont(Font(12.0f));
		g.drawText(label, Rectangle<float>(140, 16).withCentre(labelScreen.translated(0, -10)), Justification::centred, false);
	}

	g.setColour(dimText);
	g.setFont(Font(11.0f));
	g.drawText("Drag : orbit" + sep + "Right drag : pan" + sep + "Wheel : zoom" + sep + "Double click : reset",
		area.reduced(8, 4).removeFromBottom(16), Justification::bottomRight, true);
}
