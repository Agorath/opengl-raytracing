#pragma once

#include <glm/gtc/matrix_transform.hpp>

namespace Animation
{
    extern bool CurrentlyRenderingAnimation;
    extern int CurrentFrame;
    extern int CurrentPass;
    extern int TotalFrameCount;

    extern glm::vec3 PositionA, PositionB;
    extern glm::vec2 OrientationA, OrientationB;

    extern float CameraSpeed;

    extern int FramePasses; // How many times a frame should be rendered before the combined result is saved to disk.
    extern int FrameRate;

    void SetStartPosition(glm::vec3 cameraPos, float cameraYaw, float cameraPitch);
    void SetEndPosition(glm::vec3 cameraPos, float cameraYaw, float cameraPitch);

    void RecalculateTotalFrameCount();
    glm::vec3 CalculateCurrentCameraPosition();
    glm::vec2 CalculateCurrentCameraOrientation();
}
