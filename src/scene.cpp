#include "scene.h"

#include <iostream>
#include <string>

extern bool RefreshRequired;

namespace Scene
{
    GLuint BoundShader;
    std::vector<Object> Objects;
    std::vector<PointLight> Lights;
    Material PlaneMaterial;

    GLuint SkyboxTexture;

    glm::vec3 CameraPosition(0, 1, 2);
    float CameraYaw = 0.0f, CameraPitch = 0.0f;

    int ShadowResolution = 20;
    int LightBounces = 5;
    int FramePasses = 4;
    float Blur = 0.002f; // Slight blur (les than a pixel) = anti-aliasing
    float BloomRadius = 0.02f;
    float BloomIntensity = 0.5f;
    float SkyboxStrength = 1.0F;
    float SkyboxGamma = 2.2F;
    float SkyboxCeiling = 10.0F;
    bool PlaneVisible = true;

    int SelectedObjectIndex = -1;

    Material::Material() = default;

    Material::Material(const std::initializer_list<float>& albedo) : Material::Material(
        albedo, {0, 0, 0}, {0, 0, 0}, 1.0f, 1.0f, 0.0f, 0.5f)
    {
    }

    Material::Material(const std::initializer_list<float>& albedo, const std::initializer_list<float>& specular,
                       const std::initializer_list<float>& emission, const float emissionStrength,
                       const float roughness,
                       const float specularHighlight, const float specularExponent)
    {
        for (int i = 0; i < 3; i++)
        {
            this->m_Albedo[i] = *(albedo.begin() + i);
            this->m_Specular[i] = *(specular.begin() + i);
            this->m_Emission[i] = *(emission.begin() + i);
        }
        this->m_EmissionStrength = emissionStrength;
        this->m_Roughness = roughness;
        this->m_SpecularHighlight = specularHighlight;
        this->m_SpecularExponent = specularExponent;
    }

    Object::Object() = default;

    Object::Object(const unsigned int type, const std::initializer_list<float>& position,
                   const std::initializer_list<float>& scale, const Material& material)
    {
        this->m_Type = type;
        for (int i = 0; i < 3; i++) this->m_Position[i] = *(position.begin() + i);
        for (int i = 0; i < 3; i++) this->m_Scale[i] = *(scale.begin() + i);
        this->m_Material = material;
    }

    PointLight::PointLight() = default;

    PointLight::PointLight(const std::initializer_list<float>& position, const float radius,
                           const std::initializer_list<float>& color, const float power, const float reach)
    {
        for (int i = 0; i < 3; i++) this->m_Position[i] = *(position.begin() + i);
        this->m_Radius = radius;
        for (int i = 0; i < 3; i++) this->m_Color[i] = *(color.begin() + i);
        this->m_Power = power;
        this->m_Reach = reach;
    }

    void PlaceMirrorSpheres()
    {
    }

    void SendObjectData(const size_t objectIndex)
    {
        const std::string i_str = std::to_string(objectIndex);
        glUniform1ui(
            glGetUniformLocation(BoundShader, std::string("u_objects[").append(i_str).append("].type").c_str()),
            Objects[objectIndex].m_Type);
        glUniform3f(
            glGetUniformLocation(BoundShader, std::string("u_objects[").append(i_str).append("].position").c_str()),
            Objects[objectIndex].m_Position[0], Objects[objectIndex].m_Position[1], Objects[objectIndex].m_Position[2]);
        glUniform3f(
            glGetUniformLocation(BoundShader, std::string("u_objects[").append(i_str).append("].scale").c_str()),
            Objects[objectIndex].m_Scale[0], Objects[objectIndex].m_Scale[1], Objects[objectIndex].m_Scale[2]);
        glUniform3f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.albedo").c_str()),
            Objects[objectIndex].m_Material.m_Albedo[0], Objects[objectIndex].m_Material.m_Albedo[1],
            Objects[objectIndex].m_Material.m_Albedo[2]);
        glUniform3f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.specular").c_str()),
            Objects[objectIndex].m_Material.m_Specular[0], Objects[objectIndex].m_Material.m_Specular[1],
            Objects[objectIndex].m_Material.m_Specular[2]);
        glUniform3f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.emission").c_str()),
            Objects[objectIndex].m_Material.m_Emission[0], Objects[objectIndex].m_Material.m_Emission[1],
            Objects[objectIndex].m_Material.m_Emission[2]);
        glUniform1f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.emissionStrength").c_str()),
            Objects[objectIndex].m_Material.m_EmissionStrength);
        glUniform1f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.roughness").c_str()),
            Objects[objectIndex].m_Material.m_Roughness);
        glUniform1f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.specularHighlight").
                                                           c_str()),
            Objects[objectIndex].m_Material.m_SpecularHighlight);
        glUniform1f(
            glGetUniformLocation(BoundShader,
                                 std::string("u_objects[").append(i_str).append("].material.specularExponent").c_str()),
            Objects[objectIndex].m_Material.m_SpecularExponent);
    }

    void Bind(const GLuint shaderProgram)
    {
        BoundShader = shaderProgram;

        for (size_t i = 0; i < Lights.size(); i++)
        {
            glUniform3f(
                glGetUniformLocation(shaderProgram,
                                     std::string("u_lights[").append(std::to_string(i)).append("].position").c_str()),
                Lights[i].m_Position[0], Lights[i].m_Position[1], Lights[i].m_Position[2]);
            glUniform1f(glGetUniformLocation(shaderProgram,
                                             std::string("u_lights[").append(std::to_string(i)).append("].radius").
                                                                      c_str()), Lights[i].m_Radius);
            glUniform3f(
                glGetUniformLocation(shaderProgram,
                                     std::string("u_lights[").append(std::to_string(i)).append("].color").c_str()),
                Lights[i].m_Color[0], Lights[i].m_Color[1], Lights[i].m_Color[2]);
            glUniform1f(glGetUniformLocation(shaderProgram,
                                             std::string("u_lights[").append(std::to_string(i)).append("].power").
                                                                      c_str()), Lights[i].m_Power);
            glUniform1f(glGetUniformLocation(shaderProgram,
                                             std::string("u_lights[").append(std::to_string(i)).append("].reach").
                                                                      c_str()), Lights[i].m_Reach);
        }

        glUniform3f(glGetUniformLocation(shaderProgram, "u_planeMaterial.albedo"), PlaneMaterial.m_Albedo[0],
                    PlaneMaterial.m_Albedo[1], PlaneMaterial.m_Albedo[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_planeMaterial.specular"), PlaneMaterial.m_Specular[0],
                    PlaneMaterial.m_Specular[1], PlaneMaterial.m_Specular[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_planeMaterial.emission"), PlaneMaterial.m_Emission[0],
                    PlaneMaterial.m_Emission[1], PlaneMaterial.m_Emission[2]);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.emissionStrength"),
                    PlaneMaterial.m_EmissionStrength);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.roughness"), PlaneMaterial.m_Roughness);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.specularHighlight"),
                    PlaneMaterial.m_SpecularHighlight);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.specularExponent"),
                    PlaneMaterial.m_SpecularExponent);

        glUniform1i(glGetUniformLocation(shaderProgram, "u_shadowResolution"), ShadowResolution);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_lightBounces"), LightBounces);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_framePasses"), FramePasses);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_blur"), Blur);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_bloomRadius"), BloomRadius);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_bloomIntensity"), BloomIntensity);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_skyboxStrength"), SkyboxStrength);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_skyboxGamma"), SkyboxGamma);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_skyboxCeiling"), SkyboxCeiling);

        for (size_t i = 0; i < Objects.size(); i++)
        {
            SendObjectData(i);
        }

        glUniform1i(glGetUniformLocation(BoundShader, "u_selectedSphereIndex"), SelectedObjectIndex);
        glUniform1i(glGetUniformLocation(BoundShader, "u_planeVisible"), PlaneVisible);
    }

    void Unbind()
    {
        BoundShader = 0;
    }

    bool SphereIntersection(const glm::vec3 position, const float radius, const glm::vec3 rayOrigin,
                            const glm::vec3 rayDirection,
                            float* hitDistance)
    {
        const float t = glm::dot(position - rayOrigin, rayDirection);
        const glm::vec3 p = rayOrigin + rayDirection * t;

        const float y = glm::length(position - p);
        if (y < radius)
        {
            const float x = sqrt(radius * radius - y * y);
            const float t1 = t - x;
            if (t1 > 0)
            {
                *hitDistance = t1;
                return true;
            }
        }

        return false;
    }

    bool BoxIntersection(glm::vec3 position, glm::vec3 size, glm::vec3 rayOrigin, glm::vec3 rayDirection,
                         float* hitDistance)
    {
        float t1 = -1000000000000.0f;
        float t2 = 1000000000000.0f;

        glm::vec3 boxMin = position - size / 2.0f;
        glm::vec3 boxMax = position + size / 2.0f;

        glm::vec3 t0s = (boxMin - rayOrigin) / rayDirection;
        glm::vec3 t1s = (boxMax - rayOrigin) / rayDirection;

        glm::vec3 tsmaller = min(t0s, t1s);
        glm::vec3 tbigger = max(t0s, t1s);

        t1 = std::max({t1, tsmaller.x, tsmaller.y, tsmaller.z});
        t2 = std::min({t2, tbigger.x, tbigger.y, tbigger.z});

        *hitDistance = t1;

        return t1 >= 0 && t1 <= t2;
    }

    bool PlaneIntersection(const glm::vec3 planeNormal, const glm::vec3 planePoint, const glm::vec3 rayOrigin,
                           const glm::vec3 rayDirection,
                           float* hitDistance)
    {
        if (const float denom = glm::dot(planeNormal, rayDirection); abs(denom) > 0.0001)
        {
            const glm::vec3 d = planePoint - rayOrigin;
            *hitDistance = glm::dot(d, planeNormal) / denom;
            return (*hitDistance >= 0.0001);
        }

        return false;
    }

    void SelectHovered(const float mouseX, const float mouseY, const int screenWidth, const int screenHeight,
                       const glm::vec3 cameraPosition,
                       const glm::mat4& rotationMatrix)
    {
        const float relativeMouseX = mouseX / static_cast<float>(screenWidth);
        const float relativeMouseY = 1.0f - mouseY / static_cast<float>(screenHeight);
        const glm::vec2 centeredUV = (2.0f * glm::vec2(relativeMouseX, relativeMouseY) - glm::vec2(1.0)) * glm::vec2(
            static_cast<float>(screenWidth) / static_cast<float>(screenHeight), 1.0);
        const glm::vec3 rayDir = glm::normalize(glm::vec4(centeredUV, -1.0, 0.0)) * rotationMatrix;

        float minDist = -1.f;
        SelectedObjectIndex = -1;
        for (size_t sphereIndex = 0; sphereIndex < Objects.size(); sphereIndex++)
        {
            if (Objects[sphereIndex].m_Type == 0) continue;

            float dist;
            if (Objects[sphereIndex].m_Type == 1 && SphereIntersection(
                glm::vec3(Objects[sphereIndex].m_Position[0], Objects[sphereIndex].m_Position[1],
                          Objects[sphereIndex].m_Position[2]), Objects[sphereIndex].m_Scale[0], cameraPosition, rayDir,
                &dist))
            {
                if (minDist == -1.f || dist < minDist)
                {
                    minDist = dist;
                    SelectedObjectIndex = static_cast<int>(sphereIndex);
                }
            }
            else if (Objects[sphereIndex].m_Type == 2 && BoxIntersection(
                glm::vec3(Objects[sphereIndex].m_Position[0], Objects[sphereIndex].m_Position[1],
                          Objects[sphereIndex].m_Position[2]),
                glm::vec3(Objects[sphereIndex].m_Scale[0], Objects[sphereIndex].m_Scale[1],
                          Objects[sphereIndex].m_Scale[2]),
                cameraPosition, rayDir, &dist))
            {
                if (minDist == -1.f || dist < minDist)
                {
                    minDist = dist;
                    SelectedObjectIndex = static_cast<int>(sphereIndex);
                }
            }
        }

        if (BoundShader)
        {
            glUniform1i(glGetUniformLocation(BoundShader, "u_selectedSphereIndex"), SelectedObjectIndex);
        }
    }

    void MousePlace(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition,
                    glm::mat4 rotationMatrix)
    {
        float relativeMouseX = mouseX / static_cast<float>(screenWidth);
        float relativeMouseY = 1.0f - mouseY / static_cast<float>(screenHeight);
        glm::vec2 centeredUV = (2.0f * glm::vec2(relativeMouseX, relativeMouseY) - glm::vec2(1.0)) * glm::vec2(
            static_cast<float>(screenWidth) / static_cast<float>(screenHeight), 1.0f);
        glm::vec3 rayDir = glm::normalize(glm::vec4(centeredUV, -1.0, 0.0)) * rotationMatrix;

        // Find plane intersection
        glm::vec3 planeNormal = glm::vec3(0, 1, 0);
        if (float denom = glm::dot(planeNormal, rayDir); abs(denom) > 0.0001)
        {
            glm::vec3 d = -cameraPosition;
            float hitDistance = glm::dot(d, planeNormal) / denom;
            if (hitDistance >= 0.0001)
            {
                glm::vec3 position = cameraPosition + rayDir * hitDistance;
                if (SelectedObjectIndex >= 0)
                {
                    Object selectedObject = Objects[SelectedObjectIndex];
                    Objects.push_back(Scene::Object(selectedObject.m_Type,
                                                    {
                                                        position[0],
                                                        position[1] + static_cast<float>(selectedObject.m_Type) == 1.f
                                                            ? selectedObject.m_Scale[0]
                                                            : (selectedObject.m_Scale[1] / 2.0f),
                                                        position[2]
                                                    }, {
                                                        selectedObject.m_Scale[0], selectedObject.m_Scale[1],
                                                        selectedObject.m_Scale[2]
                                                    }, selectedObject.m_Material));
                }
                else
                {
                    Objects.push_back(Scene::Object(1, {position[0], position[1] + 1.0f, position[2]},
                                                    {1.0f, 1.0f, 1.0f},
                                                    Material({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
                                                             0.0f,
                                                             1.0f, 0.0f, 0.0f)));
                }

                SendObjectData(Objects.size() - 1);
                RefreshRequired = true;
            }
        }
    }
}
