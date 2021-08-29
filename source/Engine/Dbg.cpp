#include "Dbg.h"
#include "Common.h"
#include "debugdraw/debugdraw.h"


std::vector<Dbg::Ray> Dbg::rays;
std::vector<Dbg::Point> Dbg::points;
std::vector<Dbg::Axes> Dbg::axes;
std::vector<std::string> Dbg::texts;

void Dbg::Draw(::Ray ray, float length, Color color) {
	rays.push_back(Ray{ ray, length, color });
}

void Dbg::Draw(Vector3 point, float radius) {
	points.push_back(Point{ point, radius });
}
void Dbg::Draw(const Sphere& sphere) {
	Draw(sphere.pos, sphere.radius);
}
void Dbg::DrawLine(Vector3 from, Vector3 to, Color color) {
	Draw(::Ray(from, to - from), Vector3::Distance(from, to), color);
}


void Dbg::Draw(const AABB& aabb) {
	Vector3 dirX = Vector3(aabb.max.x - aabb.min.x, 0.f, 0.f);
	Vector3 dirY = Vector3(0.f, aabb.max.y - aabb.min.y, 0.f);
	Vector3 dirZ = Vector3(0.f, 0.f, aabb.max.z - aabb.min.z);

	Dbg::DrawLine(aabb.min, aabb.min + dirX);
	Dbg::DrawLine(aabb.min + dirY, aabb.min + dirY + dirX);
	Dbg::DrawLine(aabb.min + dirZ, aabb.min + dirZ + dirX);
	Dbg::DrawLine(aabb.min + dirY + dirZ, aabb.min + dirY + dirZ + dirX);

	Dbg::DrawLine(aabb.min, aabb.min + dirZ);
	Dbg::DrawLine(aabb.min + dirY, aabb.min + dirY + dirZ);
	Dbg::DrawLine(aabb.min + dirX, aabb.min + dirX + dirZ);
	Dbg::DrawLine(aabb.min + dirY + dirX, aabb.min + dirY + dirX + dirZ);

	Dbg::DrawLine(aabb.min, aabb.min + dirY);
	Dbg::DrawLine(aabb.min + dirZ, aabb.min + dirZ + dirY);
	Dbg::DrawLine(aabb.min + dirX, aabb.min + dirX + dirY);
	Dbg::DrawLine(aabb.min + dirZ + dirX, aabb.min + dirZ + dirX + dirY);
}

void Dbg::Draw(Matrix4 matr, float length) {
	Draw(::Ray(GetPos(matr), GetRot(matr) * Vector3_up), length, Colors::green);
	Draw(::Ray(GetPos(matr), GetRot(matr) * Vector3_right), length, Colors::red);
	Draw(::Ray(GetPos(matr), GetRot(matr) * Vector3_forward), length, Colors::blue);
}

void Dbg::Init() {
	bgfx_examples::ddInit();
}

void Dbg::ClearAll() {
	rays.clear();
	points.clear();
	texts.clear();
	axes.clear();
}
void Dbg::Term() {
	ClearAll();

	bgfx_examples::ddShutdown();
}

void Dbg::DrawAll() {
	OPTICK_EVENT();
	bgfx_examples::DebugDrawEncoder dde;
	dde.begin(0);

	dde.setState(false, false, true);

	for (const auto& ray : rays) {
		dde.setColor(ray.color.ToIntARGB());
		dde.moveTo(ray.ray.origin.x, ray.ray.origin.y, ray.ray.origin.z);
		auto nextPos = ray.ray.GetPoint(ray.length);
		dde.lineTo(bx::Vec3(nextPos.x, nextPos.y, nextPos.z));
	}
	dde.setColor(Colors::white.ToIntARGB());

	for (const auto& point : points) {
		dde.drawOrb(point.pos.x, point.pos.y, point.pos.z, point.radius);
	}

	int yTextOffset = 3;
	for (int i = 0; i < texts.size(); i++) {
		bgfx::dbgTextPrintf(1, yTextOffset + i, 0x0f, texts[i].c_str());
	}

	ClearAll();

	dde.end();
}
