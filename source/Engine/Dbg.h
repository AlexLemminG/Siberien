#pragma once

#include "Math.h"
#include <vector>

class Dbg {
public:
	static void Draw(Ray ray);
	static void Draw(Vector3 point);

	static void Init();
	static void Term();

	static void DrawAll();

private:
	static void ClearAll();

	static std::vector<Ray> rays;
	static std::vector<Vector3> points;
};