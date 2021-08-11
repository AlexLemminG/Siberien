#pragma once

#include "Component.h"
#include "Math.h"

class Health : public Component {
public:
	int GetAmount() const { return amount; }
	bool IsDead() const { return amount <= 0; }
	void DoDamage(int damage) { amount = Mathf::Max(0, amount - damage); }
private:
	int amount = 100;
	REFLECT_BEGIN(Health);
	REFLECT_VAR(amount);
	REFLECT_END();
};