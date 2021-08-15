#include "Health.h"
#include "Time.h"

DECLARE_TEXT_ASSET(Health);

void Health::DoDamage(int damage) {
	if (isInvinsible) {
		return;
	}
	amount = Mathf::Clamp(amount - damage, 0, maxAmount);
	lastDamageTime = Time::time();
}
