#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Gui
{
    extern GLFWwindow* Window;
    extern bool ShouldQuit;
    extern bool AnimationRenderWindowVisible;

    void Init(GLFWwindow* window);
    void Cleanup();

    void Render();
}
