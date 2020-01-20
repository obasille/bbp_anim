#pragma once

#include "ospcommon/vec.h"
#include "ospray/ospray.h"
#include <vector>

// Main class holding all the scene geometry and animation data
// It renders spheres bouncing from far away towards the viewer
// and drawing a text. Then the spheres play a wave animation and fade away.
// The bouncing animation is computed from the final position and played
// backward.
// The background is just a gray plane.
// The code is based on OSPRay tutorials (specifically
// ospTutorialBouncingSpheres.cpp)
class Scene
{
    using vec2f = ospcommon::vec2f;
    using vec3f = ospcommon::vec3f;
    using vec4f = ospcommon::vec4f;

public:
    Scene();
    ~Scene();

    // Get OSPRay world
    OSPModel getWorld() { return _world; }

    // Play next animation frame
    bool tick();

private:
    // Data for each sphere
    struct Sphere
    {
        // Rendering
        vec3f center{};
        float radius{};
        vec4f color{};

        // Simulation
        vec3f speed{};
        float maxHeight{};
        vec2f velocity{};
        vec3f endPos{};
        float refRadius{};
        float fadeOffDuration{};
        std::vector<vec3f> positions{};
    };

    // Generates spheres to display the given text
    void generateSpheres(std::string text);
    // Compute spheres animations
    void computeAnimations();
    // Creates OSPVRay geometry object for the spheres
    OSPGeometry createSpheresGeometry();
    // Creates OSPVRay geometry object for background
    OSPGeometry createBackgroundGeometry();
    // Create OSPVRay object holding the scene geometry
    void createWorld();
    // Commit geometry changes
    void updateSpheresGeometry();

    // Our list of animated spheres
    std::vector<Sphere> _spheres;

    // OSPRay objects
    OSPGeometry _spheresGeometry;
    OSPModel _world;

    //
    // Animation stuff
    //
    const float _deltaTime = 0.025f;

    // The animation goes through these different states
    enum class AnimPhase
    {
        playback,
        wave,
        delay,
        fadeOut,
        done,
    };

    // Store the current state of the spheres animation
    // It's a simple state machine
    class AnimState
    {
    public:
        // Animate to the next frame
        bool operator()(std::vector<Sphere>& spheres, const float deltaTime);

    private:
        bool doPlayback(std::vector<Sphere>& spheres);
        bool doWave(std::vector<Sphere>& spheres);
        bool doFadeOut(std::vector<Sphere>& spheres);

        AnimPhase phase = AnimPhase::playback;  // Current animation phase
        float _t = 0.f;                         // Current time
        float _t0 = 0.f;                        // Start time of the current phase
        int _playbackIndex = 0;
        float _waveX0 = 0.f, _waveX1 = 0.f;
    } _animState;
};