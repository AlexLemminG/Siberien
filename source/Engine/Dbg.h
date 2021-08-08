#pragma once

#include "Common.h"
#include "Math.h"
#include <vector>
#include <string>

class Dbg {
public:
	static void Draw(Ray ray);
	static void Draw(Vector3 point, float radius = 0.1f);
	static void Text(std::string text);

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

	static void ClearAll();

	static std::vector<std::string> texts;
	static std::vector<Ray> rays;
	static std::vector<Point> points;
};