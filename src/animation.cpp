#include "animation.h"

namespace Animation
{
    bool CurrentlyRenderingAnimation = false;
    int CurrentFrame = 0;
    int CurrentPass = 0;
    int TotalFrameCount;

    glm::vec3 PositionA, PositionB;
    glm::vec2 OrientationA, OrientationB;

    float CameraSpeed = 1.0f;

    int FramePasses = 16;
    int FrameRate = 24;

    void SetStartPosition(const glm::vec3 cameraPos, const float cameraYaw, const float cameraPitch)
    {
        PositionA = cameraPos;
        OrientationA = glm::vec2(cameraYaw, cameraPitch);
    }

    void SetEndPosition(const glm::vec3 cameraPos, const float cameraYaw, const float cameraPitch)
    {
        PositionB = cameraPos;
        OrientationB = glm::vec2(cameraYaw, cameraPitch);
    }

    void RecalculateTotalFrameCount()
    {
        TotalFrameCount = static_cast<int>(distance(PositionA, PositionB) / CameraSpeed * static_cast<float>(FrameRate));
    }

    glm::vec3 CalculateCurrentCameraPosition()
    {
        return mix(PositionA, PositionB, static_cast<float>(CurrentFrame) / static_cast<float>(TotalFrameCount));
    }

    glm::vec2 CalculateCurrentCameraOrientation()
    {
        return mix(OrientationA, OrientationB, static_cast<float>(CurrentFrame) / static_cast<float>(TotalFrameCount));
    }
}
