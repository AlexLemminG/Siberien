#pragma once

#include "System.h"

class GameObject;

class CorpseRemoveSystem : public System<CorpseRemoveSystem>{
	virtual void Update() override;
public:
	void Add(std::shared_ptr<GameObject> corpse) { corpses.push_back(corpse); }
private:
	int maxCorpsesCount = 100;
	std::vector<std::shared_ptr<GameObject>> corpses;
};