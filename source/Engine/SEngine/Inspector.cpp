

#include "Common.h"
#include "Reflect.h"
#include "Serialization.h"
#include <dear-imgui/imgui.h>
#include "System.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneManager.h"
#include "GameEvents.h"
#include "Resources.h"
#include "Camera.h"
#include "Input.h"
#include "Graphics.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Light.h"
#include "Dbg.h"
#include "bx/bx.h"
#include <bgfx_utils.h>
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "MeshCollider.h"
#include "PhysicsSystem.h"
#include "Physics.h"
#include "dear-imgui/widgets/gizmo.h"

//TODO to utils
Sphere GetSphere(std::shared_ptr<GameObject> go) {
	bool hasSphere = false;
	Sphere sphere;
	auto renderer = go->GetComponent<MeshRenderer>();
	if (renderer) {
		auto meshSphere = renderer->mesh->boundingSphere;
		auto scale = renderer->m_transform->GetScale();
		float maxScale = Mathf::Max(Mathf::Max(scale.x, scale.y), scale.z);
		meshSphere.radius *= maxScale;
		meshSphere.pos = renderer->m_transform->GetMatrix() * meshSphere.pos;

		hasSphere = true;
		sphere = meshSphere;
	}

	auto pointLight = go->GetComponent<PointLight>();
	if (pointLight) {
		Sphere lightSphere;
		lightSphere.pos = go->transform()->GetPosition();
		lightSphere.radius = pointLight->radius;

		if (hasSphere) {
			sphere = Sphere::Combine(sphere, lightSphere);
		}
		else {
			hasSphere = true;
			sphere = lightSphere;
		}
	}

	if (hasSphere) {
		return sphere;
	}
	else {
		return Sphere(go->transform()->GetPosition(), 0.f);
	}
}


OBB GetOBB(std::shared_ptr<GameObject> go) {
	auto renderer = go->GetComponent<MeshRenderer>();
	if (renderer) {
		auto aabb = renderer->mesh->aabb;
		auto obb = renderer->m_transform->GetMatrix() * aabb.ToOBB();
		return obb;
	}

	auto sphere = GetSphere(go);
	return sphere.ToAABB().ToOBB();
}

AABB GetAABB(std::shared_ptr<GameObject> go) {
	return GetOBB(go).ToAABB();
}

bool RaycastExact(std::shared_ptr<GameObject> go, Ray ray) {
	float maxDistance = 100000.f;
	if (!IsOverlapping(GetSphere(go), ray)) {
		return false;
	}
	auto meshRenderer = go->GetComponent<MeshRenderer>();
	if (meshRenderer) {
		auto mesh = meshRenderer->mesh;
		if (mesh) {
			auto collider = MeshColliderStorageSystem::Get()->GetStored(mesh);
			if (collider) {
				auto scaledShape = std::make_shared<btScaledBvhTriangleMeshShape>(collider.get(), btConvert(go->transform()->GetScale()));
				auto info = btRigidBody::btRigidBodyConstructionInfo(0, nullptr, scaledShape.get());
				auto matr = go->transform()->GetMatrix();
				SetScale(matr, Vector3_one);//TODO optimize
				info.m_startWorldTransform = btConvert(matr);
				auto* rb = new btRigidBody(info);

				PhysicsSystem::Get()->dynamicsWorld->addRigidBody(rb);

				Physics::RaycastHit hit;
				bool overlapping = false;
				if (Physics::Raycast(hit, ray, maxDistance)) {
					overlapping = true;
				}

				PhysicsSystem::Get()->dynamicsWorld->removeRigidBody(rb);

				delete rb;
				if (overlapping) {
					return true;
				}
			}
		}
	}
	//TODO not only point light
	auto pointLight = go->GetComponent<PointLight>();
	if (pointLight) {
		auto center = pointLight->gameObject()->transform()->GetPosition();
		auto radius = pointLight->radius;
		if (IsOverlapping(Sphere(center, radius), ray)) {
			return true;
		}
	}
	return false;
}

std::vector<std::shared_ptr<GameObject>> GetObjectsUnderCursor() {
	float maxDistance = 1000.f;
	auto mouseRay = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
	std::vector<std::shared_ptr<GameObject>> result;
	for (auto obj : Scene::Get()->GetAllGameObjects()) {
		if (Bits::IsMaskTrue(obj->flags, GameObject::FLAGS::IS_HIDDEN_IN_INSPECTOR)) {
			continue;
		}
		if (RaycastExact(obj, mouseRay)) {
			result.push_back(obj);
		}
	}
	Vector3 cameraPos = mouseRay.origin;
	std::sort(result.begin(), result.end(), [cameraPos](std::shared_ptr<GameObject> x, std::shared_ptr<GameObject> y) {
		auto xPos = GetOBB(x).GetCenter();
		auto yPos = GetOBB(y).GetCenter();
		//TODO exact raycast hit results
		return (cameraPos - xPos).LengthSquared() < (cameraPos - yPos).LengthSquared();
		});
	return std::move(result);
}

class InspectorWindow : public System<InspectorWindow> {
	GameEventHandle onSceneLoadedHandle;

	std::shared_ptr<Object> selectedObject;

	int nextSelectedObjectIndex = 0;
	Vector2 prevMousePos;
public:
	std::shared_ptr<Camera> editorCamera;

	virtual bool Init() override {
		onSceneLoadedHandle = SceneManager::onSceneLoaded.Subscribe([this]() {
			auto cameraPrefab = AssetDatabase::Get()->Load<GameObject>("engine\\editorCamera.asset");
			if (!cameraPrefab) {
				return;
			}
			auto camera = Object::Instantiate(cameraPrefab);
			camera->flags = Bits::SetMaskTrue(camera->flags, GameObject::FLAGS::IS_HIDDEN_IN_INSPECTOR);
			Scene::Get()->AddGameObject(camera);
			this->editorCamera = camera->GetComponent<Camera>();
			});

		return true;
	}
	virtual void Term() override {
		SceneManager::onSceneLoaded.Unsubscribe(onSceneLoadedHandle);
	}

	void DrawInspector(char* object, std::string name, ReflectedTypeBase* type) {
		if (type->GetName() == ::GetReflectedType<float>()->GetName()) {
			float* f = (float*)(object);
			ImGui::DragFloat(name.c_str(), f, 0.1f, 0.f, 0.f, "%.5f", ImGuiSliderFlags_NoRoundToFormat);
		}
		else if (type->GetName() == ::GetReflectedType<int>()->GetName()) {
			int* i = (int*)(object);
			ImGui::DragInt(name.c_str(), i);
		}
		else if (type->GetName() == ::GetReflectedType<bool>()->GetName()) {
			bool* b = (bool*)(object);
			ImGui::Checkbox(name.c_str(), b);
		}
		else if (type->GetName() == ::GetReflectedType<std::string>()->GetName()) {
			std::string* str = (std::string*)(object);
			char buff[256];
			strncpy(buff, str->c_str(), Mathf::Min(str->size() + 1, 256));//TODO more than 256
			if (ImGui::InputText(name.c_str(), buff, 256)) {
				*str = std::string(buff);
			}
		}
		else if (type->GetName() == ::GetReflectedType<Color>()->GetName()) {
			Color* c = (Color*)(object);
			ImGui::ColorEdit4(name.c_str(), &c->r, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf);
		}
		else if (type->GetName() == ::GetReflectedType<Vector3>()->GetName()) {
			Vector3* v = (Vector3*)(object);
			ImGui::DragFloat3(name.c_str(), &v->x);
		}
		else if (type->GetName() == ::GetReflectedType<Transform>()->GetName()) {
			Transform* transform = (Transform*)(object);
			auto pos = transform->GetPosition();
			auto euler = Mathf::RadToDeg(transform->GetEulerAngles());
			auto scale = transform->GetScale();
			if (ImGui::TreeNode(name.c_str())) {
				//TODO store exact values somewhere
				if (ImGui::DragFloat3("position", &pos.x, 0.1f)) {
					transform->SetPosition(pos);
				}
				if (ImGui::DragFloat3("euler", &euler.x)) {
					transform->SetEulerAngles(Mathf::DegToRad(euler));
				}
				if (ImGui::DragFloat3("scale", &scale.x, 0.1f)) {
					transform->SetScale(scale);
				}
				ImGui::TreePop();
			}
		}
		else if (dynamic_cast<ReflectedTypeSharedPtrBase*>(type)) {
			std::shared_ptr<Object>* v = (std::shared_ptr<Object>*)(object);
			if (v->get()) {
				ImGui::LabelText(name.c_str(), "non null ref");
			}
			else {
				ImGui::LabelText(name.c_str(), "nullptr");
			}
		}
		else if (type->GetName() == ::GetReflectedType<GameObject>()->GetName()) {
			GameObject* go = (GameObject*)(object);
			char buff[256];
			strncpy(buff, go->tag.c_str(), Mathf::Min(go->tag.size() + 1, 256));
			if (ImGui::InputText("tag", buff, 256)) {
				go->tag = std::string(buff);
			}
			for (auto c : go->components) {
				if (c == nullptr) {
					ASSERT(false);
				}
				else {
					ImGui::PushID(c.get());
					bool isEnabled = c->IsEnabled();
					if (ImGui::Checkbox("", &isEnabled)) {
						c->SetEnabled(isEnabled);
					}
					ImGui::SameLine();
					DrawInspector((char*)c.get(), c->GetType()->GetName(), c->GetType());
					ImGui::PopID();
				}
			}
		}
		else if (dynamic_cast<ReflectedTypeStdVectorBase*>(type)) {
			auto vecType = dynamic_cast<ReflectedTypeStdVectorBase*>(type);
			auto elementType = vecType->GetElementType();
			int elementSize = vecType->GetElementSizeOf();

			std::vector<char>& v = *(std::vector<char>*)(object); //TODO D - DANGARAS
			int size = v.size() / elementSize;//TODO add sizeOf to ReflectedTypeBase
			if (ImGui::TreeNode(name.c_str())) {
				if (ImGui::InputInt("size", &size)) {
					size = Mathf::Max(size, 0);
					//TODO default constructor to ReflectedTypeBase
					v.resize(size * elementSize);//D - DANGARAS (no constructors/destructors and stuff are called)
				}
				for (int i = 0; i < size; i++) {
					DrawInspector(&v[i * elementSize], FormatString("%d", i), elementType);
				}
				ImGui::TreePop();
			}
		}
		else {
			auto typeNonBase = dynamic_cast<ReflectedTypeNonTemplated*>(type);
			if (typeNonBase) {
				if (ImGui::TreeNode(name.c_str())) {
					for (const auto& field : typeNonBase->fields) {
						char* var = object + field.offset;
						DrawInspector(var, field.name, field.type);
					}
					ImGui::TreePop();
				}
			}
			else {
				//TODO error
				ImGui::LabelText(name.c_str(), "???");
				return;
			}
		}
	}

	template<typename T>
	void DrawInspector(T& object) {
		auto typeBase = object.GetType();
		DrawInspector((((char*)&object)), "", typeBase);
	}

	void DrawOutliner() {
		auto scene = Scene::Get();
		static std::string filter;
		//TODO some wrapper for imgui string input
		char buff[256];
		strncpy(buff, filter.c_str(), Mathf::Min(filter.size() + 1, 256));
		if (ImGui::InputText("filter", buff, 256)) {
			filter = std::string(buff);
		}

		auto& gameObjects = scene->GetAllGameObjects();
		ImGui::BeginChild("left pane", ImVec2(150, 0), true);
		for (int i = 0; i < gameObjects.size(); i++)
		{
			auto go = gameObjects[i];
			std::string name = go->tag.size() > 0 ? go->tag.c_str() : "-";
			if (filter.size() > 0) {
				if (name.find(filter) == -1) {
					continue;//TODO ignoreCase
				}
			}
			if (Bits::IsMaskTrue(go->flags, GameObject::FLAGS::IS_HIDDEN_IN_INSPECTOR)) {
				continue;
			}

			ImGui::PushID(i);
			if (ImGui::Selectable(name.c_str(), selectedObject == gameObjects[i])) {
				selectedObject = gameObjects[i];
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
	}

	void DrawGizmosSelected(std::shared_ptr<GameObject> go) {
		for (auto c : go->components) {
			auto pl = std::dynamic_pointer_cast<PointLight>(c);
			if (pl) {
				auto sphereInner = Sphere{ pl->gameObject()->transform()->GetPosition(), pl->innerRadius };
				auto sphereOuter = Sphere{ pl->gameObject()->transform()->GetPosition(), pl->radius };
				Dbg::Draw(sphereInner).SetColor(Color(1, 1, 1, 1));
				Dbg::Draw(sphereOuter).SetColor(Color(1, 1, 1, 0.1f));
			}
		}
	}

	void Update() {
		if (!Engine::Get()->IsEditorMode()) {
			return;//TODO draw but only in pause
		}
		auto scene = Scene::Get();
		if (!scene) {
			return;
		}
		auto screenSize = Graphics::Get()->GetScreenSize();
		ImGui::SetNextWindowPos(ImVec2(screenSize.x, 0), ImGuiCond_Once, ImVec2(1.0, 0.0));
		ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_Once);

		ImGui::Begin("Inspector");

		DrawOutliner();
		ImGui::SameLine();

		ImGui::BeginChild("right pane", ImVec2(0, 0), true);
		if (selectedObject != nullptr) {
			DrawInspector(*selectedObject);
		}
		ImGui::EndChild();
		ImGui::End();

		//TODO refactor
		if (editorCamera != nullptr && Input::GetKeyDown(SDL_SCANCODE_F) && selectedObject != nullptr) {
			auto selectedGameObject = std::dynamic_pointer_cast<GameObject>(selectedObject);
			Matrix4 mat = Matrix4::Identity();
			auto rot = editorCamera->gameObject()->transform()->GetRotation();
			auto aabb = GetSphere(selectedGameObject);
			SetPos(mat, aabb.pos);
			auto offset = Matrix4::Transform(rot * Vector3_forward * aabb.radius * (-2.f), rot.ToMatrix(), Vector3_one);
			mat = mat * offset;
			editorCamera->gameObject()->transform()->SetMatrix(mat);
		}

		auto mousePos = Input::GetMousePosition();
		if (mousePos != prevMousePos) {
			prevMousePos = mousePos;
			nextSelectedObjectIndex = 0;
		}

		static bool gizmoDisabled = true;
		static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
		static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::MODE::WORLD);

		if (Input::GetKeyDown(SDL_SCANCODE_Q)) {
			gizmoDisabled = true;
		}
		if (Input::GetKeyDown(SDL_SCANCODE_W)) {
			gizmoDisabled = false;
			mCurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		}
		if (Input::GetKeyDown(SDL_SCANCODE_R)) {
			gizmoDisabled = false;
			mCurrentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
		}
		if (Input::GetKeyDown(SDL_SCANCODE_E)) {
			gizmoDisabled = false;
			mCurrentGizmoOperation = ImGuizmo::OPERATION::SCALE;
		}
		if (Input::GetKeyDown(SDL_SCANCODE_Z)) {
			if (mCurrentGizmoMode == ImGuizmo::MODE::WORLD) {
				mCurrentGizmoMode = ImGuizmo::MODE::LOCAL;
			}
			else {
				mCurrentGizmoMode = ImGuizmo::MODE::WORLD;
			}
		}

		if (selectedObject) {
			auto go = std::dynamic_pointer_cast<GameObject>(selectedObject);
			auto box = GetOBB(go);
			Dbg::Draw(box);
			DrawGizmosSelected(go);

			if (!gizmoDisabled) {
				ImGuiIO& io = ImGui::GetIO();
				ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
				const auto& view = Camera::GetMain()->GetViewMatrix();
				const auto& proj = Camera::GetMain()->GetProjectionMatrix();
				auto model = go->transform()->GetMatrix();
				auto deltaMatrix = Matrix4::Identity();
				ImGuizmo::Manipulate(&view(0, 0), &proj(0, 0), mCurrentGizmoOperation, mCurrentGizmoMode, &model(0, 0), &deltaMatrix(0, 0));
				if (deltaMatrix != Matrix4::Identity()) {
					if (mCurrentGizmoMode == ImGuizmo::MODE::WORLD && mCurrentGizmoOperation == ImGuizmo::OPERATION::SCALE) {
						SetRot(model, go->transform()->GetRotation());//BUGFIX
					}
					go->transform()->SetMatrix(model);
					//TODO undo
				}
			}
		}

		if (Input::GetMouseButtonDown(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ImGuizmo::IsUsing()) {
			auto objectsUnderCursor = GetObjectsUnderCursor();
			if (objectsUnderCursor.size() > 0) {
				auto idx = nextSelectedObjectIndex % objectsUnderCursor.size();
				selectedObject = objectsUnderCursor[idx];
			}
			nextSelectedObjectIndex++;
		}
	}
};

REGISTER_SYSTEM(InspectorWindow);