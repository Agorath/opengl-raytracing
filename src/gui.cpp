#include "gui.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "animation.h"
#include "scene.h"

// The excessive use of the extern keyword here is probably not ideal. These functions should be declared in a header file but trying that resulted in linking errors whereas this works fine.
extern float* LoadImageData(char const* filename, int* x, int* y, int* channelsInFile, int desiredChannels);
extern void FreeImageData(void* imageData);
extern bool RefreshRequired;

namespace Gui
{
    GLFWwindow* Window;
    bool ShouldQuit = false;
    bool AnimationRenderWindowVisible = false;

    void Init(GLFWwindow* window)
    {
        Window = window;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        const ImGuiIO& io = ImGui::GetIO();
        (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        io.Fonts->AddFontFromFileTTF("OpenSans-Bold.ttf", 15.0f);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
    }

    void Cleanup()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    std::string ArrayElementName(const char* arrayName, const int index, const char* keyName)
    {
        return std::string(arrayName).append("[").append(std::to_string(index)).append("].").append(keyName);
    }

    void ShaderFloatParameter(const char* name, const char* displayName, float* floatPtr)
    {
        ImGui::Text("%s", displayName);
        ImGui::SameLine();
        if (ImGui::InputFloat(std::string("##").append(name).c_str(), floatPtr))
        {
            if (Scene::BoundShader)
                glUniform1f(glGetUniformLocation(Scene::BoundShader, name), *floatPtr);
            RefreshRequired = true;
        }
    }

    void ShaderSliderParameter(const char* name, const char* displayName, float* floatPtr)
    {
        ImGui::Text("%s", displayName);
        ImGui::SameLine();
        if (ImGui::SliderFloat(std::string("##").append(name).c_str(), floatPtr, 0.0f, 1.0f))
        {
            if (Scene::BoundShader)
                glUniform1f(glGetUniformLocation(Scene::BoundShader, name), *floatPtr);
            RefreshRequired = true;
        }
    }

    void ShaderVecParameter(const char* name, const char* displayName, float* floatPtr)
    {
        ImGui::Text("%s", displayName);
        ImGui::SameLine();
        if (ImGui::InputFloat3(std::string("##").append(name).c_str(), floatPtr))
        {
            if (Scene::BoundShader)
            {
                glUniform3f(glGetUniformLocation(Scene::BoundShader, name), *(floatPtr + 0),
                            *(floatPtr + 1), *(floatPtr + 2));
            }
            RefreshRequired = true;
        }
    }

    void ShaderColorParameter(const char* name, const char* displayName, float* floatPtr)
    {
        ImGui::Text("%s", displayName);
        ImGui::SameLine();
        if (ImGui::ColorPicker3(name, floatPtr))
        {
            if (Scene::BoundShader)
            {
                glUniform3f(glGetUniformLocation(Scene::BoundShader, name), *(floatPtr + 0),
                            *(floatPtr + 1), *(floatPtr + 2));
            }
            RefreshRequired = true;
        }
    }

    void ObjectSettingsUi()
    {
        ImGui::Begin("Selected object");

        ImGui::PushItemWidth(-1);
        if (Scene::SelectedObjectIndex != -1)
        {
            const int i = Scene::SelectedObjectIndex;
            const std::string indexStr = std::to_string(i);

            ImGui::Text("%s", std::string("Object #").append(indexStr).c_str());

            ShaderVecParameter(ArrayElementName("u_objects", i, "position").c_str(), "Position",
                               Scene::Objects[i].m_Position);

            ImGui::Text("Is box");
            ImGui::SameLine();
            bool isBox = Scene::Objects[i].m_Type == 2;
            const std::string typeVariableName = ArrayElementName("u_objects", i, "type");
            const std::string scaleVariableName = ArrayElementName("u_objects", i, "scale");
            if (ImGui::Checkbox(std::string("##").append(typeVariableName).c_str(), &isBox))
            {
                Scene::Objects[i].m_Type = isBox ? 2 : 1;
                if (Scene::BoundShader)
                {
                    glUniform1ui(glGetUniformLocation(Scene::BoundShader, typeVariableName.c_str()),
                                 Scene::Objects[i].m_Type);
                }
                RefreshRequired = true;

                if (isBox)
                {
                    Scene::Objects[i].m_Scale[0] *= 2.0f;
                    Scene::Objects[i].m_Scale[1] *= 2.0f;
                    Scene::Objects[i].m_Scale[2] *= 2.0f;
                }
                else
                {
                    const float minDimension = std::min({
                        Scene::Objects[i].m_Scale[0], Scene::Objects[i].m_Scale[1], Scene::Objects[i].m_Scale[2]
                    });
                    Scene::Objects[i].m_Scale[0] = minDimension / 2.0f;
                    Scene::Objects[i].m_Scale[1] = minDimension / 2.0f;
                    Scene::Objects[i].m_Scale[2] = minDimension / 2.0f;
                }

                if (Scene::BoundShader)
                {
                    glUniform3f(glGetUniformLocation(Scene::BoundShader, scaleVariableName.c_str()),
                                Scene::Objects[i].m_Scale[0], Scene::Objects[i].m_Scale[1],
                                Scene::Objects[i].m_Scale[2]);
                }
            }

            if (Scene::Objects[i].m_Type == 1)
            {
                ImGui::Text("Radius");
                ImGui::SameLine();
                if (ImGui::InputFloat(std::string("##").append(scaleVariableName).c_str(),
                                      &Scene::Objects[i].m_Scale[0]))
                {
                    Scene::Objects[i].m_Scale[1] = Scene::Objects[i].m_Scale[0];
                    Scene::Objects[i].m_Scale[2] = Scene::Objects[i].m_Scale[0];
                    if (Scene::BoundShader)
                    {
                        glUniform3f(
                            glGetUniformLocation(Scene::BoundShader, scaleVariableName.c_str()),
                            Scene::Objects[i].m_Scale[0], Scene::Objects[i].m_Scale[1], Scene::Objects[i].m_Scale[2]);
                    }
                    RefreshRequired = true;
                }
            }
            else if (Scene::Objects[i].m_Type == 2)
            {
                ShaderVecParameter(scaleVariableName.c_str(), "Scale", Scene::Objects[i].m_Scale);
            }

            ShaderColorParameter(ArrayElementName("u_objects", i, "material.albedo").c_str(), "Albedo",
                                 Scene::Objects[i].m_Material.m_Albedo);
            ShaderColorParameter(ArrayElementName("u_objects", i, "material.specular").c_str(), "Specular",
                                 Scene::Objects[i].m_Material.m_Specular);
            ShaderColorParameter(ArrayElementName("u_objects", i, "material.emission").c_str(), "Emission",
                                 Scene::Objects[i].m_Material.m_Emission);
            ShaderFloatParameter(ArrayElementName("u_objects", i, "material.emissionStrength").c_str(),
                                 "Emission Strength", &Scene::Objects[i].m_Material.m_EmissionStrength);

            ShaderSliderParameter(ArrayElementName("u_objects", i, "material.roughness").c_str(), "Roughness",
                                  &Scene::Objects[i].m_Material.m_Roughness);
            ShaderSliderParameter(ArrayElementName("u_objects", i, "material.specularHighlight").c_str(), "Highlight",
                                  &Scene::Objects[i].m_Material.m_SpecularHighlight);
            ShaderSliderParameter(ArrayElementName("u_objects", i, "material.specularExponent").c_str(), "Exponent",
                                  &Scene::Objects[i].m_Material.m_SpecularExponent);

            ImGui::NewLine();
        }
        else
        {
            ImGui::Text("Plane");

            ImGui::Text("Visible");
            ImGui::SameLine();
            if (ImGui::Checkbox("##plane_visible", &Scene::PlaneVisible))
            {
                if (Scene::BoundShader)
                {
                    glUniform1ui(glGetUniformLocation(Scene::BoundShader, "u_planeVisible"),
                                 Scene::PlaneVisible);
                }
                RefreshRequired = true;
            }

            ShaderColorParameter("u_planeMaterial.albedo", "Albedo", Scene::PlaneMaterial.m_Albedo);
            ShaderColorParameter("u_planeMaterial.specular", "Specular", Scene::PlaneMaterial.m_Specular);
            ShaderColorParameter("u_planeMaterial.emission", "Emission", Scene::PlaneMaterial.m_Emission);
            ShaderFloatParameter("u_planeMaterial.emissionStrength", "Emission Strength",
                                 &Scene::PlaneMaterial.m_EmissionStrength);

            ShaderSliderParameter("u_planeMaterial.roughness", "Roughness", &Scene::PlaneMaterial.m_Roughness);
            ShaderSliderParameter("u_planeMaterial.specularHighlight", "Highlight",
                                  &Scene::PlaneMaterial.m_SpecularHighlight);
            ShaderSliderParameter("u_planeMaterial.specularExponent", "Exponent",
                                  &Scene::PlaneMaterial.m_SpecularExponent);
        }


        ImGui::PopItemWidth();
        ImGui::End();
    }

    void LightSettingsUi()
    {
        ImGui::Begin("Light settings");

        ImGui::PushItemWidth(-1);
        for (size_t i = 0; i < Scene::Lights.size(); i++)
        {
            std::string indexStr = std::to_string(i);

            ImGui::Text("%s", std::string("Light #").append(indexStr).c_str());
            ImGui::Text("Position");
            ImGui::SameLine();
            if (ImGui::InputFloat3(std::string("##light_pos_").append(indexStr).c_str(), Scene::Lights[i].m_Position))
            {
                if (Scene::BoundShader)
                {
                    glUniform3f(
                        glGetUniformLocation(Scene::BoundShader,
                                             std::string("u_lights[").append(std::to_string(i)).append("].position").
                                                                      c_str()), Scene::Lights[i].m_Position[0],
                        Scene::Lights[i].m_Position[1], Scene::Lights[i].m_Position[2]);
                }
                RefreshRequired = true;
            }

            ImGui::Text("Radius");
            ImGui::SameLine();
            if (ImGui::InputFloat(std::string("##light_radius_").append(indexStr).c_str(), &Scene::Lights[i].m_Radius))
            {
                if (Scene::BoundShader)
                {
                    glUniform1f(
                        glGetUniformLocation(Scene::BoundShader,
                                             std::string("u_lights[").append(std::to_string(i)).append("].radius").
                                                                      c_str()),
                        Scene::Lights[i].m_Radius);
                }
                RefreshRequired = true;
            }

            ImGui::Text("%s", std::string("Light #").append(indexStr).c_str());
            ImGui::Text("Color");
            ImGui::SameLine();
            if (ImGui::ColorPicker3(std::string("##light_color_").append(indexStr).c_str(), Scene::Lights[i].m_Color))
            {
                if (Scene::BoundShader)
                {
                    glUniform3f(
                        glGetUniformLocation(Scene::BoundShader,
                                             std::string("u_lights[").append(std::to_string(i)).append("].color").
                                                                      c_str()),
                        Scene::Lights[i].m_Color[0], Scene::Lights[i].m_Color[1], Scene::Lights[i].m_Color[2]);
                }
                RefreshRequired = true;
            }

            ImGui::Text("Power");
            ImGui::SameLine();
            if (ImGui::InputFloat(std::string("##light_power_").append(indexStr).c_str(), &Scene::Lights[i].m_Power))
            {
                if (Scene::BoundShader)
                {
                    glUniform1f(
                        glGetUniformLocation(Scene::BoundShader,
                                             std::string("u_lights[").append(std::to_string(i)).append("].power").
                                                                      c_str()),
                        Scene::Lights[i].m_Power);
                }
                RefreshRequired = true;
            }

            ImGui::Text("Reach");
            ImGui::SameLine();
            if (ImGui::InputFloat(std::string("##light_reach_").append(indexStr).c_str(), &Scene::Lights[i].m_Reach))
            {
                if (Scene::BoundShader)
                {
                    glUniform1f(
                        glGetUniformLocation(Scene::BoundShader,
                                             std::string("u_lights[").append(std::to_string(i)).append("].reach").
                                                                      c_str()),
                        Scene::Lights[i].m_Reach);
                }
                RefreshRequired = true;
            }

            if (i < 2) ImGui::NewLine();
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

    void AppSettingsUi()
    {
        ImGui::Begin("App");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        ImGui::PushItemWidth(-1);
        ImGui::Text("Shadow resolution");
        ImGui::SameLine();
        if (ImGui::InputInt("##shadowResolution", &Scene::ShadowResolution))
        {
            if (Scene::BoundShader)
            {
                glUniform1i(glGetUniformLocation(Scene::BoundShader, "u_shadowResolution"),
                            Scene::ShadowResolution);
            }
            RefreshRequired = true;
        }

        ImGui::Text("Light bounces");
        ImGui::SameLine();
        if (ImGui::InputInt("##lightBounces", &Scene::LightBounces))
        {
            if (Scene::BoundShader)
            {
                glUniform1i(glGetUniformLocation(Scene::BoundShader, "u_lightBounces"),
                            Scene::LightBounces);
            }
            RefreshRequired = true;
        }

        ImGui::Text("Passes per frame");
        ImGui::SameLine();
        if (ImGui::InputInt("##framePasses", &Scene::FramePasses))
        {
            if (Scene::BoundShader)
            {
                glUniform1i(glGetUniformLocation(Scene::BoundShader, "u_framePasses"),
                            Scene::FramePasses);
            }
            RefreshRequired = true;
        }

        ImGui::Text("Blur");
        ImGui::SameLine();
        if (ImGui::InputFloat("##blur", &Scene::Blur))
        {
            if (Scene::BoundShader)
                glUniform1f(glGetUniformLocation(Scene::BoundShader, "u_blur"), Scene::Blur);
            RefreshRequired = true;
        }

        ImGui::Text("Bloom Radius");
        ImGui::SameLine();
        if (ImGui::InputFloat("##bloomRadius", &Scene::BloomRadius))
        {
            if (Scene::BoundShader)
            {
                glUniform1f(glGetUniformLocation(Scene::BoundShader, "u_bloomRadius"),
                            Scene::BloomRadius);
            }
            RefreshRequired = true;
        }

        ImGui::Text("Bloom Intensity");
        ImGui::SameLine();
        if (ImGui::InputFloat("##bloomIntensity", &Scene::BloomIntensity))
        {
            if (Scene::BoundShader)
            {
                glUniform1f(glGetUniformLocation(Scene::BoundShader, "u_bloomIntensity"),
                            Scene::BloomIntensity);
            }
            RefreshRequired = true;
        }

        if (ImGui::Button("Quit"))
        {
            ShouldQuit = true;
        }
        ImGui::PopItemWidth();

        ImGui::End();
    }

    void SkyboxSettingsUi()
    {
        ImGui::Begin("Skybox");

        ImGui::PushItemWidth(-1);
        ImGui::Text("Intensity");
        ImGui::SameLine();
        if (ImGui::InputFloat("##skyboxStrength", &Scene::SkyboxStrength))
        {
            if (Scene::BoundShader)
            {
                glUniform1f(glGetUniformLocation(Scene::BoundShader, "u_skyboxStrength"),
                            Scene::SkyboxStrength);
            }
            RefreshRequired = true;
        }

        ImGui::Text("Gamma");
        ImGui::SameLine();
        if (ImGui::InputFloat("##skyboxGamma", &Scene::SkyboxGamma))
        {
            if (Scene::BoundShader)
            {
                glUniform1f(glGetUniformLocation(Scene::BoundShader, "u_skyboxGamma"),
                            Scene::SkyboxGamma);
            }
            RefreshRequired = true;
        }

        ImGui::Text("Ceiling");
        ImGui::SameLine();
        if (ImGui::InputFloat("##skyboxCeiling", &Scene::SkyboxCeiling))
        {
            if (Scene::BoundShader)
            {
                glUniform1f(glGetUniformLocation(Scene::BoundShader, "u_skyboxCeiling"),
                            Scene::SkyboxCeiling);
            }
            RefreshRequired = true;
        }

        static char skyboxFilename[64];

        ImGui::Text("Filename");
        ImGui::SameLine();
        ImGui::InputText("##skyboxFileName", skyboxFilename, 64);

        if (ImGui::Button("Load"))
        {
            int sbWidth, sbHeight, sbChannels;
            if (float* skyboxData = LoadImageData(std::string("skyboxes\\").append(skyboxFilename).c_str(), &sbWidth,
                                                  &sbHeight, &sbChannels, 0))
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Scene::SkyboxTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sbWidth, sbHeight, 0, GL_RGB, GL_FLOAT, skyboxData);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glActiveTexture(GL_TEXTURE0);

                FreeImageData(skyboxData);

                skyboxFilename[0] = 0;

                RefreshRequired = true;
            }
            else
            {
                std::cout << "Failed to load skyboxes\\" << skyboxFilename << '\n';
            }
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

    void AnimationRenderingUi()
    {
        ImGui::Begin("Animation Rendering");

        if (ImGui::Button("Set start point"))
        {
            Animation::SetStartPosition(Scene::CameraPosition, Scene::CameraYaw, Scene::CameraPitch);
        }

        if (ImGui::Button("Set end point"))
        {
            Animation::SetEndPosition(Scene::CameraPosition, Scene::CameraYaw, Scene::CameraPitch);
        }

        ImGui::InputInt("animationFramePasses", &Animation::FramePasses);
        ImGui::InputFloat("animationSpeed", &Animation::CameraSpeed);
        ImGui::InputInt("animationFrameRate", &Animation::FrameRate);

        if (ImGui::Button("Render"))
        {
            Animation::CurrentFrame = -1;
            // Set to -1 so the main loop can finish its current iteration before the animation rendering process starts
            Animation::CurrentPass = 0;
            Animation::RecalculateTotalFrameCount();
            Animation::CurrentlyRenderingAnimation = true;
        }

        ImGui::Text("Rendered %d/%d frames.", Animation::CurrentFrame, Animation::TotalFrameCount);

        ImGui::End();
    }

    void cameraSettingsUI()
    {
        ImGui::Begin("Camera");
        ImGui::PushItemWidth(-1);
        ImGui::Text("Position");
        ImGui::SameLine();
        if (ImGui::InputFloat3("##cameraPosition", &Scene::CameraPosition.x))
        {
            RefreshRequired = true;
        }

        ImGui::Text("Orientation (Yaw-Pitch)");
        ImGui::SameLine();
        float cameraOrientation[2] = {Scene::CameraYaw, Scene::CameraPitch};
        if (ImGui::InputFloat2("##cameraOrientation", cameraOrientation))
        {
            Scene::CameraYaw = cameraOrientation[0];
            Scene::CameraPitch = cameraOrientation[1];

            RefreshRequired = true;
        }
        ImGui::PopItemWidth();
        ImGui::End();
    }

    void Render()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ObjectSettingsUi();
        LightSettingsUi();
        AppSettingsUi();
        SkyboxSettingsUi();
        cameraSettingsUI();
        if (AnimationRenderWindowVisible) AnimationRenderingUi();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
