#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "VulkanApp.h"
#include <iostream>

int main()
{
    ParticlesApp app;
    ParticlesApp::Config cfg;
    cfg.cam.velocity = { 0.0f, 0.0f, 0.0f };
    cfg.cam.pos = { 0.0f, 0.0f, 0.0f };
    cfg.cam.yaw = 0.0;
    cfg.cam.pitch = 0.0;
    cfg.movement_speed = 3.0f;
    cfg.sesitivity = 0.0008f;

    try
    {
        ParticlesApp::config() = cfg;
        app.init();
        app.run();
        app.shutdown();
    }
    catch (std::invalid_argument& e)
    {
        std::cout << "\n\n";
        std::cout << "Invalid argument exception occured:\nWhat: " << e.what() << std::endl;
        std::cout << "\n\n";
    }
    catch (std::runtime_error& e)
    {
        std::cout << "\n\n";
        std::cout << "Runtime error occured:\nWhat: " << e.what() << std::endl;
        std::cout << "\n\n";
    }
    catch (std::exception& e)
    {
        std::cout << "\n\n";
        std::cout << "Unhandled exception occured:\nWhat: " << e.what() << std::endl;
        std::cout << "\n\n";
    }
}