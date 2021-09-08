#pragma once

#include "Reflect.h"
#include "ryml.hpp"

//TODO move
void SerRymlTree(SerializationContext& context, const ryml::Tree& tree);
void DesRymlTree(const SerializationContext& context, ryml::Tree& tree);
REFLECT_CUSTOM_EXT(ryml::Tree, SerRymlTree, DesRymlTree);


class PrefabInstance {
public:
	std::shared_ptr<GameObject> CreateGameObject() const;
private:
	std::shared_ptr<GameObject> prefab;
	ryml::Tree overrides;


	REFLECT_BEGIN(PrefabInstance);
	REFLECT_VAR(prefab);
	REFLECT_VAR(overrides);
	REFLECT_END();
};