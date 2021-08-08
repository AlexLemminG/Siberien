#include "Dbg.h"
#include "debugdraw/debugdraw.h"


std::vector<Ray> Dbg::rays;
std::vector<Vector3> Dbg::points;

void Dbg::Draw(Ray ray) {
	rays.push_back(ray);
}

void Dbg::Draw(Vector3 point) {
	points.push_back(point);
}

void Dbg::Init() {
	bgfx_examples::ddInit();
}

void Dbg::ClearAll(){
	rays.clear();
	points.clear();
}
void Dbg::Term() {
	ClearAll();

	bgfx_examples::ddShutdown();
}

void Dbg::DrawAll() {
	bgfx_examples::DebugDrawEncoder dde;
	dde.begin(0);
	for (const auto& ray : rays) {
		dde.moveTo(ray.origin.x, ray.origin.y, ray.origin.z);
		auto nextPos = ray.GetPoint(100);
		dde.lineTo(bx::Vec3(nextPos.x, nextPos.y, nextPos.z));
	}

	for (const auto& point: points) {
		dde.drawOrb(point.x, point.y, point.z, 0.1f);
	}

	dde.drawGrid(bx::Vec3(0, 1, 0), bx::Vec3(0, 0, 0));

	ClearAll();

	dde.end();
}
