cmake_minimum_required(VERSION 3.25)

project(FPSGame
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)

add_executable(FPSGame
    src/main.cpp
    src/core/Application.h
    src/core/Application.cpp
    src/core/Window.h
    src/core/Window.cpp
    src/core/Time.h
    src/core/Time.cpp
    src/rendering/Shader.h
    src/rendering/Shader.cpp
    src/rendering/Mesh.h
    src/rendering/Mesh.cpp
    src/rendering/Renderer.h
    src/rendering/Renderer.cpp
    src/rendering/TextRenderer.h
    src/rendering/TextRenderer.cpp
    src/rendering/Camera.h
    src/rendering/Camera.cpp
    src/rendering/Texture.h
    src/rendering/Texture.cpp
    src/rendering/Model.h
    src/rendering/Model.cpp
    src/rendering/SkinnedMesh.h
    src/rendering/SkinnedMesh.cpp
    src/rendering/SkeletalModel.h
    src/rendering/SkeletalModel.cpp
    src/input/InputManager.h
    src/input/InputManager.cpp
    src/util/FileUtils.h
    src/util/FileUtils.cpp
    src/util/MeshFactory.h
    src/util/MeshFactory.cpp
    src/util/ModelLoader.h
    src/util/ModelLoader.cpp
    src/util/SkeletalModelLoader.h
    src/util/SkeletalModelLoader.cpp
    src/scene/Scene.h
    src/scene/Scene.cpp
    src/scene/Entity/Entity.h
    src/scene/Entity/Entity.cpp
    src/scene/Animation/Animator.h
    src/scene/Animation/Animator.cpp
    src/scene/Entity/Characters/Person.h
    src/scene/Entity/Characters/Person.cpp
    src/scene/Entity/Weapons/Weapon.h
    src/scene/Entity/Weapons/Weapon.cpp
    src/scene/Entity/Weapons/Pistol.h
    src/scene/Entity/Weapons/Pistol.cpp
    src/physics/AABB.h
    src/physics/PhysicsWorld.h
    src/physics/PhysicsWorld.cpp
    src/physics/RigidBodyWorld.h
    src/physics/RigidBodyWorld.cpp
    src/rendering/Light.h
    src/scene/ProjectileManager.h
    src/scene/ProjectileManager.cpp
    src/scene/Entity/Objects/Bullet.h
    src/scene/Entity/Objects/Door.h
    src/scene/Entity/Objects/Door.cpp
    src/scene/ParticleSystem.h
    src/scene/ParticleSystem.cpp
    src/audio/AudioEngine.h
    src/audio/AudioEngine.cpp
    src/scene/Entity/Player/Hand.h
    src/scene/Entity/Player/Hand.cpp)

target_include_directories(FPSGame
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Stb_INCLUDE_DIR}
)

target_link_libraries(FPSGame
    PRIVATE
        OpenGL::GL
        glfw
        glm::glm
        glad::glad
        freetype
        assimp::assimp
        BulletDynamics
        BulletCollision
        LinearMath
)

if(MSVC)
    target_compile_options(FPSGame PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(FPSGame PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT FPSGame
)

file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets.stamp
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
        $<TARGET_FILE_DIR:FPSGame>/assets
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/assets.stamp
    DEPENDS ${ASSET_FILES}
    COMMENT "Copying assets..."
)

add_custom_target(CopyAssets ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/assets.stamp
)

add_dependencies(FPSGame CopyAssets)
