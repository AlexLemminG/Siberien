#include "Common.h"
#include "Reflect.h"
#include "Serialization.h"
#include <dear-imgui/imgui.h>
#include "System.h"
#include "GameObject.h"
#include "Scene.h"



class InspectorWindow : public System<InspectorWindow> {
	std::shared_ptr<Object> selectedObject;

	void DrawInspector(char* object, std::string name, ReflectedTypeBase* type) {
		if (type->GetName() == ::GetReflectedType<float>()->GetName()) {
			float* f = (float*)(object);
			ImGui::DragFloat(name.c_str(), f, 0.1f);
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

	void Update() {
		auto scene = Scene::Get();
		if (!scene) {
			return;
		}
		ImGui::Begin("Inspector");

		static std::string filter;
		{
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
				// FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
				std::string name = gameObjects[i]->tag.size() > 0 ? gameObjects[i]->tag.c_str() : "-";
				if (filter.size() > 0) {
					if (name.find(filter) == -1) {
						continue;//TODO ignoreCase
					}
				}

				ImGui::PushID(i);
				if (ImGui::Selectable(gameObjects[i]->tag.size() > 0 ? gameObjects[i]->tag.c_str() : "-", selectedObject == gameObjects[i])) {
					selectedObject = gameObjects[i];
				}
				ImGui::PopID();
			}
			ImGui::EndChild();
		}
		ImGui::SameLine();

		ImGui::BeginGroup();
		if (selectedObject != nullptr) {
			DrawInspector(*selectedObject);
		}
		ImGui::EndGroup();
		ImGui::End();
	}
};

REGISTER_SYSTEM(InspectorWindow);