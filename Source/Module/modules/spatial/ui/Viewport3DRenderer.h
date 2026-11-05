/*
  ==============================================================================

	Viewport3DRenderer.h
	Created: 5 Nov 2026
	Author:  DrawZone

	Read-only 3D view of a Spatial module (surface floor, zones, people),
	drawn with JUCE Graphics (no OpenGL) from the same TopViewModel as the
	top view. Surface meters : x right, y towards the front of the top view,
	z up. The camera orbits around a target point on the floor.

  ==============================================================================
*/

#pragma once

class Viewport3DRenderer
{
public:
	struct Vec3
	{
		float x = 0, y = 0, z = 0;
		Vec3() {}
		Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
		Vec3 operator+(Vec3 v) const { return { x + v.x, y + v.y, z + v.z }; }
		Vec3 operator-(Vec3 v) const { return { x - v.x, y - v.y, z - v.z }; }
		Vec3 operator*(float k) const { return { x * k, y * k, z * k }; }
		float dot(Vec3 v) const { return x * v.x + y * v.y + z * v.z; }
		Vec3 cross(Vec3 v) const { return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x }; }
		float length() const { return std::sqrt(dot(*this)); }
		Vec3 normalised() const { float l = length(); return l > 0 ? *this * (1.0f / l) : *this; }
	};

	//orbit camera : yaw around the vertical axis (0 = from the front edge of the surface), pitch above the floor
	struct Camera
	{
		float yaw = 0;			// radians
		float pitch = .9f;		// radians, from minPitch (grazing) to maxPitch (top view)
		float distance = 10;	// meters from the target
		Vec3 target;			// meters
		bool fitted = false;	// false : framed on the surface at the next paint

		static constexpr float fov = .9f;	// vertical field of view, radians
		static constexpr float minPitch = .05f;
		static constexpr float maxPitch = 1.55f;

		void fit(Vec3 min, Vec3 max);	// default view of this box (all the surfaces)
		void orbit(float dx, float dy);		// mouse pixels
		void pan(float dx, float dy, float viewHeight);
		void zoom(float factor);			// > 1 : closer
	};

	//m : all the surfaces (TopViewModel::update with -1) ; highlightedSurface : index in m.surfaces, -1 = none
	static void paint(Graphics& g, const TopViewModel& m, Rectangle<float> area, Camera& camera, int highlightedSurface = -1);
};
