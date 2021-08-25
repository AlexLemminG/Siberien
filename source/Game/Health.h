#pragma once

#include "Component.h"
#include "SMath.h"

class Health : public Component {
public:
	int GetAmount() const { return amount; }
	int GetMaxAmount() const { return maxAmount; }
	bool IsDead() const { return amount <= 0; }
	void DoDamage(int damage);
	void DoHeal(int heal) { amount = Mathf::Clamp(amount + heal, 0, maxAmount); }

	void OnEnable() override { maxAmount = amount; }

	float GetLastDamageTime() { return lastDamageTime; }

	void SetInvinsible(bool isInvinsible) {
		this->isInvinsible = isInvinsible;
	}
private:
	int maxAmount = 100;
	int amount = 100;
	float lastDamageTime = -100.f;
	bool isInvinsible = false;
	REFLECT_BEGIN(Health);
	REFLECT_VAR(amount);
	REFLECT_END();
};