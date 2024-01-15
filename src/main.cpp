#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include "animation.h"
#include "gui.h"
#include "scene.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#include "procedural_scenes.h"

enum
{
    CdsFullscreen = 4
};

constexpr GLfloat vertices[] = {
    -1.0F, -1.0F, 0.0F,
    1.0F, -1.0F, 0.0F,
    1.0F, 1.0F, 0.0F,
    1.0F, 1.0F, 0.0F,
    -1.0F, 1.0F, 0.0F,
    -1.0F, -1.0F, 0.0F
};

constexpr GLfloat uvs[] = {
    0.0F, 0.0F,
    1.0F, 0.0F,
    1.0F, 1.0F,
    1.0F, 1.0F,
    0.0F, 1.0F,
    0.0F, 0.0F,
};

int ScreenWidth = 1920, ScreenHeight = 1080;
GLuint ShaderProgram;
GLuint ScreenTexture;
bool MouseAbsorbed = false;
bool RefreshRequired = false;

glm::mat4 RotationMatrix(1);
glm::vec3 ForwardVector(0, 0, -1);

GLint DirectOutPassUniformLocation, AccumulatedPassesUniformLocation, TimeUniformLocation, CamPosUniformLocation,
       RotationMatrixUniformLocation, AspectRatioUniformLocation, DebugKeyUniformLocation;

GLuint Fbo;

void RenderAnimation(GLFWwindow* window, glm::vec3 posA, float yawA, float pitchA, glm::vec3 posB, float yawB,
                     float pitchB, int frames, int framePasses, int* renderedFrames = nullptr);

void FramebufferSizeCallback(GLFWwindow* window, const int width, const int height)
{
    glViewport(0, 0, width, height);
    ScreenWidth = width;
    ScreenHeight = height;

    glBindTexture(GL_TEXTURE_2D, ScreenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ScreenWidth, ScreenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

    RefreshRequired = true;
}

void MousebuttonCallback(GLFWwindow* window, const int button, const int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && !MouseAbsorbed && !ImGui::IsWindowHovered(
        ImGuiHoveredFlags_AnyWindow))
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (glfwGetKey(window, GLFW_KEY_E))
        {
            Scene::MousePlace(static_cast<float>(mouseX), static_cast<float>(mouseY), ScreenWidth, ScreenHeight, Scene::CameraPosition, RotationMatrix);
        }
        else
        {
            Scene::SelectHovered(static_cast<float>(mouseX), static_cast<float>(mouseY), ScreenWidth, ScreenHeight, Scene::CameraPosition, RotationMatrix);
        }
    }
}

void KeyCallback(GLFWwindow* window, const int key, int scancode, const int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            if (MouseAbsorbed)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                MouseAbsorbed = false;
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwSetCursorPos(window, ScreenWidth / 2.0, ScreenHeight / 2.0);
                MouseAbsorbed = true;

                Scene::SelectedObjectIndex = -1;
                glUniform1i(glGetUniformLocation(ShaderProgram, "u_selectedSphereIndex"), -1);
            }
        }
        else if (key == GLFW_KEY_R)
        {
            Gui::AnimationRenderWindowVisible = !Gui::AnimationRenderWindowVisible;
        }
    }
}

// Loads the shader source from disk, replaces the constants and returns a new OpenGL Program
GLuint CreateShaderProgram(const char* vertexFilePath, const char* fragmentFilePath)
{
    // Create the shaders
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string vertexShaderCode;
    if (std::ifstream vertexShaderStream(vertexFilePath, std::ios::in); vertexShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << vertexShaderStream.rdbuf();
        vertexShaderCode = sstr.str();
        vertexShaderStream.close();
    }
    else
    {
        printf("Unable to open %s.\n", vertexFilePath);
        return 0;
    }

    // Read the Fragment Shader code from the file
    std::string fragmentShaderCode;
    if (std::ifstream fragmentShaderStream(fragmentFilePath, std::ios::in); fragmentShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << fragmentShaderStream.rdbuf();
        fragmentShaderCode = sstr.str();
        fragmentShaderStream.close();
    }

    GLint result = GL_FALSE;
    int infoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertexFilePath);
    char const* vertexSourcePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShaderId, 1, &vertexSourcePointer, nullptr);
    glCompileShader(vertexShaderId);

    // Check Vertex Shader
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        std::vector<char> vertexShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderId, infoLogLength, nullptr, vertexShaderErrorMessage.data());
        printf("%s\n", vertexShaderErrorMessage.data());
    }

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragmentFilePath);
    char const* fragmentSourcePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderId, 1, &fragmentSourcePointer, nullptr);
    glCompileShader(fragmentShaderId);

    // Check Fragment Shader
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        std::vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderId, infoLogLength, nullptr, fragmentShaderErrorMessage.data());
        printf("%s\n", fragmentShaderErrorMessage.data());
    }

    // Link the program
    printf("Linking program\n");
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);

    // Check the program
    glGetProgramiv(programId, GL_LINK_STATUS, &result);
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        std::vector<char> programErrorMessage(infoLogLength + 1);
        glGetProgramInfoLog(programId, infoLogLength, nullptr, programErrorMessage.data());
        printf("%s\n", programErrorMessage.data());
    }

    glDetachShader(programId, vertexShaderId);
    glDetachShader(programId, fragmentShaderId);

    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    return programId;
}

// Uses createShaderProgram to create a program with the correct constants depending on the Scene and reassigns everything that needs to be. If a program already exists, it is deleted.
void RecompileShader()
{
    if (ShaderProgram)
        glDeleteProgram(ShaderProgram);
    ShaderProgram = CreateShaderProgram("shaders\\vertex.glsl", "shaders\\fragment.glsl");
    glUseProgram(ShaderProgram);
    Scene::Bind(ShaderProgram);

    DirectOutPassUniformLocation = glGetUniformLocation(ShaderProgram, "u_directOutputPass");
    AccumulatedPassesUniformLocation = glGetUniformLocation(ShaderProgram, "u_accumulatedPasses");
    TimeUniformLocation = glGetUniformLocation(ShaderProgram, "u_time");
    CamPosUniformLocation = glGetUniformLocation(ShaderProgram, "u_cameraPosition");
    RotationMatrixUniformLocation = glGetUniformLocation(ShaderProgram, "u_rotationMatrix");
    AspectRatioUniformLocation = glGetUniformLocation(ShaderProgram, "u_aspectRatio");
    DebugKeyUniformLocation = glGetUniformLocation(ShaderProgram, "u_debugKeyPressed");

    glUniform1i(glGetUniformLocation(ShaderProgram, "u_screenTexture"), 0);
    glUniform1i(glGetUniformLocation(ShaderProgram, "u_skyboxTexture"), 1);
}

float* LoadImageData(char const* filename, int* x, int* y, int* channelsInFile, const int desiredChannels)
{
    return stbi_loadf(filename, x, y, channelsInFile, desiredChannels);
}

void FreeImageData(void* imageData)
{
    stbi_image_free(imageData);
}

bool HandleMovementInput(GLFWwindow* window, double deltaTime, glm::vec3& cameraPosition, float& cameraYaw,
                         float& cameraPitch, glm::mat4* rotationMatrix)
{
    bool moved = false;

    double mouseX;
    double mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwSetCursorPos(window, ScreenWidth / 2.0, ScreenHeight / 2.0);

    float xOffset = static_cast<float>(mouseX - ScreenWidth / 2.0);
    float yOffset = static_cast<float>(mouseY - ScreenHeight / 2.0);

    if (xOffset != 0.0F || yOffset != 0.0F) moved = true;

    cameraYaw += xOffset * 0.002F;
    cameraPitch += yOffset * 0.002F;

    if (cameraPitch > 1.5707F)
        cameraPitch = 1.5707F;
    if (cameraPitch < -1.5707F)
        cameraPitch = -1.5707F;

    *rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), cameraPitch, glm::vec3(1, 0, 0)), cameraYaw,
                                  glm::vec3(0, 1, 0));

    glm::vec3 forward = glm::vec3(glm::vec4(0, 0, -1, 0) * (*rotationMatrix));

    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::cross(forward, up);

    glm::vec3 movementDirection(0);
    float multiplier = 1;

    if (glfwGetKey(window, GLFW_KEY_W))
    {
        movementDirection += forward;
    }
    if (glfwGetKey(window, GLFW_KEY_S))
    {
        movementDirection -= forward;
    }
    if (glfwGetKey(window, GLFW_KEY_D))
    {
        movementDirection += right;
    }
    if (glfwGetKey(window, GLFW_KEY_A))
    {
        movementDirection -= right;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE))
    {
        movementDirection += up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
    {
        movementDirection -= up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
    {
        multiplier = 5;
    }


    if (glm::length(movementDirection) > 0.0f)
    {
        cameraPosition += glm::normalize(movementDirection) * static_cast<float>(deltaTime) * (float)multiplier;
        moved = true;
    }

    return moved;
}

// Source: https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
void SaveImage(GLFWwindow* w, const int accumulatedPasses, const char* filepath)
{
    int width, height;
    glfwGetFramebufferSize(w, &width, &height);
    constexpr GLsizei nrChannels = 3;
    GLsizei stride = nrChannels * width;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    const GLsizei bufferSize = stride * height;
    float* floatBuffer = new float[bufferSize];
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, floatBuffer);

    unsigned char* byteBuffer = new unsigned char[bufferSize];
    for (int i = 0; i < bufferSize; i++)
    {
        byteBuffer[i] = static_cast<unsigned char>(std::min(
            floatBuffer[i] / static_cast<float>(accumulatedPasses), 1.0f) * 255);
    }

    stbi_flip_vertically_on_write(true);
    stbi_write_png(filepath, width, height, nrChannels, byteBuffer, stride);

    delete[] floatBuffer;
    delete[] byteBuffer;
}

void RenderAnimation(GLFWwindow* window, const glm::vec3 posA, const float yawA, const float pitchA,
                     const glm::vec3 posB, const float yawB, const float pitchB, const int frames,
                     const int framePasses, int* renderedFrames)
{
    if (renderedFrames != nullptr) *renderedFrames = 0;

    glUniform1i(glGetUniformLocation(ShaderProgram, "u_framePasses"), framePasses);
    for (int frame = 0; frame < frames; frame++)
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) break;

        const float t = static_cast<float>(frame) / static_cast<float>(frames);
        const glm::vec3 position = posA + (posB - posA) * t;
        const float yaw = yawA + (yawB - yawA) * t;
        const float pitch = pitchA + (pitchB - pitchA) * t;

        glm::mat4 rotMatrix = glm::rotate(glm::rotate(glm::mat4(1), pitch, glm::vec3(1, 0, 0)), yaw,
                                          glm::vec3(0, 1, 0));

        glUniform3f(CamPosUniformLocation, position.x, position.y, position.z);
        glUniformMatrix4fv(RotationMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(rotMatrix));
        glUniform1f(AspectRatioUniformLocation, static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight));

        glBindFramebuffer(GL_FRAMEBUFFER, Fbo);
        glUniform1i(DirectOutPassUniformLocation, 0);
        glUniform1i(AccumulatedPassesUniformLocation, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        SaveImage(window, 1, std::string("anim\\").append(std::to_string(frame)).append(".png").c_str());
        if (renderedFrames != nullptr) *renderedFrames += 1;

        std::cout << "Rendered frame " << frame << "/" << frames << '\n';
    }
    glUniform1i(glGetUniformLocation(ShaderProgram, "u_framePasses"), Scene::FramePasses);
}

int main()
{
    std::cout << "Loading skybox\n";
    int sbWidth, sbHeight, sbChannels;
    float* skyboxData = stbi_loadf("skyboxes\\kiara_9_dusk_2k.hdr", &sbWidth, &sbHeight, &sbChannels, 0);

    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW!\n";
        return -1;
    }

    GLFWwindow* programWindow = glfwCreateWindow(ScreenWidth, ScreenHeight, "OpenGL Raytracing", nullptr, nullptr);

    if (!programWindow)
    {
        std::cout << "Failed to create a window!\n";
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(programWindow, FramebufferSizeCallback);
    glfwSetMouseButtonCallback(programWindow, MousebuttonCallback);
    glfwSetKeyCallback(programWindow, KeyCallback);

    glfwMakeContextCurrent(programWindow);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW!\n";
        glfwTerminate();
        return -1;
    }

    PlaceBasicScene();
    RecompileShader();

    Gui::Init(programWindow);

    glGenTextures(1, &Scene::SkyboxTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Scene::SkyboxTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sbWidth, sbHeight, 0, GL_RGB, GL_FLOAT, skyboxData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(skyboxData);

    GLuint vertexArray;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);

    GLuint uvBuffer;
    glGenBuffers(1, &uvBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void*>(nullptr));
    glEnableVertexAttribArray(0);


    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, static_cast<void*>(nullptr));
    glEnableVertexAttribArray(1);

    glBindVertexArray(vertexArray);

    glGenTextures(1, &ScreenTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ScreenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ScreenWidth, ScreenHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &Fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, Fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ScreenTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR: Framebuffer is not complete!\n";
        return -1;
    }

    glUniform1i(glGetUniformLocation(ShaderProgram, "u_screenTexture"), 0);
    glUniform1i(glGetUniformLocation(ShaderProgram, "u_skyboxTexture"), 1);

    glViewport(0, 0, ScreenWidth, ScreenHeight);
    glDisable(GL_DEPTH_TEST);

    double deltaTime = 0.0f;
    int freezeCounter = 0;
    int accumulatedPasses = 0;
    while (!glfwWindowShouldClose(programWindow) && !Gui::ShouldQuit)
    {
        const double preTime = glfwGetTime();
        glfwPollEvents();

        if (Animation::CurrentlyRenderingAnimation)
        {
            if (Animation::CurrentFrame == -1) Animation::CurrentFrame = 0;
            // Setting currentFrame to -1 ensures we don't start writing frames before this code has been called.

            Scene::CameraPosition = Animation::CalculateCurrentCameraPosition();
            const glm::vec2 cameraOrientation = Animation::CalculateCurrentCameraOrientation();
            RotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), cameraOrientation.y, glm::vec3(1, 0, 0)),
                                         cameraOrientation.x, glm::vec3(0, 1, 0));

            if (Animation::CurrentPass == 0) RefreshRequired = true;
        }
        else
        {
            if (MouseAbsorbed)
            {
                if (HandleMovementInput(programWindow, deltaTime, Scene::CameraPosition, Scene::CameraYaw,
                                        Scene::CameraPitch, &RotationMatrix))
                {
                    RefreshRequired = true;
                }
            }
            else
            {
                RotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), Scene::CameraPitch, glm::vec3(1, 0, 0)),
                                             Scene::CameraYaw, glm::vec3(0, 1, 0));
            }


            if (glfwGetKey(programWindow, GLFW_KEY_ESCAPE) && glfwGetKey(programWindow, GLFW_KEY_LEFT_SHIFT)) break;
            glUniform1i(DebugKeyUniformLocation, glfwGetKey(programWindow, GLFW_KEY_F));
        }

        if (RefreshRequired)
        {
            accumulatedPasses = 0;
            RefreshRequired = false;
            glUniform1i(AccumulatedPassesUniformLocation, accumulatedPasses);
            // If the shader receives a value of 0 for accumulatedPasses, it will discard the buffer and just output what it rendered on that frame.
        }


        glUniform1f(TimeUniformLocation, static_cast<float>(preTime));
        glUniform3f(CamPosUniformLocation, Scene::CameraPosition.x, Scene::CameraPosition.y, Scene::CameraPosition.z);
        glUniformMatrix4fv(RotationMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(RotationMatrix));
        glUniform1f(AspectRatioUniformLocation, static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight));

        // Step 1: render to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, Fbo);
        glUniform1i(DirectOutPassUniformLocation, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        accumulatedPasses += 1;

        // Step 2: render to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUniform1i(DirectOutPassUniformLocation, 1);
        glUniform1i(AccumulatedPassesUniformLocation, accumulatedPasses);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (!MouseAbsorbed && !Animation::CurrentlyRenderingAnimation) Gui::Render();

        glfwSwapBuffers(programWindow);

        deltaTime = glfwGetTime() - preTime;
        if (deltaTime > 1.0F)
        {
            freezeCounter += 1;
            if (freezeCounter >= 2)
            {
                std::cout << "Freeze detected. Shutting down...\n";
                break;
            }
        }
        else
        {
            freezeCounter = 0;
        }

        // Because Animation::currentlyRenderingAnimation is set by the GUI, it will be true down here before it is caught above. This is why GUI will initially set the currentFrame to -1 so this code knows it must not do anything.
        if (Animation::CurrentlyRenderingAnimation && Animation::CurrentFrame >= 0)
        {
            if (Animation::CurrentPass >= Animation::FramePasses - 1)
            {
                SaveImage(programWindow, 1,
                          std::string("render_output\\").append(std::to_string(Animation::CurrentFrame)).append(".png").
                                                         c_str());

                Animation::CurrentFrame++;
                if (Animation::CurrentFrame >= Animation::TotalFrameCount)
                {
                    Animation::CurrentlyRenderingAnimation = false;
                }

                Animation::CurrentPass = 0;
            }
            else
            {
                Animation::CurrentPass += 1;
            }

            if (glfwGetKey(programWindow, GLFW_KEY_ESCAPE)) Animation::CurrentlyRenderingAnimation = false;
        }
    }

    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &uvBuffer);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteProgram(ShaderProgram);
    glDeleteFramebuffers(1, &Fbo);
    glDeleteTextures(1, &ScreenTexture);


    Gui::Cleanup();

    glfwDestroyWindow(programWindow);
    glfwTerminate();

    return 0;
}
