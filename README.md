# FPSGAME

FPSGAME is a small first-person shooter project written in C++20 with OpenGL 3.3. It is an experimental game and engine codebase focused on learning rendering, gameplay physics, animation, audio, and entity design without relying on a full game engine.

## Current features

- First-person movement, mouse look, jumping, gravity, and AABB world collision
- Textured room geometry, lighting, and an interactive swinging door
- Pistol viewmodel, recoil, muzzle effects, projectiles, impact particles, and audio
- Swept projectile collision to prevent tunneling through thin walls
- Static FBX, OBJ, glTF, and other Assimp-supported model loading
- Skeletal glTF/GLB loading with up to four bone influences per vertex
- CPU animation sampling, quaternion interpolation, and animation crossfading
- GPU skinning using an OpenGL texture-buffer bone palette
- A `Person` NPC that blends between idle and walk animations while patrolling
- An animation pose interface intended to support custom ragdoll physics later

## Requirements

The supplied build presets currently target Windows.

- Windows 10 or 11
- Visual Studio 2022 with the **Desktop development with C++** workload
- CMake 3.25 or newer
- Ninja
- Git
- [vcpkg](https://github.com/microsoft/vcpkg) installed at `C:\vcpkg`
- A GPU and driver supporting OpenGL 3.3

The dependency manifest installs GLFW, GLM, FreeType, stb, Assimp, miniaudio, and glad automatically through vcpkg.

## Installation

### 1. Install vcpkg

Open PowerShell and run:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

If vcpkg is installed somewhere else, update `CMAKE_TOOLCHAIN_FILE` in both presets in `CMakePresets.json`.

### 2. Clone the project

```powershell
git clone <repository-url> FPSGAME
cd FPSGAME
```

Replace `<repository-url>` with this repository's Git URL.

### 3. Configure and build

Run these commands from an **x64 Native Tools Command Prompt for VS 2022** or an x64 Visual Studio developer PowerShell. Using an x86 developer environment can make the linker select x86 Windows SDK libraries for the x64 executable.

Debug build:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Release build:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
```

CMake copies `src/assets` beside the executable during the build. Do not move the executable away from that generated asset directory unless you copy the `assets` folder with it.

### 4. Run the game

Debug:

```powershell
cd out\build\windows-debug
.\FPSGame.exe
```

Release:

```powershell
cd out\build\windows-release
.\FPSGame.exe
```

## Controls

| Input | Action |
| --- | --- |
| `W`, `A`, `S`, `D` | Move |
| Mouse | Look |
| `Space` | Jump |
| Left mouse button | Fire pistol |
| `E` | Interact with a nearby door |
| `Escape` | Exit |

The project also contains temporary developer controls for adjusting the pistol and hand transforms. These are tuning tools and may change without notice.

## Animated person asset

The game loads the NPC model from:

```text
src/assets/models/Characters/Person/person.glb
```

You can replace the current blocky placeholder without changing code. The replacement should:

- Be a glTF or GLB asset saved as `person.glb`
- Contain the complete humanoid mesh and a deforming armature
- Have valid vertex weights for the skinned body
- Include animation clips whose names contain `Idle` and `Walk`
- Use a Y-up orientation with the character's feet near local Y=0
- Have scale and rotation applied before export from Blender
- Include embedded textures or texture files located relative to the model

Rigid accessories without skin weights are attached to the skeleton root. Isolated unweighted vertices in an otherwise skinned mesh are attached to that mesh's dominant bone, but properly weight-painting the replacement model will produce better results.

When exporting from Blender, enable skinning and animations, include deform bones, and ensure the Idle and Walk actions are exported. The loader reports a descriptive warning and disables the NPC if the required skeletal or animation data is missing; the rest of the game continues running.

## Project structure

```text
src/
  core/          Application loop, window, and timing
  input/         Keyboard and mouse input
  physics/       AABB-based player and world collision
  rendering/     Shaders, meshes, textures, models, and skinning
  scene/         World, entities, animation, particles, and projectiles
  audio/         Audio playback
  util/          Asset and model loading helpers
  assets/        Models, textures, shaders, fonts, and audio
```

## Contributing

Contributions are welcome. Before starting a large feature, open an issue or discuss the design so it fits the project's small-engine architecture.

### Workflow

1. Fork the repository and create a focused branch:

   ```powershell
   git checkout -b (Prefix):short-description
   ```
	Prefix includes:
		D-> Defect fix
		E-> Enhancement
		F-> New Feature
		O-> Other

2. Make focused changes and avoid mixing unrelated refactors into the same contribution.
3. Add new source files to `CMakeLists.txt`.
4. Place runtime assets under `src/assets` and include their source and license information in the pull request. Do not contribute assets that cannot legally be redistributed.
5. Build the Debug configuration and manually exercise the behavior you changed.
6. Run a Release build when changing rendering, animation, physics, memory ownership, or platform code.
7. Check the patch before committing:

   ```powershell
   git diff --check
   git status --short
   ```

8. Commit with a concise description and open a pull request explaining:
   - What changed and why
   - How the change was tested
   - Any new controls, assets, dependencies, or known limitations

### Code guidelines

- Use C++20 and follow the style already present in nearby files.
- Keep ownership explicit with RAII and smart pointers.
- Treat compiler warnings as problems; the project builds with `/W4` on MSVC and `-Wall -Wextra -Wpedantic` elsewhere.
- Keep rendering, animation, physics, and gameplay responsibilities separated.
- Preserve existing public behavior unless the contribution intentionally changes it and documents the change.
- Avoid committing generated files from `out/`, local IDE settings, or vcpkg build artifacts.
- Validate external data at asset-loading boundaries and provide actionable error messages.

## Roadmap

Likely future work includes a higher-detail humanoid, custom rigid-body and joint-based ragdolls, projectile reactions, animation-to-ragdoll pose transfer, improved character collision, enemy behavior, and automated animation/loader tests.
