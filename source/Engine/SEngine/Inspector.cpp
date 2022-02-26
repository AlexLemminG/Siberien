

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
#include "Editor.h"
#include "dear-imgui/widgets/gizmo.h"
#include "DbgVars.h"

DBG_VAR_BOOL(dbg_showInspector, "Inspector", false);


//TODO to utils
Sphere GetSphere(std::shared_ptr<GameObject> go) {
	bool hasSphere = false;
	Sphere sphere;
	auto renderer = go->GetComponent<MeshRenderer>();
	if (renderer && renderer->mesh) {
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
	if (renderer && renderer->mesh) {
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
	//TODO not only spot light
	auto spotLight = go->GetComponent<SpotLight>();
	if (spotLight) {
		auto center = spotLight->gameObject()->transform()->GetPosition();
		auto radius = spotLight->radius;
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

	bool IsChanged(std::string fullPath) {
		return false;
	}

	struct VarInfo {
		ryml::NodeRef yaml = ryml::NodeRef();
		ryml::NodeRef root = ryml::NodeRef();
		std::vector<std::string> path;
		std::shared_ptr<Object> rootObj;

	private:
		VarInfo() {
		}
	public:

		VarInfo (std::shared_ptr<Object> obj) {
			auto uid = AssetDatabase::Get()->GetAssetUID(obj);
			if (!uid.empty()) {
				//it's an asset
				yaml = AssetDatabase::Get()->GetOriginalSerializedAsset(obj);
				root = yaml;
				rootObj = obj;
			}
			else {
				yaml = nullptr;
				root = nullptr;
				rootObj = obj;
				auto currentScene = Scene::Get();
				if (currentScene) {
					auto go = std::dynamic_pointer_cast<GameObject>(obj);
					auto component = std::dynamic_pointer_cast<Component>(obj);
					if(component){
						go = component->gameObject();
					}
					int idx = currentScene->GetInstantiatedPrefabIdx(go);
					if (idx != -1) {
						VarInfo real{ currentScene }; //TODO pray for it to not to be instantiated scene
						real = real.Child("prefabInstances");
						real = real.Child(idx);
						real = real.Child("overrides");
						rootObj = currentScene;
						if (component) {
							real = real.Child(component->GetType()->GetName());//TODO not always right
						}
						else {
							real = real.Child("GameObject");
						}
						path = real.path;
						yaml = real.yaml;
						root = real.root;
					}
				}
			}
		}

		VarInfo Child(std::string name) const {
			VarInfo child;
			child.path = path;
			child.path.push_back(name);
			child.root = root;
			child.rootObj = rootObj;
			if (yaml.valid() && yaml.is_map()) {
				child.yaml = yaml.find_child(c4::csubstr(name.c_str(), name.length()));
			}
			return child;
		}
		VarInfo Child(int i) const {
			auto name = (std::to_string(i));
			VarInfo child;
			child.path = path;
			child.path.push_back(name);
			child.root = root;
			child.rootObj = rootObj;
			if (yaml.valid() && yaml.is_seq()) {
				child.yaml = yaml.child(i);
			}
			return child;
		}
		bool Exist() const {
			return yaml.valid();
		}
		template<typename T>
		void SetValue(const T* t) {
			SetValue(GetReflectedType(t), (void*)t);
		}
		void SetValue(ReflectedTypeBase* type, void* val) {
			if (root == nullptr) {
				return;
			}
			SerializationContext rootContext{ root };
			SerializationContext context = rootContext;
			for (auto& name : path) {
				if (name.size() > 0 && name[0] >= '0' && name[1] <= '9') {
					int idx = std::stoi(name);
					for (int i = 0; i < idx; i++) {
						context.Child(i);//creating empty children
					}
					context = context.Child(idx);
				}
				else {
					context = context.Child(name);
				}
			}
			Editor::SetDirty(rootObj);
			type->Serialize(context, val);
			std::ofstream fout("out.yaml");
			fout << root.tree()->rootref();
		}
	};

	std::vector<bool> needToPopEditedVarStyle;
	void BeginInspector(VarInfo& varInfo) {
		if (varInfo.Exist()) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
			needToPopEditedVarStyle.push_back(true);
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 128));
			needToPopEditedVarStyle.push_back(true);
		}
	}
	void EndInspector(VarInfo& varInfo) {
		if (needToPopEditedVarStyle.back()) {
			ImGui::PopStyleColor();
		}
		needToPopEditedVarStyle.pop_back();
	}

	void DrawInspector(char* object, std::string name, ReflectedTypeBase* type, VarInfo& varInfo, bool expanded = false) {
		if (expanded) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		}
		if (type->GetName() == ::GetReflectedType<float>()->GetName()) {
			float* f = (float*)(object);
			BeginInspector(varInfo);
			float speed = Mathf::Max(Mathf::Abs(*f) / 100.f, 0.01f);
			if (ImGui::DragFloat(name.c_str(), f, speed, 0.f, 0.f, "%.5f", ImGuiSliderFlags_NoRoundToFormat)) {
				varInfo.SetValue(f);
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<int>()->GetName()) {
			int* i = (int*)(object);
			BeginInspector(varInfo);
			float speed = Mathf::Max(float(Mathf::Abs(*i)) / 100.f, 1.f);
			if (ImGui::DragInt(name.c_str(), i, speed)) {
				varInfo.SetValue(i);
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<bool>()->GetName()) {
			bool* b = (bool*)(object);
			BeginInspector(varInfo);
			if (ImGui::Checkbox(name.c_str(), b)) {
				varInfo.SetValue(b);
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<std::string>()->GetName()) {
			std::string* str = (std::string*)(object);
			char buff[256];
			strncpy(buff, str->c_str(), Mathf::Min(str->size() + 1, 256));//TODO more than 256
			BeginInspector(varInfo);
			if (ImGui::InputText(name.c_str(), buff, 256)) {
				*str = std::string(buff);
				varInfo.SetValue(str);
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Color>()->GetName()) {
			Color* c = (Color*)(object);
			BeginInspector(varInfo);
			if (ImGui::ColorEdit4(name.c_str(), &c->r, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf)) {
				varInfo.SetValue(c);
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Vector3>()->GetName()) {
			Vector3* v = (Vector3*)(object);
			BeginInspector(varInfo);
			float speed = Mathf::Max(Mathf::Max(Mathf::Abs(v->x), Mathf::Max(v->y, v->z)) / 100.f, 0.01f);
			if (ImGui::DragFloat3(name.c_str(), &v->x, speed)) {
				varInfo.SetValue(v);
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Transform>()->GetName()) {
			Transform* transform = (Transform*)(object);
			auto pos = transform->GetPosition();
			auto euler = Mathf::RadToDeg(transform->GetEulerAngles());
			auto scale = transform->GetScale();
			if (ImGui::TreeNode(name.c_str())) {
				//TODO store exact values somewhere
				DrawInspector((char*)&pos.x, "position", GetReflectedType<Vector3>(), varInfo.Child("pos"));
				DrawInspector((char*)&euler.x, "euler", GetReflectedType<Vector3>(), varInfo.Child("euler"));//TODO fix shown to user euler changed after applied to matrix and back
				DrawInspector((char*)&scale.x, "scale", GetReflectedType<Vector3>(), varInfo.Child("scale"));

				//TODO if changed
				transform->SetPosition(pos);
				transform->SetEulerAngles(Mathf::DegToRad(euler));
				transform->SetScale(scale);

				ImGui::TreePop();
			}
		}
		else if (dynamic_cast<ReflectedTypeSharedPtrBase*>(type)) {
			std::shared_ptr<Object>* v = (std::shared_ptr<Object>*)(object);
			if (v->get()) {
				//TODO selection
				if (ImGui::Button(name.c_str())) {
					selectedObject = *v;
				}
			}
			else {
				ImGui::LabelText(name.c_str(), "nullptr");
			}
		}
		else if (type->GetName() == ::GetReflectedType<GameObject>()->GetName()) {
			GameObject* go = (GameObject*)(object);
			DrawInspector((char*)&go->tag, "tag", GetReflectedType<std::string>(), varInfo.Child("tag"));
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

					VarInfo childInfo{ c };
					DrawInspector((char*)c.get(), c->GetType()->GetName(), c->GetType(), childInfo, true);
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
				BeginInspector(varInfo);
				if (ImGui::InputInt("size", &size)) {
					size = Mathf::Max(size, 0);
					//TODO default constructor to ReflectedTypeBase
					v.resize(size * elementSize);//D - DANGARAS (no constructors/destructors and stuff are called)
					varInfo.SetValue(type, object);
				}
				EndInspector(varInfo);
				for (int i = 0; i < size; i++) {
					auto childInfo = varInfo.Child(i);
					DrawInspector(&v[i * elementSize], FormatString("%d", i), elementType, childInfo);//TODO apply changes to whole yaml arrray if one element is changed
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
						auto childInfo = varInfo.Child(field.name);
						DrawInspector(var, field.name, field.type, childInfo);
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
	void DrawInspector(T& object, VarInfo& varInfo) {
		auto typeBase = object.GetType();
		DrawInspector((((char*)&object)), "", typeBase, varInfo, true);
	}

	void DrawOutliner() {
		auto scene = Scene::Get();
		static std::string filter;

		//TODO move to some other place
		if (ImGui::Button("ShadowSettings")) {
			selectedObject = AssetDatabase::Get()->Load("settings.asset$ShadowSettings");
		}

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
		bool hasGizmos = false;
		Color innerLightColor = Color(1, 1, 1, 1);
		Color outerLightColor = Color(1, 1, 1, 0.1f);
		for (auto c : go->components) {
			auto pl = std::dynamic_pointer_cast<PointLight>(c);
			if (pl) {
				auto sphereInner = Sphere{ go->transform()->GetPosition(), pl->innerRadius };
				auto sphereOuter = Sphere{ go->transform()->GetPosition(), pl->radius };
				Dbg::Draw(sphereInner).SetColor(innerLightColor);
				Dbg::Draw(sphereOuter).SetColor(outerLightColor);
				hasGizmos = true;
			}

			auto sl = std::dynamic_pointer_cast<SpotLight>(c);
			if (sl) {
				float radius = sl->radius;
				float innerRadius = sl->innerRadius;
				float angle = Mathf::DegToRad(sl->outerAngle / 2.f);
				float innerAngle = Mathf::DegToRad(sl->innerAngle / 2.f);
				Vector3 pos = go->transform()->GetPosition();
				Vector3 dir = go->transform()->GetForward();

				float dirMultInner = Mathf::Cos(innerAngle);
				float dirMultOuter = Mathf::Cos(angle);
				float radiusMultInner = Mathf::Sin(innerAngle);
				float radiusMultOuter = Mathf::Sin(angle);
				Dbg::DrawCone(pos, pos + dir * radius * dirMultOuter, radius * radiusMultOuter).SetColor(outerLightColor);
				Dbg::DrawCone(pos, pos + dir * radius * dirMultInner, radius * radiusMultInner).SetColor(innerLightColor);
				Dbg::DrawCone(pos, pos + dir * innerRadius * dirMultOuter, innerRadius * radiusMultOuter).SetColor(innerLightColor);
				hasGizmos = true;
			}
		}
		if (!hasGizmos) {
			auto box = GetOBB(go);
			Dbg::Draw(box);
		}
	}

	void Update() {
		if (!dbg_showInspector && !Editor::Get()->IsInEditMode()) {
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
		if (selectedObject == nullptr) {
			selectedObject = Scene::Get();
		}
		if (selectedObject != nullptr) {
			VarInfo varInfo{ selectedObject };
			DrawInspector(*selectedObject, varInfo);
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

			if (go != nullptr) {
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


						VarInfo varInfo{ go->transform() };
						if (varInfo.yaml.valid()) {
							varInfo.SetValue(go->transform().get());
						}
						//TODO undo
					}
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