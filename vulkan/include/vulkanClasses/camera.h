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


namespace vkx {

	class Camera : public Object3D {
		private:

		public:
			enum CameraType { follow, firstperson };
			CameraType type = CameraType::firstperson;


			bool isFirstPerson = true;

			const float MAX_PITCH{ (float)M_PI_2 * 0.95f };
			
			float fov = 60.0f;
			float aspect;
			float znear;
			float zfar;
			

			struct {
				uint32_t index = 0;
				glm::vec3 point = glm::vec3(0.0f, 0.0f, 0.0f);
				glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);
			} followOpts;

			struct {
				float distance = 5.0f;
				float phi = 0.0f;
				float theta = 0.0f;
			} sphericalCoords;

			float rotationSpeed = 1.0f;
			float movementSpeed = 0.05f;

	
			// todo: fix this:
			struct {
				glm::mat4 view;
				glm::mat4 transform;
				glm::mat4 projection;
			} matrices;


			Camera();

			void updateViewMatrix();


			void setProjection(float fov, float aspect, float znear, float zfar);


			void setAspectRatio(float aspect);

	};
}