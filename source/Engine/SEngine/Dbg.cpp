#include "Dbg.h"
#include "Common.h"
#include "debugdraw/debugdraw.h"


static std::vector<Dbg::Ray> rays;
static std::vector<Dbg::Point> points;
static std::vector<Dbg::Axes> axes;
static std::vector<std::string> texts;

void Dbg::Draw(::Ray ray, float length, Color color) {
	rays.push_back(Ray{ ray, length, color });
}

Dbg::DrawHandle& Dbg::Draw(Vector3 point, float radius) {
	Point p = Point{ point, radius };
	points.push_back(p);
	return points.back();
}
Dbg::DrawHandle& Dbg::Draw(const Sphere& sphere) {
	return Draw(sphere.pos, sphere.radius);
}
void Dbg::DrawLine(Vector3 from, Vector3 to, Color color) {
	Draw(::Ray(from, to - from), Vector3::Distance(from, to), color);
}

void Dbg::Draw(Frustum frustum) {
	auto inv = frustum.GetMatrix().Inverse();

	Vector3 a1 = inv * Vector3(-1, +1, -1);
	Vector3 a2 = inv * Vector3(-1, +1, +1);
	Vector3 a3 = inv * Vector3(+1, +1, -1);
	Vector3 a4 = inv * Vector3(+1, +1, +1);

	Vector3 a5 = inv * Vector3(-1, -1, -1);
	Vector3 a6 = inv * Vector3(-1, -1, +1);
	Vector3 a7 = inv * Vector3(+1, -1, -1);
	Vector3 a8 = inv * Vector3(+1, -1, +1);

	//top
	Dbg::DrawLine(a1, a2);
	Dbg::DrawLine(a2, a3);
	Dbg::DrawLine(a3, a4);
	Dbg::DrawLine(a4, a1);
	
	//bottom
	Dbg::DrawLine(a5, a6);
	Dbg::DrawLine(a6, a7);
	Dbg::DrawLine(a7, a8);
	Dbg::DrawLine(a8, a5);

	//sides
	Dbg::DrawLine(a1, a5);
	Dbg::DrawLine(a2, a6);
	Dbg::DrawLine(a3, a7);
	Dbg::DrawLine(a4, a8);
}

void Dbg::Draw(const AABB& aabb) {
	OBB obb = aabb.ToOBB();
	Draw(obb);
}

void Dbg::Draw(const OBB& obb) {
	auto rot = Matrix4::ToRotationMatrix(obb.GetCenterMatrix());
	Vector3 dirX = rot * Vector3(obb.GetSize().x, 0.f, 0.f);
	Vector3 dirY = rot * Vector3(0.f, obb.GetSize().y, 0.f);
	Vector3 dirZ = rot * Vector3(0.f, 0.f, obb.GetSize().z);

	auto min = obb.GetCenterMatrix() * (-obb.GetSize() / 2.f);

	Dbg::DrawLine(min, min + dirX);
	Dbg::DrawLine(min + dirY, min + dirY + dirX);
	Dbg::DrawLine(min + dirZ, min + dirZ + dirX);
	Dbg::DrawLine(min + dirY + dirZ, min + dirY + dirZ + dirX);

	Dbg::DrawLine(min, min + dirZ);
	Dbg::DrawLine(min + dirY, min + dirY + dirZ);
	Dbg::DrawLine(min + dirX, min + dirX + dirZ);
	Dbg::DrawLine(min + dirY + dirX, min + dirY + dirX + dirZ);

	Dbg::DrawLine(min, min + dirY);
	Dbg::DrawLine(min + dirZ, min + dirZ + dirY);
	Dbg::DrawLine(min + dirX, min + dirX + dirY);
	Dbg::DrawLine(min + dirZ + dirX, min + dirZ + dirX + dirY);
}

void Dbg::Draw(Matrix4 matr, float length) {
	Draw(::Ray(GetPos(matr), GetRot(matr) * Vector3_up), length, Colors::green);
	Draw(::Ray(GetPos(matr), GetRot(matr) * Vector3_right), length, Colors::red);
	Draw(::Ray(GetPos(matr), GetRot(matr) * Vector3_forward), length, Colors::blue);
}

void Dbg::Text(std::string text) {
	texts.push_back(text);
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
	dde.begin(1);

	dde.setState(false, false, true);

	for (const auto& ray : rays) {
		dde.setColor(ray.color.ToIntARGB());
		dde.moveTo(ray.ray.origin.x, ray.ray.origin.y, ray.ray.origin.z);
		auto nextPos = ray.ray.GetPoint(ray.length);
		dde.lineTo(bx::Vec3(nextPos.x, nextPos.y, nextPos.z));
	}
	dde.setColor(Colors::white.ToIntARGB());

	dde.setWireframe(true);
	for (const auto& point : points) {
		dde.setColor(point.color.ToIntARGB());

		float weight = 0.f;
		dde.drawCircle(bx::Vec3(0, 0, 1), bx::Vec3(point.pos.x, point.pos.y, point.pos.z), point.radius, weight);

		dde.drawCircle(bx::Vec3(0, 1, 0), bx::Vec3(point.pos.x, point.pos.y, point.pos.z), point.radius, weight);

		dde.drawCircle(bx::Vec3(1, 0, 0), bx::Vec3(point.pos.x, point.pos.y, point.pos.z), point.radius, weight);
		//dde.drawOrb(point.pos.x, point.pos.y, point.pos.z, point.radius);
	}
	dde.setWireframe(false);
	dde.setColor(Colors::white.ToIntARGB());

	int yTextOffset = 3;
	for (int i = 0; i < texts.size(); i++) {
		bgfx::dbgTextPrintf(1, yTextOffset + i, 0x0f, texts[i].c_str());
	}

	ClearAll();

	dde.end();
}

Dbg::DrawHandle& Dbg::DrawHandle::SetColor(const Color& color) {
	this->color = color;
	return *this;
}
