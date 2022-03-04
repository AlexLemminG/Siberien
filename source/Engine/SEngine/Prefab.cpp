#include "Prefab.h"
#include "Object.h"
#include "GameObject.h"
#include "Resources.h"

void DuplicateSerialized(const ryml::NodeRef& from, ryml::NodeRef& to) {
	if (from.has_key()) {
		to.set_key_serialized(from.key());
	}
	if (from.has_val()) {
		to.set_val_serialized(from.val());
	}
	to |= from.type();
	for (auto child : from) {
		ryml::NodeRef toChild;
		if (from.is_seq()) {
			int idx = from.child_pos(child);
			while (to.num_children() <= idx) {
				to.append_child();
			}
			toChild = to[idx];
		}
		else if (!to.has_child(child.key())) {
			toChild = to.append_child();
		}
		else {
			toChild = to[child.key()];
		}
		DuplicateSerialized(child, toChild);
	}
}

std::shared_ptr<GameObject> PrefabInstance::CreateGameObject() const {
	auto serNode = AssetDatabase::Get()->GetOriginalSerializedAsset(AssetDatabase::Get()->GetAssetPath(prefab));
	if (serNode == nullptr) {
		ASSERT(false);
		return nullptr;
	}
	ryml::Tree serNodeClone = *serNode;
	DuplicateSerialized(overrides.rootref(), serNodeClone.rootref());

	SerializationContext c{ serNodeClone };
	c.database = AssetDatabase::Get();

	auto gameObject = std::dynamic_pointer_cast<GameObject>(Object::Instantiate(c));
	return gameObject;
}

std::shared_ptr<GameObject> PrefabInstance::GetOriginalPrefab() const { return prefab; }


//TODO move
void SerRymlTree(SerializationContext& context, const ryml::Tree& tree) {
	auto& node = context.GetYamlNode();
	DuplicateSerialized(tree.rootref(), node);
}

void DesRymlTree(const SerializationContext& context, ryml::Tree& tree) {
	auto& node = context.GetYamlNode();
	DuplicateSerialized(node, tree.rootref());
}
