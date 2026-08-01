#pragma once

#include "audio/AudioEngine.h"
#include "core/Window.h"
#include "core/Time.h"
#include "input/InputManager.h"
#include "physics/PhysicsWorld.h"
#include "rendering/Camera.h"
#include "rendering/Light.h"
#include "rendering/Renderer.h"
#include "rendering/Shader.h"
#include "rendering/TextRenderer.h"
#include "scene/Scene.h"
#include "scene/ParticleSystem.h"
#include "scene/ProjectileManager.h"
#include "scene/Entity/Weapons/Pistol.h"
#include "scene/Entity/Objects/Door.h"
#include "scene/Entity/Player/Hand.h"
#include "scene/Entity/Characters/Person.h"

#include <memory>

class Application
{
public:
	Application(const int width, const int height, const std::string& title);
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void run();

private:
	void processInput();
	void render() const;

	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputManager> m_input;
	std::unique_ptr<Shader> m_shader;
	std::unique_ptr<Shader> m_textShader;
	std::unique_ptr<Shader> m_skeletalShader;
	std::unique_ptr<Camera> m_camera;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<TextRenderer> m_textRenderer;
	std::unique_ptr<Scene> m_scene;
	std::unique_ptr<Pistol> m_pistol;
	std::unique_ptr<PhysicsWorld> m_physics;
	std::unique_ptr<ProjectileManager> m_projectiles;
	std::unique_ptr<ParticleSystem> m_particles;
	std::unique_ptr<Door> m_door;
	std::unique_ptr<AudioEngine> m_audio;
	std::unique_ptr<Hand> m_hand;
	std::shared_ptr<SkeletalModel> m_personModel;
	std::unique_ptr<Person> m_person;

	Light m_light;

	glm::vec3 m_bodyPosition;

	Time m_time;
	int m_width;
	int m_height;
};
