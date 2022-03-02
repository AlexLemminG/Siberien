#include "Dbg.h"
#include "Common.h"
#include "debugdraw/debugdraw.h"


static std::vector<Dbg::Ray> rays;
static std::vector<Dbg::Point> points;
static std::vector<Dbg::Cone> cones;
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


Dbg::DrawHandle& Dbg::DrawCone(Vector3 from, Vector3 to, float radius) {
	Cone c = Cone{ from, to, radius };
	cones.push_back(c);
	return cones.back();
}

void Dbg::Draw(Frustum frustum) {
	float depthStart = bgfx::getCaps()->homogeneousDepth ? -1 : 0;
	auto inv = frustum.GetMatrix().Inverse();
	AABB aabb = AABB(Vector3(-1, -1, depthStart), Vector3(1, 1, 1));
	auto vertices = aabb.GetVertices();
	auto tris = aabb.GetQuadIndices();

	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			int i1 = i * 4 + j;
			int i2 = i * 4 + (j + 1) % 4;
			Dbg::DrawLine(inv * vertices[tris[i1]], inv * vertices[tris[i2]]);
		}
	}
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
	cones.clear();
}
void Dbg::Term() {
	ClearAll();

	bgfx_examples::ddShutdown();
}

void Dbg::DrawAllGizmos(void* ddeVoid, float alphaMultiplier) {
	bgfx_examples::DebugDrawEncoder& dde = *(bgfx_examples::DebugDrawEncoder*)ddeVoid;
	for (const auto& ray : rays) {
		Color color = ray.color;
		color.a *= alphaMultiplier;
		dde.setColor(color.ToIntARGB());
		dde.moveTo(ray.ray.origin.x, ray.ray.origin.y, ray.ray.origin.z);
		auto nextPos = ray.ray.GetPoint(ray.length);
		dde.lineTo(bx::Vec3(nextPos.x, nextPos.y, nextPos.z));
	}
	dde.setColor(Colors::white.ToIntARGB());

	dde.setWireframe(true);
	for (const auto& point : points) {
		Color color = point.color;
		color.a *= alphaMultiplier;
		dde.setColor(color.ToIntARGB());

		float weight = 0.f;
		dde.drawCircle(bx::Vec3(0, 0, 1), bx::Vec3(point.pos.x, point.pos.y, point.pos.z), point.radius, weight);

		dde.drawCircle(bx::Vec3(0, 1, 0), bx::Vec3(point.pos.x, point.pos.y, point.pos.z), point.radius, weight);

		dde.drawCircle(bx::Vec3(1, 0, 0), bx::Vec3(point.pos.x, point.pos.y, point.pos.z), point.radius, weight);
	}

	dde.setLod(3);
	for (const auto& cone : cones) {
		Color color = cone.color;
		color.a *= alphaMultiplier;
		dde.setColor(color.ToIntARGB());
		dde.drawCone(bx::Vec3(cone.to.x, cone.to.y, cone.to.z), bx::Vec3(cone.from.x, cone.from.y, cone.from.z), cone.radius);
	}
	dde.setLod(0);
}

void Dbg::DrawAll(int viewId) {
	OPTICK_EVENT();
	bgfx_examples::DebugDrawEncoder dde;
	dde.begin(viewId);

	//drawing with bigger opacity without ztest
	dde.setState(false, false, true);
	DrawAllGizmos(&dde, 0.1f);

	//drawing with ztest/zwrite
	dde.setState(true, true, true);
	DrawAllGizmos(&dde, 1.f);

	dde.setColor(Colors::white.ToIntARGB());

	dde.setWireframe(false);
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
