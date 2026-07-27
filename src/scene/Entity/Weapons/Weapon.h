#pragma once

#include "scene/Entity/Entity.h"

class Weapon : public Entity
{
public:
	explicit Weapon(float fireRate);
	~Weapon() override = default;

	[[nodiscard]] float fireRate() const { return m_fireRate; }

protected:
	float m_fireRate;
};