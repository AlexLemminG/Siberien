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
	static void Draw(Matrix4 axes, float length = 1.0f);
	static void Text(std::string text) {
		texts.push_back(text);
	}

	template<typename ... Args>
	static void Text(const std::string& format, Args ... args)
	{
		std::string str = FormatString(format, args...);
		texts.push_back(str);
	}


	static void Init();
	static void Term();

	static void DrawAll();

private:
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

	static std::vector<std::string> texts;
	static std::vector<Ray> rays;
	static std::vector<Point> points;
	static std::vector<Axes> axes;
};