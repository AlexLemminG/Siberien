#pragma once

#include "Common.h"
#include "SMath.h"
#include <vector>
#include <string>
#include "SMath.h"

class SE_CPP_API Dbg {
public:
	static void Draw(Ray ray, float length = 1.f, Color color = Colors::white);
	static void Draw(Vector3 point, float radius = 0.1f);
	static void DrawLine(Vector3 from, Vector3 to, Color color = Colors::white);
	static void Draw(const Sphere& sphere);
	static void Draw(const AABB& aabb);
	static void Draw(Frustum aabb);
	static void Draw(Matrix4 axes, float length = 1.0f);
	static void Text(std::string text);

	template<typename ... Args>
	static void Text(const std::string& format, Args ... args)
	{
		std::string str = FormatString(format, args...);
		Text(str);
	}


	static void Init();
	static void Term();

	static void DrawAll();

public:
	class Point {
	public:
		Vector3 pos;
		float radius;
	};
	class Axes {
	public:
		Matrix4 matr;
		float length;
	};
	class Ray {
	public:
		::Ray ray;
		float length;
		Color color;
	};

	static void ClearAll();
};