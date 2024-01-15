#pragma once

#include <random>

#include "scene.h"

inline void PlaceRandomSpheres()
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 1000); // define the range

    for (int i = 0; i < 64; i++)
    {
        float radius = static_cast<float>(distr(gen)) / 1000.0f;
        float x = static_cast<float>(distr(gen)) / 1000.0F - 0.5f;
        float y = static_cast<float>(distr(gen)) / 1000.0F - 0.5f;
        float z = static_cast<float>(distr(gen)) / 1000.0F - 0.5f;
        const float length = x * x + y * y + z * z;
        x = x / length * 5.0f;
        y = y / length * 5.0f;
        z = z / length * 5.0f;

        bool collision = false;
        for (int j = 0; j < i; j++)
        {
            if (const float dist = glm::distance(glm::vec3(x, y, z),
                                                 glm::vec3(Scene::Objects[j].m_Position[0],
                                                           Scene::Objects[j].m_Position[1],
                                                           Scene::Objects[j].m_Position[2])); dist < radius +
                Scene::Objects[j].
                m_Scale[0])
            {
                collision = true;
                break;
            }
        }

        if (collision)
        {
            i--;
        }
        else
        {
            Scene::Objects.push_back(Scene::Object(1, {x, y, z}, {radius, radius, radius}, Scene::Material(
                                                       {
                                                           static_cast<float>(distr(gen)) / 1000.0f,
                                                           static_cast<float>(distr(gen)) / 1000.0f,
                                                           static_cast<float>(distr(gen)) / 1000.0f
                                                       },
                                                       {
                                                           static_cast<float>(distr(gen)) / 1000.0f,
                                                           static_cast<float>(distr(gen)) / 1000.0f,
                                                           static_cast<float>(distr(gen)) / 1000.0f
                                                       }, {0, 0, 0},
                                                       0.0f, static_cast<float>(distr(gen)) / 1000.0f, 0.0f, 0.0f)));
        }
    }

    Scene::Objects.push_back(Scene::Object(1, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                                           Scene::Material({1.0f, 1.0f, 1.0f})));

    Scene::Lights.push_back(Scene::PointLight({0.0f, 5.0f, 0.0f}, 0.5f, {1.0f, 1.0f, 1.0f}, 1.0f, 100.0f));

    Scene::PlaneVisible = false;
}

inline void PlaceMirrorSpheres()
{
    for (int i = -4; i <= 3; i++)
    {
        for (int j = -4; j <= 3; j++)
        {
            Scene::Objects.push_back(Scene::Object(1, {static_cast<float>(i), 1.0f, static_cast<float>(j)},
                                                   {0.5f, 0.5f, 0.5f},
                                                   Scene::Material({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                                                                   {0.0f, 0.0f, 0.0f},
                                                                   0.0f, 0.2f,
                                                                   0.0f, 0.0f)));
        }
    }

    Scene::PlaneMaterial = Scene::Material({1.0f, 1.0f, 1.0f}, {0.75f, 0.75f, 0.75f}, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f,
                                           0.0f, 0.0f);

    Scene::Lights.push_back(Scene::PointLight({0.0f, 5.0f, 0.0f}, 0.5f, {1.0f, 1.0f, 1.0f}, 1.0f, 100.0f));
}

inline void PlaceBasicScene()
{
    Scene::Objects.push_back(Scene::Object(1, {0.0f, 0.5f, 0.0f}, {0.5f, 0.5f, 0.5f},
                                           Scene::Material({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
                                                           0.0f, 1.0f,
                                                           0.0f,
                                                           0.0f)));

    Scene::PlaneMaterial = Scene::Material({1.0f, 1.0f, 1.0f}, {0.75f, 0.75f, 0.75f}, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f,
                                           0.0f, 0.0f);

    Scene::Lights.push_back(Scene::PointLight({0.0f, 5.0f, 0.0f}, 0.5f, {1.0f, 1.0f, 1.0f}, 1.0f, 100.0f));
}
