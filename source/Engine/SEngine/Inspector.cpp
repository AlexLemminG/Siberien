

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
#include "BoxCollider.h"

DBG_VAR_BOOL(dbg_showInspector, "Inspector", false);

//TODO add to some header
extern void DuplicateSerialized(const ryml::NodeRef& from, ryml::NodeRef& to);

//TODO to utils
Sphere GetSphere(std::shared_ptr<GameObject> go) {
	bool hasSphere = false;
	Sphere sphere;
	auto renderer = go->GetComponent<MeshRenderer>();
	if (renderer && renderer->mesh) {
		auto meshSphere = renderer->mesh->boundingSphere;
		auto scale = go->transform()->GetScale();
		float maxScale = Mathf::Max(Mathf::Max(scale.x, scale.y), scale.z);
		meshSphere.radius *= maxScale;
		meshSphere.pos = go->transform()->GetMatrix() * meshSphere.pos;

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
static bool IsInScene(std::shared_ptr<GameObject> go) {
	return Scene::Get() != nullptr && std::find(Scene::Get()->GetAllGameObjects().begin(), Scene::Get()->GetAllGameObjects().end(), go) != Scene::Get()->GetAllGameObjects().end();
}
bool RaycastExact(std::shared_ptr<GameObject> go, Ray ray, float& distance) {
	float maxDistance = 100000.f;
	if (!GeomUtils::IsOverlapping(GetSphere(go), ray)) {
		return false;
	}
	auto meshRenderer = go->GetComponent<MeshRenderer>();
	if (meshRenderer) {
		auto mesh = meshRenderer->mesh;
		if (mesh) {
			auto collider = MeshColliderStorageSystem::Get()->GetStored(mesh);
			if (collider) {
				auto scaledShape = std::make_unique<btScaledBvhTriangleMeshShape>(collider.get(), btConvert(go->transform()->GetScale()));
				auto info = btRigidBody::btRigidBodyConstructionInfo(0, nullptr, scaledShape.get());
				auto matr = go->transform()->GetMatrix();
				SetScale(matr, Vector3_one);//TODO optimize
				info.m_startWorldTransform = btConvert(matr);
				auto* rb = new btRigidBody(info);

				const std::string& layer = PhysicsSystem::reservedLayerName;
				int group;
				int mask;
				PhysicsSystem::Get()->GetGroupAndMask(layer, group, mask);
				PhysicsSystem::Get()->dynamicsWorld->addRigidBody(rb, group, mask);

				Physics::RaycastHit hit;
				bool overlapping = false;

				if (Physics::Raycast(hit, ray, maxDistance, mask)) {
					overlapping = true;
				}

				PhysicsSystem::Get()->dynamicsWorld->removeRigidBody(rb);

				//PhysicsSystem::Get()->dynamicsWorld->debugDrawObject(btConvert(matr), scaledShape.get(), btVector3(1, 1, 1));

				delete rb;
				if (overlapping) {
					distance = Vector3::Distance(hit.GetPoint(), ray.origin);
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
		if (GeomUtils::IsOverlapping(Sphere(center, radius), ray)) {
			distance = Vector3::DotProduct(center - ray.origin, ray.dir);
			if (distance > 0) {
				return true;
			}
		}
	}
	//TODO not only spot light
	auto spotLight = go->GetComponent<SpotLight>();
	if (spotLight) {
		auto center = spotLight->gameObject()->transform()->GetPosition();
		auto radius = spotLight->radius;
		if (GeomUtils::IsOverlapping(Sphere(center, radius), ray)) {
			distance = Vector3::DotProduct(center - ray.origin, ray.dir);
			if (distance > 0) {
				return true;
			}
		}
	}
	return false;
}

std::vector<std::shared_ptr<GameObject>> GetObjectsUnderCursor() {
	float maxDistance = 1000.f;
	auto mouseRay = Camera::GetMain()->ScreenPointToRay(Input::GetMousePosition());
	std::vector<std::pair<std::shared_ptr<GameObject>, float>> resultWithDistance;
	for (auto obj : Scene::Get()->GetAllGameObjects()) {
		if (Bits::IsMaskTrue(obj->flags, GameObject::FLAGS::IS_HIDDEN_IN_INSPECTOR)) {
			continue;
		}
		float distance;
		if (RaycastExact(obj, mouseRay, distance)) {
			resultWithDistance.push_back(std::pair<std::shared_ptr<GameObject>, float>(obj, distance));
		}
	}
	Vector3 cameraPos = mouseRay.origin;
	std::sort(resultWithDistance.begin(), resultWithDistance.end(), [cameraPos](std::pair<std::shared_ptr<GameObject>, float> x, std::pair<std::shared_ptr<GameObject>, float> y) {
		if (x.first->GetComponent<MeshRenderer>() != nullptr && y.first->GetComponent<MeshRenderer>() == nullptr) {
			return true;
		}
		else if (x.first->GetComponent<MeshRenderer>() == nullptr && y.first->GetComponent<MeshRenderer>() != nullptr) {
			return false;
		}
		return x.second < y.second;
		});


	std::vector<std::shared_ptr<GameObject>> result;
	for (auto obj : resultWithDistance) {
		result.push_back(obj.first);
	}
	return std::move(result);
}


static std::shared_ptr<GameObject> GetGameObject(std::shared_ptr<Object> object) {
	if (std::dynamic_pointer_cast<Component>(object)) {
		return std::dynamic_pointer_cast<Component>(object)->gameObject();
	}
	else {
		return std::dynamic_pointer_cast<GameObject>(object);
	}
}
static void CallOnValidate(std::shared_ptr<Object> object) {
	auto go = GetGameObject(object);
	if (go && IsInScene(go)) {
		for (auto c : go->components) {
			c->OnValidate();
		}
	}

	// TODO rename method ?
	Editor::Get()->onGameObjectEdited.Invoke(go);
}
static void CallOnDrawGizmos(std::shared_ptr<Object> object) {
	auto go = GetGameObject(object);
	if (go && IsInScene(go)) {
		for (auto c : go->components) {
			c->OnDrawGizmos();
		}
	}
}


class InspectorWindow : public System<InspectorWindow> {
	GameEventHandle onSceneLoadedHandle;

	//std::shared_ptr<Object> selectedObject;

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

	//TODO try to merge code with SerializationContext
	struct VarInfo {
		ryml::NodeRef yaml = ryml::NodeRef();
		ryml::NodeRef root = ryml::NodeRef();
		std::vector<std::string> path;
		std::shared_ptr<Object> rootObjToSave; // root object of asset to save after editing
		std::shared_ptr<Object> rootObjToCallValidate; // last Object up in hierarchy

	private:
		VarInfo() {
		}
	public:

		VarInfo(std::shared_ptr<Object> obj) {
			auto uid = AssetDatabase::Get()->GetAssetUID(obj);
			if (!uid.empty()) {
				//it's an asset
				yaml = AssetDatabase::Get()->GetOriginalSerializedAsset(obj);
				root = yaml;
				rootObjToCallValidate = obj;
				rootObjToSave = obj;
			}
			else {
				yaml = nullptr;
				root = nullptr;
				rootObjToCallValidate = obj;
				auto currentScene = Scene::Get();
				if (currentScene) {
					auto go = std::dynamic_pointer_cast<GameObject>(obj);
					auto component = std::dynamic_pointer_cast<Component>(obj);
					if (component) {
						go = component->gameObject();
					}
					int idx = currentScene->GetInstantiatedPrefabIdx(go.get());
					if (idx != -1) {
						VarInfo real{ currentScene }; //TODO pray for it to not to be instantiated scene
						real = real.Child("prefabInstances");
						real = real.Child(idx);
						real = real.Child("overrides");
						rootObjToSave = currentScene;
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
			child.rootObjToSave = rootObjToSave;
			child.rootObjToCallValidate = rootObjToCallValidate;
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
			child.rootObjToSave = rootObjToSave;
			child.rootObjToCallValidate = rootObjToCallValidate;
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
		void SetValue(ReflectedTypeBase* type, const void* val) {
			if (root == nullptr) {
				return;
			}
			SerializationContext rootContext{ root };
			SerializationContext context = rootContext;
			//TODO for prefabs we need to also change or reload overrides
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
			Editor::SetDirty(rootObjToSave);
			type->Serialize(context, val);

			CallOnValidate(rootObjToCallValidate);
		}
		void ResetValue(ReflectedTypeBase* type, void* val) {
			//find and clear value from yaml
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
			if (path.size() == 0) {
				context.ClearValue();
			}
			else {
				context.Clear();
			}
			//auto childPos = context.GetYamlNode().parent().child_pos(context.GetYamlNode());
			//context.GetYamlNode().parent().remove_child(childPos);

			//find default value to put in 'val'
			SerializationContext defaultContext;
			auto prefab = Scene::Get()->GetSourcePrefab(GetGameObject(Editor::Get()->selectedObject).get());
			int pathFirstIdx = 0;
			std::shared_ptr<Object> clearedObj;
			if (prefab != nullptr) {
				auto rootComponent = std::dynamic_pointer_cast<Component>(rootObjToCallValidate);
				auto rootGameObject = std::dynamic_pointer_cast<GameObject>(rootObjToCallValidate);
				if (rootComponent) {
					int idx = -1;
					for (int i = 0; i < rootComponent->gameObject()->components.size(); i++) {
						if (rootComponent->gameObject()->components[i] == rootComponent) {
							idx = i;
							break;
						}
					}
					if (idx != -1) {
						//TODO support overrides in prefab components list
						clearedObj = prefab->components[idx];
					}
					else {
						ASSERT(false);
					}
				}
				else if (rootGameObject) {
					clearedObj = rootGameObject;
				}
				else {
					ASSERT(false);
				}
			}
			else {
				//TODO double check
				std::string emptyContextYamlStr = rootObjToCallValidate->GetType()->GetName() + ": ~";
				auto tree = ryml::parse(ryml::csubstr(emptyContextYamlStr.c_str(), emptyContextYamlStr.size()));
				clearedObj = AssetDatabase::Get()->DeserializeFromYAML<Object>(tree);
			}

			if (!clearedObj) {
				ASSERT(false);//TODO ue4 like ENSURE key
			}
			else {
				//HACK dangaras
				char* clearedVal = ((char*)clearedObj.get() + ((char*)val - (char*)rootObjToCallValidate.get()));

				SerializationContext valContext{};
				type->Serialize(valContext, clearedVal);
				type->Deserialize(valContext, val);
			}

			Editor::SetDirty(rootObjToSave);

			CallOnValidate(rootObjToCallValidate);
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
	//returns true if value changed
	bool DrawInspector(char* object, std::string name, ReflectedTypeBase* type, VarInfo& varInfo, bool expanded = false) {
		bool changed = false;
		bool canReset = varInfo.rootObjToCallValidate->GetType()->GetName() != ::GetReflectedType<Transform>()->GetName(); //transform is kinda hacky
		canReset &= varInfo.Exist();
		canReset &= !name.empty();
		if (expanded) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		}
		if (type->GetName() == ::GetReflectedType<float>()->GetName()) {
			float* f = (float*)(object);
			BeginInspector(varInfo);
			float speed = Mathf::Max(Mathf::Abs(*f) / 100.f, 0.01f);
			if (ImGui::DragFloat(name.c_str(), f, speed, 0.f, 0.f, "%.5f", ImGuiSliderFlags_NoRoundToFormat)) {
				varInfo.SetValue(f);
				changed = true;
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<int>()->GetName()) {
			int* i = (int*)(object);
			BeginInspector(varInfo);
			float speed = Mathf::Max(float(Mathf::Abs(*i)) / 100.f, 1.f);
			if (ImGui::DragInt(name.c_str(), i, speed)) {
				varInfo.SetValue(i);
				changed = true;
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<bool>()->GetName()) {
			bool* b = (bool*)(object);
			BeginInspector(varInfo);
			if (ImGui::Checkbox(name.c_str(), b)) {
				varInfo.SetValue(b);
				changed = true;
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
				changed = true;
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Color>()->GetName()) {
			Color* c = (Color*)(object);
			BeginInspector(varInfo);
			if (ImGui::ColorEdit4(name.c_str(), &c->r, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf)) {
				varInfo.SetValue(c);
				changed = true;
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Vector3>()->GetName()) {
			Vector3* v = (Vector3*)(object);
			BeginInspector(varInfo);
			Vector3 absV = Vector3(Mathf::Abs(v->x), Mathf::Abs(v->y), Mathf::Abs(v->z));
			float speed = Mathf::Max(Mathf::Max(absV.x, Mathf::Max(absV.y, absV.z)) / 100.f, 0.01f);
			if (ImGui::DragFloat3(name.c_str(), &v->x, speed)) {
				varInfo.SetValue(v);
				changed = true;
			}

			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Vector4>()->GetName()) {
			Vector4* v = (Vector4*)(object);
			BeginInspector(varInfo);
			Vector4 absV = Vector4(Mathf::Abs(v->x), Mathf::Abs(v->y), Mathf::Abs(v->z), Mathf::Abs(v->w));
			float speed = Mathf::Max(Mathf::Max(Mathf::Max(absV.x, absV.y), Mathf::Max(absV.z, absV.w)) / 100.f, 0.01f);
			if (ImGui::DragFloat4(name.c_str(), &v->x, speed)) {
				varInfo.SetValue(v);
				changed = true;
			}
			EndInspector(varInfo);
		}
		else if (type->GetName() == ::GetReflectedType<Transform>()->GetName()) {
			canReset = false;
			Transform* transform = (Transform*)(object);
			auto pos = transform->GetPosition();
			auto euler = Mathf::RadToDeg(transform->GetEulerAngles());
			auto scale = transform->GetScale();
			if (ImGui::TreeNode(name.c_str())) {
				//TODO store exact values somewhere
				changed |= DrawInspector((char*)&pos.x, "position", GetReflectedType<Vector3>(), varInfo.Child("pos"));
				changed |= DrawInspector((char*)&euler.x, "euler", GetReflectedType<Vector3>(), varInfo.Child("euler"));//TODO fix shown to user euler changed after applied to matrix and back
				changed |= DrawInspector((char*)&scale.x, "scale", GetReflectedType<Vector3>(), varInfo.Child("scale"));

				if (changed) {
					//TODO bugfix zero scale can break euler angles
					transform->SetPosition(pos);
					transform->SetEulerAngles(Mathf::DegToRad(euler));
					transform->SetScale(scale);
					CallOnValidate(varInfo.rootObjToCallValidate);
					if (!AssetDatabase::Get()->GetAssetPath(transform->gameObject()->transform()).empty()) {
						Editor::SetDirty(transform->gameObject()->transform());
					}
				}

				ImGui::TreePop();
			}
		}
		else if (dynamic_cast<ReflectedTypeSharedPtrBase*>(type)) {
			std::shared_ptr<Object>* v = (std::shared_ptr<Object>*)(object);
			if (v->get()) {
				//TODO selection
				if (ImGui::Button(name.c_str())) {
					Editor::Get()->selectedObject = *v;
				}
			}
			else {
				ImGui::LabelText(name.c_str(), "nullptr");
			}
		}
		else if (type->GetName() == ::GetReflectedType<GameObject>()->GetName()) {
			GameObject* go = (GameObject*)(object);
			auto prefab = Scene::Get()->GetSourcePrefab(go);
			if (prefab) {
				if (ImGui::Button("Open Prefab")) {
					Editor::Get()->selectedObject = prefab;

					//HACK honk honk, welcome to clown world
					for (auto c : prefab->components) {
						if (c->m_gameObject.lock() == nullptr) {
							c->m_gameObject = prefab;
						}
					}
				}
			}
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
						CallOnValidate(c);
						changed = true;
						//TODO support saving
					}
					ImGui::SameLine();

					VarInfo childInfo{ c };
					changed |= DrawInspector((char*)c.get(), c->GetType()->GetName(), c->GetType(), childInfo, true);
					ImGui::PopID();
				}
			}
			if (!prefab) {
				if (ImGui::Button("Add Component")) {
					// goes to next AddComponent popup
				}
				{
					ReflectedTypeBase* componentType = nullptr;
					if (ImGui::BeginPopupContextItem("AddComponent", ImGuiPopupFlags_MouseButtonLeft)) {
						//TODO sort types by name
						for (auto type : GetSerialiationInfoStorage().GetAllTypes()) {
							auto subtype = type;
							while (subtype != nullptr) {
								if (subtype->GetName() == GetReflectedType<Component>()->GetName()) {
									if (ImGui::Selectable(type->GetName().c_str())) {
										componentType = type;
									}
									break;
								}
								subtype = subtype->GetParentType();
							}
						}
						ImGui::EndPopup();
					}
					if (componentType != nullptr) {
						//TODO choose type
						//TODO extract method
						int newIdx = go->components.size();
						auto newComponentStr = componentType->GetName() + ": ~";
						auto newComponentYaml = ryml::parse(c4::to_csubstr(newComponentStr.c_str()));
						SerializationContext componentContext{ newComponentYaml };
						auto newComponent = std::dynamic_pointer_cast<Component>(Object::Instantiate(componentContext));
						//TODO assert not null
						newComponent->m_gameObject = go->transform()->gameObject();//HACK wowoowowoowowo
						//TODO init newComponent
						//TODO separate AddComponent/RemoveComponent method
						auto allGameObject = Scene::Get()->GetAllGameObjects();
						bool isInScene = std::find(allGameObject.begin(), allGameObject.end(), go->transform()->gameObject()) != allGameObject.end();
						if (isInScene) {
							Scene::Get()->RemoveGameObjectImmediately(go->transform()->gameObject());
						}
						go->components.push_back(newComponent);
						std::string assetPath = AssetDatabase::Get()->GetAssetPath(go->transform()->gameObject());
						if (!assetPath.empty()) {
							std::string id = AssetDatabase::Get()->AddObjectToAsset(assetPath, newComponent);
							std::string idRef = "$" + id;
							varInfo.Child("components").Child(newIdx).SetValue(&idRef);
							auto nodeRef = AssetDatabase::Get()->GetOriginalSerializedAsset(newComponent->gameObject());
							SerializationContext context{ nodeRef.parent() };//HACK som much hacks in one place wow
							context.Child(newComponent->GetType()->GetName() + "$" + id);//HACK child creates node with no val right now, but it may change in the future
							VarInfo childInfo{ newComponent };
							childInfo.ResetValue(newComponent->GetType(), newComponent.get());

						}
						if (isInScene) {
							Scene::Get()->AddGameObject(go->transform()->gameObject());
						}
					}
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
					changed = true;
				}
				EndInspector(varInfo);
				for (int i = 0; i < size; i++) {
					auto childInfo = varInfo.Child(i);
					changed |= DrawInspector(&v[i * elementSize], FormatString("%d", i), elementType, childInfo);//TODO apply changes to whole yaml arrray if one element is changed
				}
				ImGui::TreePop();
			}
		}
		else {
			auto typeNonBase = dynamic_cast<ReflectedTypeNonTemplated*>(type);
			if (typeNonBase) {
				BeginInspector(varInfo);
				if (ImGui::TreeNode(name.c_str())) {
					//TODO remove duplication with code below
					if (ImGui::BeginPopupContextItem(name.c_str(), ImGuiPopupFlags_MouseButtonRight)) {
						std::string str = "reset " + name;
						if (canReset && ImGui::Selectable(str.c_str())) {
							varInfo.ResetValue(type, object);
						}
						auto go = GetGameObject(varInfo.rootObjToCallValidate);
						auto prefab = Scene::Get()->GetSourcePrefab(go.get());
						if (!prefab) {
							//it is not prefab, so we can remove this component
							//TODO check if it is a component!!!!
							//TODO dont remove transform
							//TODO RemoveComponent method
							std::string str = "remove " + name;
							if (ImGui::Selectable(str.c_str())) {
								int idx = -1;
								for (int i = 0; i < go->components.size(); i++) {
									if (varInfo.rootObjToCallValidate == go->components[i]) {
										idx = i;
										break;
									}
								}
								if (idx != -1) {
									auto component = go->components[idx];
									auto allGameObject = Scene::Get()->GetAllGameObjects();
									bool isInScene = std::find(allGameObject.begin(), allGameObject.end(), go) != allGameObject.end();
									if (isInScene) {
										Scene::Get()->RemoveGameObjectImmediately(go);
									}
									//TODO disable component first
									std::string oldId = AssetDatabase::Get()->RemoveObjectFromAsset(component);
									go->components.erase(go->components.begin() + idx);

									std::string assetPath = AssetDatabase::Get()->GetAssetPath(go->transform()->gameObject());
									if (!assetPath.empty()) {
										SerializationContext context{ AssetDatabase::Get()->GetOriginalSerializedAsset(go) };
										context.Child("components").Child(idx).Clear();
										SerializationContext contextRoot{ AssetDatabase::Get()->GetOriginalSerializedAsset(go).parent() };
										contextRoot.Child(component->GetType()->GetName() + "$" + oldId).Clear();
										if (oldId == component->GetType()->GetName()) {
											contextRoot.Child(component->GetType()->GetName()).Clear();
										}
									}

									if (isInScene) {
										Scene::Get()->AddGameObject(go);
									}
									Editor::SetDirty(go);
									CallOnValidate(go);
								}
								else {
									ASSERT(false);
								}
							}
						}
						ImGui::EndPopup();
					}

					for (const auto& field : typeNonBase->fields) {
						char* var = object + field.offset;
						auto childInfo = varInfo.Child(field.name);
						changed |= DrawInspector(var, field.name, field.type, childInfo);
					}
					ImGui::TreePop();
				}
				EndInspector(varInfo);
				canReset = false;
			}
			else {
				//TODO error
				ImGui::LabelText(name.c_str(), "???");
			}
		}
		if (canReset && ImGui::BeginPopupContextItem(name.c_str(), ImGuiPopupFlags_MouseButtonRight)) {
			std::string str = "reset " + name;
			if (ImGui::Selectable(str.c_str())) {
				varInfo.ResetValue(type, object);
			}
			ImGui::EndPopup();
		}
		return changed;
	}

	template<typename T>
	void DrawInspector(T& object, VarInfo& varInfo) {
		auto typeBase = object.GetType();
		DrawInspector((((char*)&object)), "", typeBase, varInfo, true);
	}
	bool isSettings = false;
	void DrawOutliner() {
		auto scene = Scene::Get();
		static std::string filter;

		//TODO move to some other place
		if (ImGui::Button(isSettings ? "GameObjects" : "Settings")) {
			isSettings = !isSettings;
		}

		//TODO some wrapper for imgui string input
		char buff[256];
		strncpy(buff, filter.c_str(), Mathf::Min(filter.size() + 1, 256));
		if (ImGui::InputText("filter", buff, 256)) {
			filter = std::string(buff);
		}

		ImGui::BeginChild("left pane", ImVec2(150, 0), true);
		if (isSettings) {
			auto allSettings = AssetDatabase::Get()->LoadAll("settings.asset");
			for (int i = 0; i < allSettings.size(); i++)
			{
				auto setting = allSettings[i];
				if (ImGui::Selectable(setting->GetDbgName().c_str(), Editor::Get()->selectedObject == setting)) {
					Editor::Get()->selectedObject = setting;
				}
			}
		}
		else
		{
			auto& gameObjects = scene->GetAllGameObjects();
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
				if (ImGui::Selectable(name.c_str(), Editor::Get()->selectedObject == gameObjects[i])) {
					Editor::Get()->selectedObject = gameObjects[i];
				}
				std::string deleteGameObjectStr = "context " + name;
				if (ImGui::BeginPopupContextItem(deleteGameObjectStr.c_str(), ImGuiPopupFlags_MouseButtonRight)) {
					std::string str = "delete " + name;
					if (ImGui::Selectable(str.c_str())) {
						//TODO separate delete method
						auto go = gameObjects[i];
						int prefabIdx = Scene::Get()->GetInstantiatedPrefabIdx(go.get());
						if (prefabIdx != -1) {
							VarInfo varInfo{ Scene::Get() }; //TODO pray for it to not to be instantiated scene
							varInfo = varInfo.Child("prefabInstances");
							varInfo = varInfo.Child(prefabIdx);

							SerializationContext c{ varInfo.yaml };
							c.Clear();
							auto& instances = Scene::Get()->prefabInstances;
							Scene::Get()->prefabInstances.erase(Scene::Get()->prefabInstances.begin() + prefabIdx);
							Scene::Get()->instantiatedPrefabs.erase(Scene::Get()->instantiatedPrefabs.begin() + prefabIdx);
						}
						else {
							VarInfo varInfo{ Scene::Get() }; //TODO pray for it to not to be instantiated scene
							varInfo = varInfo.Child("gameObjects");
							varInfo = varInfo.Child(i);
							SerializationContext c{ varInfo.yaml };
							c.Clear();

							std::vector<std::shared_ptr<Object>> objects;
							objects.push_back(go);
							for (auto c : go->components) {
								objects.push_back(c);
							}
							for (auto o : objects) {
								if (AssetDatabase::Get()->GetAssetPath(o) == AssetDatabase::Get()->GetAssetPath(Scene::Get())) {
									VarInfo varInfo{ Scene::Get() }; //TODO pray for it to not to be instantiated scene
									SerializationContext c{ varInfo.root.tree()->rootref() };
									auto id = AssetDatabase::Get()->RemoveObjectFromAsset(o);
									c.Child(o->GetType()->GetName() + "$" + id).Clear();
									if (id == o->GetType()->GetName()) {
										c.Child(o->GetType()->GetName()).Clear();

									}
								}
							}
							//TODO clear references to that game object if it is in scene
						}
						Editor::SetDirty(Scene::Get());
						Scene::Get()->RemoveGameObject(go);
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
			}
			if (ImGui::Button("new gameObject")) {
				//falls to next context menu
			}
			if (ImGui::BeginPopupContextItem("new gameObject", ImGuiPopupFlags_MouseButtonLeft)) {
				//TODO sort types by name
				if (ImGui::Selectable("empty")) {
					auto go = std::make_shared<GameObject>();
					go->components.push_back(std::make_shared<Transform>());
					auto goId = AssetDatabase::Get()->AddObjectToAsset(AssetDatabase::Get()->GetAssetPath(Scene::Get()), go);
					auto transformId = AssetDatabase::Get()->AddObjectToAsset(AssetDatabase::Get()->GetAssetPath(Scene::Get()), go->transform());
					VarInfo v{ Scene::Get() };

					int gameObjectIndex = Scene::Get()->gameObjects.size() - Scene::Get()->instantiatedPrefabs.size();
					auto goContext = Object::Serialize(go);
					SerializationContext sceneContext{ v.yaml };
					auto gameObjectsListContext = sceneContext.Child("gameObjects");
					gameObjectsListContext.Child(gameObjectsListContext.Size()) << std::string("$" + goId);

					SerializationContext sceneRoot{ sceneContext.GetYamlNode().tree()->rootref() };

					goContext.Child("GameObject$0").Child("components").Child(0) << std::string("$" + transformId);

					int tempCount = 0;
					for (auto node : goContext.GetYamlNode().children()) {
						auto newNode = sceneRoot.GetYamlNode().append_child();
						DuplicateSerialized(node, newNode);
						if (tempCount == 0) {
							//gameobject
							newNode.set_key_serialized(("GameObject$" + goId).c_str());
						}
						else if (tempCount == 1) {
							//transform
							newNode.set_key_serialized(("Transform$" + transformId).c_str());
						}
						else {
							//alien
							ASSERT(false);
						}
						tempCount++;
					}

					Scene::Get()->AddGameObject(go);
					Editor::SetDirty(Scene::Get());
				}
				//TODO from prefab
				ImGui::EndPopup();
			}
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

			auto cam = std::dynamic_pointer_cast<Camera>(c);
			if (cam) {
				cam->OnBeforeRender();//TODO build inside GetFrustrum ?
				Dbg::Draw(cam->GetFrustum());
			}
		}
		CallOnDrawGizmos(go);
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
		if (Editor::Get()->selectedObject == nullptr) {
			Editor::Get()->selectedObject = Scene::Get();
		}
		if (Editor::Get()->selectedObject != nullptr) {
			VarInfo varInfo{ Editor::Get()->selectedObject };
			DrawInspector(*Editor::Get()->selectedObject, varInfo);
		}
		ImGui::EndChild();
		ImGui::End();

		//TODO refactor
		if (editorCamera != nullptr && Input::GetKeyDown(SDL_SCANCODE_F) && Editor::Get()->selectedObject != nullptr) {
			auto selectedGameObject = GetGameObject(Editor::Get()->selectedObject);
			if (selectedGameObject) {
				Matrix4 mat = Matrix4::Identity();
				auto rot = editorCamera->gameObject()->transform()->GetRotation();
				auto aabb = GetSphere(selectedGameObject);
				SetPos(mat, aabb.pos);
				auto offset = Matrix4::Transform(rot * Vector3_forward * aabb.radius * (-2.f), rot.ToMatrix(), Vector3_one);
				mat = mat * offset;
				editorCamera->gameObject()->transform()->SetMatrix(mat);
			}
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

		if (Editor::Get()->selectedObject) {
			//gizmos stuff
			auto go = std::dynamic_pointer_cast<GameObject>(Editor::Get()->selectedObject);

			if (go != nullptr && IsInScene(go)) {
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
				Editor::Get()->selectedObject = objectsUnderCursor[idx];
			}
			nextSelectedObjectIndex++;
		}
	}
};

REGISTER_SYSTEM(InspectorWindow);