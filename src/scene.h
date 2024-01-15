#pragma once

#include <initializer_list>
#include <vector>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Scene {
	struct Material {
		float m_Albedo[3];
		float m_Specular[3];
		float m_Emission[3];
		float m_EmissionStrength;
		float m_Roughness;
		float m_SpecularHighlight;
		float m_SpecularExponent;
	
		Material(const std::initializer_list<float>& albedo);
		Material(const std::initializer_list<float>& albedo, const std::initializer_list<float>& specular, const std::initializer_list<float>& emission, float emissionStrength, float roughness, float specularHighlight, float specularExponent);
		Material();
	};

	struct Object {
		unsigned int m_Type; // Type 0 = none (invisible), Type 1 = sphere, Type 2 = box
		float m_Position[3];
		float m_Scale[3]; // For spheres, only the x value will be used as the radius
		Material m_Material;

		Object(unsigned int type, const std::initializer_list<float>& position, const std::initializer_list<float>& scale, const Material& material);
		Object();
	};

	struct PointLight {
		float m_Position[3];
		float m_Radius;
		float m_Color[3];
		float m_Power;
		float m_Reach; // Only points within this distance of the light will be affected

		PointLight(const std::initializer_list<float>& position, float radius, const std::initializer_list<float>& color, float power, float reach);
		PointLight();
	};

	extern glm::vec3 CameraPosition;
	extern float CameraYaw, CameraPitch;

	extern GLuint BoundShader;
	extern std::vector<Object> Objects;
	extern std::vector<PointLight> Lights;
	extern Material PlaneMaterial;
	extern int ShadowResolution;
	extern int LightBounces;
	extern int FramePasses;
	extern float Blur;
	extern float BloomRadius;
	extern float BloomIntensity;
	extern float SkyboxStrength;
	extern float SkyboxGamma;
	extern float SkyboxCeiling;
	extern int SelectedObjectIndex;
	extern GLuint SkyboxTexture;
	extern bool PlaneVisible;

	void Bind(GLuint shaderProgram);
	void Unbind();
	void SelectHovered(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition, const glm::mat4& rotationMatrix);
	void MousePlace(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition, glm::mat4 rotationMatrix);
}
