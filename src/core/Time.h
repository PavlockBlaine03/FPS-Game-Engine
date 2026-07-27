#pragma once

class Time
{
public:
	void update();

	[[nodiscard]] float deltaTime() const { return m_deltaTime; }
	[[nodiscard]] float elapsedTime() const { return m_lastFrameTime; }

private:
	float m_deltaTime = 0.0f;
	float m_lastFrameTime = 0.0f;

	static constexpr float MAX_DELTA_TIME = 0.05f;
};