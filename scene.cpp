#include "scene.h"

#include <cassert>
#include <cmath>
#include <random>

#include "fonts.h"
#include "utils.h"

using namespace ospcommon;

Scene::Scene()
{
    // Create everything!
    createWorld();
}

Scene::~Scene()
{
    ospRelease(_spheresGeometry);
    ospRelease(_world);
}

void Scene::generateSpheres(std::string text)
{
    assert(_spheres.empty());

    const float letterSize = 0.2f;
    const float lineHeight = 0.3f;
    const float pixelSize = letterSize / fonts::lettersWidth;
    const float sphereRadius =
        0.48f * pixelSize; // Just a bit of space between spheres
    const float destX = -2.f;
    const float destY = 0.5f;
    const float destZ = 0;

    // Create random number distributions
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_real_distribution<float> xVelocityDistribution{-0.2f, 0.2f};
    std::uniform_real_distribution<float> zVelocityDistribution{0.8f, 1.2f};
    std::uniform_real_distribution<float> fadeOffSpeedDistribution{0.2f, 1.f};

    // Render text and create a sphere for each pixel
    fonts::renderText(text, [&](float x, float y) {
        auto &s = _spheres.emplace_back(); // C++ 17 required
        s.radius = s.refRadius = sphereRadius;

        // Position
        float yFract, yInt;
        yFract = std::modff(y, &yInt);
        s.center.x = destX + letterSize * x;
        s.center.y = destY - letterSize * yFract - lineHeight * yInt;
        s.center.z = destZ;

        // Animation data
        s.maxHeight = s.center.y;
        s.endPos = s.center;
        s.fadeOffDuration = fadeOffSpeedDistribution(gen);
        s.velocity.x = xVelocityDistribution(gen);
        s.velocity.y = -zVelocityDistribution(gen);
    });

    // Once we have all the spheres, set their colors to do a rainbow
    float i = 0;
    for (auto &s : _spheres)
    {
        auto rgb = utils::hsl2RGB(180 * i / _spheres.size(), 1, 0.5f);
        s.color.x = rgb.x;
        s.color.y = rgb.y;
        s.color.z = rgb.z;
        s.color.w = 1;
        ++i;
    }
}

void Scene::computeAnimations()
{
    const int numFrames = 150;
    const float g = 9.81f;

    for (auto &s : _spheres)
    {
        // Allocate memory for the all the frames
        s.positions.resize(numFrames);

        // Move the sphere away and store each position, the animation will be
        // played backward
        float t = 0;
        vec3f pos = s.center;
        for (auto rit = s.positions.rbegin(); rit != s.positions.rend(); ++rit)
        {
            const float maxHeight = 1 + s.maxHeight;
            const float T = sqrtf(8.f * maxHeight / g);
            const float Vmax = sqrtf(2.f * maxHeight * g);
            const float tRemainder = std::fmodf(0.5f * T + t, T);

            pos.y = -1.f + s.radius - 0.5f * g * tRemainder * tRemainder +
                    Vmax * tRemainder;

            // Add some side movements
            pos.x += _deltaTime * s.velocity.x;
            pos.z += _deltaTime * s.velocity.y;

            // Store result
            *rit = pos;
            t += _deltaTime;
        }
    }
}

bool Scene::AnimState::operator()(std::vector<Sphere> &spheres,
                                  const float deltaTime)
{
    bool done = false;

    // Play animation for the current phase, and move to next phase when current
    // phase is done
    switch (phase)
    {
    case AnimPhase::playback:
    {
        if (!doPlayback(spheres))
        {
            phase = AnimPhase::wave;
            _t0 = _t + deltaTime;
        }
    }
    break;

    case AnimPhase::wave:
        if (!doWave(spheres))
        {
            phase = AnimPhase::delay;
            _t0 = _t + deltaTime;
        }
        break;

    case AnimPhase::delay:
    {
        if ((_t - _t0) >= 1)
        {
            phase = AnimPhase::fadeOut;
            _t0 = _t + deltaTime;
        }
    }
    break;
    case AnimPhase::fadeOut:
    {
        if (!doFadeOut(spheres))
        {
            phase = AnimPhase::done;
        }
    }
    break;

    // Done
    default:
        assert(false);
    case AnimPhase::done:
        done = true;
        break;
    }

    // Increment time
    _t += deltaTime;

    return !done;
}

bool Scene::AnimState::doPlayback(std::vector<Sphere> &spheres)
{
    bool updated = false;
    for (auto &s : spheres)
    {
        if (_playbackIndex < s.positions.size())
        {
            s.center = s.positions[_playbackIndex];
            updated = true;
        }
        else
        {
            s.center = s.endPos;
        }
    }

    ++_playbackIndex;

    return updated;
}

bool Scene::AnimState::doWave(std::vector<Sphere> &spheres)
{
    bool updated = false;

    if ((_waveX0 == _waveX1) && (!spheres.empty()))
    {
        _waveX0 = spheres.front().center.x;
        _waveX1 = spheres.back().center.x;
    }

    float tRel = _t - _t0;

    // Make a wave go across the spheres field
    for (auto &s : spheres)
    {
        const float waveWidth = 0.5f;
        const float waveStrength = 0.05f;
        const float waveSpeed = 1.f;
        const float scaleFactor = 2.f;

        const float dx = (s.center.x - _waveX0) / (_waveX1 - _waveX0);
        float tWave = tRel * waveSpeed - dx;

        if ((tWave >= 0.f) && (tWave <= waveWidth))
        {
            tWave /= waveWidth;
            const float wave =
                1.f + std::sinf(float(std::_Pi) * (2 * tWave - 0.5f));
            s.center.z = s.endPos.z + waveStrength * wave;

            const float a = std::max(0.f, (tWave < 0.5f ? tWave : 1.f - tWave));
            s.radius = s.refRadius * (1.f + scaleFactor * a);

            updated = true;
        }
    }

    return updated;
}

bool Scene::AnimState::doFadeOut(std::vector<Sphere> &spheres)
{
    bool updated = false;

    float tRel = _t - _t0;

    // Fade out all the spheres
    for (auto &s : spheres)
    {
        float scale = std::max(0.f, 1.f - tRel / s.fadeOffDuration);
        s.radius = s.refRadius * scale;
        if (scale > 0.f)
        {
            updated = true;
        }
    }

    return updated;
}

OSPGeometry Scene::createSpheresGeometry()
{
    generateSpheres("The Blue Brain\nProject is\nmindblowing!");
    computeAnimations();

    // create a data object with all the sphere information
    OSPData spheresData = ospNewData(_spheres.size() * sizeof(Sphere),
                                     OSP_UCHAR, _spheres.data());

    // create the sphere geometry, and assign attributes
    _spheresGeometry = ospNewGeometry("spheres");

    ospSetData(_spheresGeometry, "spheres", spheresData);
    ospSet1i(_spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
    ospSet1i(_spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
    ospSet1i(_spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

    ospSetData(_spheresGeometry, "color", spheresData);
    ospSet1i(_spheresGeometry, "color_offset", int(offsetof(Sphere, color)));
    ospSet1i(_spheresGeometry, "color_format", int(OSP_FLOAT4));
    ospSet1i(_spheresGeometry, "color_stride", int(sizeof(Sphere)));

    // create alloy material and assign to geometry
    OSPMaterial alloyMaterial = ospNewMaterial2("pathtracer", "Alloy");
    ospCommit(alloyMaterial);

    ospSetMaterial(_spheresGeometry, alloyMaterial);

    // commit the spheres geometry
    ospCommit(_spheresGeometry);

    // release handles we no longer need
    ospRelease(spheresData);
    ospRelease(alloyMaterial);

    return _spheresGeometry;
}

OSPGeometry Scene::createBackgroundGeometry()
{
    OSPGeometry planeGeometry = ospNewGeometry("quads");

    struct Vertex
    {
        vec3f position;
        vec3f normal;
        vec4f color;
    };

    struct QuadIndex
    {
        int x;
        int y;
        int z;
        int w;
    };

    std::vector<Vertex> vertices;
    std::vector<QuadIndex> quadIndices;

    // ground plane
    int startingIndex = vertices.size();

    // extent of plane in the (x, y) directions
    const float planeExtent = 20.f;
    const float planeZ = -10;

    const vec3f back = vec3f{0.f, 0.f, -1.f};
    const vec4f gray = vec4f{0.05f, 0.05f, 0.05f, 1.f};

    vertices.push_back(
        Vertex{vec3f{-planeExtent, -planeExtent, planeZ}, back, gray});
    vertices.push_back(
        Vertex{vec3f{planeExtent, -planeExtent, planeZ}, back, gray});
    vertices.push_back(
        Vertex{vec3f{planeExtent, planeExtent, planeZ}, back, gray});
    vertices.push_back(
        Vertex{vec3f{-planeExtent, planeExtent, planeZ}, back, gray});

    quadIndices.push_back(QuadIndex{startingIndex, startingIndex + 1,
                                    startingIndex + 2, startingIndex + 3});

    // create OSPRay data objects
    std::vector<vec3f> positionVector;
    std::vector<vec3f> normalVector;
    std::vector<vec4f> colorVector;

    std::transform(vertices.begin(), vertices.end(),
                   std::back_inserter(positionVector),
                   [](Vertex const &v) { return v.position; });
    std::transform(vertices.begin(), vertices.end(),
                   std::back_inserter(normalVector),
                   [](Vertex const &v) { return v.normal; });
    std::transform(vertices.begin(), vertices.end(),
                   std::back_inserter(colorVector),
                   [](Vertex const &v) { return v.color; });

    OSPData positionData =
        ospNewData(vertices.size(), OSP_FLOAT3, positionVector.data());
    OSPData normalData =
        ospNewData(vertices.size(), OSP_FLOAT3, normalVector.data());
    OSPData colorData =
        ospNewData(vertices.size(), OSP_FLOAT4, colorVector.data());
    OSPData indexData =
        ospNewData(quadIndices.size(), OSP_INT4, quadIndices.data());

    // set vertex / index data on the geometry
    ospSetData(planeGeometry, "vertex", positionData);
    ospSetData(planeGeometry, "vertex.normal", normalData);
    ospSetData(planeGeometry, "vertex.color", colorData);
    ospSetData(planeGeometry, "index", indexData);

    // create and assign a material to the geometry
    OSPMaterial material = ospNewMaterial2("pathtracer", "OBJMaterial");
    ospCommit(material);

    ospSetMaterial(planeGeometry, material);

    // finally, commit the geometry
    ospCommit(planeGeometry);

    // release handles we no longer need
    ospRelease(positionData);
    ospRelease(normalData);
    ospRelease(colorData);
    ospRelease(indexData);
    ospRelease(material);

    return planeGeometry;
}

void Scene::createWorld()
{
    assert(_world == nullptr);

    // create the "world" model which will contain all of our geometries
    _world = ospNewModel();

    // add in spheres geometry (100 of them)
    ospAddGeometry(_world, createSpheresGeometry());

    // add in background plane geometry
    ospAddGeometry(_world, createBackgroundGeometry());

    // commit the world model
    ospCommit(_world);
}

void Scene::updateSpheresGeometry()
{
    // create new spheres data for the updated center coordinates, and assign to
    // geometry
    OSPData spheresData = ospNewData(_spheres.size() * sizeof(Sphere),
                                     OSP_UCHAR, _spheres.data());

    ospSetData(_spheresGeometry, "spheres", spheresData);

    // commit the updated spheres geometry
    ospCommit(_spheresGeometry);

    // release handles we no longer need
    ospRelease(spheresData);
}

// updates the bouncing spheres' coordinates, geometry, and model
bool Scene::tick()
{
    // update the spheres coordinates and geometry
    if (_animState(_spheres, _deltaTime))
    {
        updateSpheresGeometry();

        // commit the model since the spheres geometry changed
        ospCommit(_world);

        return true;
    }

    return false;
}
