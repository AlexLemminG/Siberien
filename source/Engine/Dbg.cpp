#include "Dbg.h"
#include "Common.h"
#include "debugdraw/debugdraw.h"


std::vector<Ray> Dbg::rays;
std::vector<Dbg::Point> Dbg::points;
std::vector<std::string> Dbg::texts;

void Dbg::Draw(Ray ray) {
	rays.push_back(ray);
}

void Dbg::Draw(Vector3 point, float radius) {
	points.push_back(Point{ point, radius });
}

void Dbg::Init() {
	bgfx_examples::ddInit();
}

void Dbg::ClearAll(){
	rays.clear();
	points.clear();
	texts.clear();
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
		dde.drawOrb(point.pos.x, point.pos.y, point.pos.z, point.radius);
	}

	int yTextOffset = 3;
	for (int i = 0; i < texts.size(); i++) {
		bgfx::dbgTextPrintf(1, yTextOffset + i, 0x0f, texts[i].c_str());
	}

	dde.drawGrid(bx::Vec3(0, 1, 0), bx::Vec3(0, 0, 0));

	ClearAll();

	dde.end();
}
