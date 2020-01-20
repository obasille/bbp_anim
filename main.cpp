#include "ospray-tutorial/GLFWOSPRayWindow.h"
#include "scene.h"
#include "utils.h"
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <memory>

using namespace ospcommon;

OSPRenderer createRenderer()
{
    // create OSPRay renderer
    OSPRenderer renderer = ospNewRenderer("pathtracer");

    // create an ambient light
    OSPLight ambientLight = ospNewLight3("ambient");
    ospCommit(ambientLight);

    // create lights data containing all lights
    OSPLight lights[] = {ambientLight};
    OSPData lightsData = ospNewData(1, OSP_LIGHT, lights, 0);
    ospCommit(lightsData);

    // set the lights to the renderer
    ospSetData(renderer, "lights", lightsData);

    // commit the renderer
    ospCommit(renderer);

    // release handles we no longer need
    ospRelease(lightsData);

    return renderer;
}

// Based on OSPRay tutorial => ospTutorialBouncingSpheres.cpp
void renderToScreen(int argc, const char **argv)
{
    Scene scene{};

    // create OSPRay renderer
    OSPRenderer renderer = createRenderer();

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
        new GLFWOSPRayWindow(vec2i{1140, 640}, box3f(vec3f(-1.f), vec3f(1.f)),
                             scene.getWorld(), renderer));

    // register a callback with the GLFW OSPRay window to update the model every
    // frame
    glfwOSPRayWindow->registerDisplayCallback(
        [&](GLFWOSPRayWindow *glfwOSPRayWindow) {
            // update the spheres coordinates and geometry
            if (scene.tick())
            {
                // update the model on the GLFW window
                glfwOSPRayWindow->setModel(scene.getWorld());
            }
        });

    glfwOSPRayWindow->registerImGuiCallback([=]() {
        static int spp = 1;
        if (ImGui::SliderInt("spp", &spp, 1, 64))
        {
            ospSet1i(renderer, "spp", spp);
            ospCommit(renderer);
        }
    });

    // start the GLFW main loop, which will continuously render
    glfwOSPRayWindow->mainLoop();

    ospRelease(renderer);
}

// Based on OSPRay tutorial => ospTutorial.c
void renderToFiles(int argc, const char **argv)
{
    // image size
    osp::vec2i imgSize;
    imgSize.x = 1280; // width
    imgSize.y = 720;  // height

    Scene scene{};

    // create OSPRay model
    OSPModel model = scene.getWorld();

    // create OSPRay renderer
    OSPRenderer renderer = createRenderer();

    // OSPRay setup

    // set the model on the renderer
    ospSetObject(renderer, "model", model);

    // create the arcball camera model
    vec2i windowSize{imgSize.x, imgSize.y};
    box3f worldBounds(vec3f(-1.f), vec3f(1.f));
    auto arcballCamera = std::unique_ptr<ArcballCamera>(
        new ArcballCamera(worldBounds, windowSize));

    // create camera
    auto camera = ospNewCamera("perspective");
    ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));

    ospSetVec3f(camera, "pos",
                osp::vec3f{arcballCamera->eyePos().x, arcballCamera->eyePos().y,
                           arcballCamera->eyePos().z});
    ospSetVec3f(camera, "dir",
                osp::vec3f{arcballCamera->lookDir().x,
                           arcballCamera->lookDir().y,
                           arcballCamera->lookDir().z});
    ospSetVec3f(camera, "up",
                osp::vec3f{arcballCamera->upDir().x, arcballCamera->upDir().y,
                           arcballCamera->upDir().z});

    ospCommit(camera);

    // set camera on the renderer
    ospSetObject(renderer, "camera", camera);

    // finally, commit the renderer
    ospCommit(renderer);

    // create and setup framebuffer
    OSPFrameBuffer framebuffer =
        ospNewFrameBuffer(imgSize, OSP_FB_SRGBA,
                          OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);

    std::cout << "Generating frames..." << std::endl;

    // Count frames for naming output file
    int frameIndex = 0;
    std::ostringstream str{};

    // Iterate until nothing to update
    while (scene.tick())
    {
        ++frameIndex;

        ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

        // render 10 more frames, which are accumulated to result in a better
        // converged image
        for (int frames = 0; frames < 20; frames++)
            ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);

        // write result into file
        str.str("");
        str << "frame" << frameIndex << ".ppm";
        const uint32_t *fb =
            (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
        utils::writePPM(str.str().data(), vec2i{imgSize.x, imgSize.y}, fb);
        ospUnmapFrameBuffer(fb, framebuffer);

        std::cout << "Frame #" << frameIndex << " generated" << std::endl;
    }

    // final cleanups
    ospRelease(framebuffer);
    ospRelease(renderer);
}

int main(int argc, const char **argv)
{
    // initialize OSPRay; OSPRay parses (and removes) its commandline
    // parameters, e.g. "--osp:debug"
    OSPError initError = ospInit(&argc, argv);

    if (initError != OSP_NO_ERROR)
        return initError;

    // set an error callback to catch any OSPRay errors and exit the application
    ospDeviceSetErrorFunc(ospGetCurrentDevice(), [](OSPError error,
                                                    const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
    });

    renderToScreen(argc, argv);
    //renderToFiles(argc, argv);

    // cleanly shut OSPRay down
    ospShutdown();

    return 0;
}