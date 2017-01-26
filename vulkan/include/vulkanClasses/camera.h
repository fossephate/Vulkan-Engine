/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <algorithm>
#include "Object3D.h"

class Camera : public vkx::Object3D {
	private:
		const float MAX_PITCH{ (float)M_PI_2 * 0.95f };
		float fov;
		float znear, zfar;

		void updateViewMatrix();

	public:
		enum CameraType { lookat, firstperson };
		CameraType type = CameraType::firstperson;

		bool changed = false;

		float rotationSpeed = 1.0f;
		float movementSpeed = 0.05f;
		//float movementSpeed = 5.0f;

		//struct {
		//	glm::quat orientation = glm::quat();
		//	glm::quat &rotation = orientation;
		//	glm::vec3 euler;
		//	glm::vec4 angleAxis;

		//	glm::vec3 translation = glm::vec3();
		//	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

		//
		//	//glm::mat4 transfMatrix = glm::mat4();
		//} transform;

		//// set refs for convenience
		//glm::quat & orientation = transform.orientation;
		//glm::quat &rotation = orientation;

		//glm::vec3 & translation = transform.translation;
		//glm::vec3 & scale = transform.scale;

	
		// todo: fix this:
		struct {
			glm::mat4 view;
			glm::mat4 transform;
			//glm::mat4 & transfMatrix = view;
			glm::mat4 projection;

		} matrices;

		//glm::mat4 & transfMatrix = matrices.transfMatrix;
		glm::mat4 & projection = matrices.projection;


		Camera();


		void setProjection(float fov, float aspect, float znear, float zfar);


		void setAspectRatio(float aspect);


		void update(float deltaTime);

};