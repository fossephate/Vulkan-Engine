


#include "camera.h"

namespace vkx {


	Camera::Camera() {
		// todo: remove this
		glm::vec2 defaultSize = glm::vec2(1280, 720);
		this->setProjection(80.0f, (float)defaultSize.x / (float)defaultSize.y, 0.0001f, 256.0f);
	}



	void Camera::updateViewMatrix() {

		// z-up directions
		//glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
		//glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
		//glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);




		// rotate around up axis and then right axis
		//glm::quat rot = glm::angleAxis(dir.x, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(dir.z, glm::vec3(0.0f, 1.0f, 0.0f));
		//glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(rot));
		//glm::mat4 translationMatrix = glm::translate(glm::mat4(), this->transform.translation);
		//glm::mat4 scaleMatrix = glm::scale(glm::mat4(), this->transform.scale);
		//this->matrices.transform = (translationMatrix) * (rotationMatrix) /** (scaleMatrix)*/;

		//this->matrices.view = (rotationMatrix) * (glm::inverse(translationMatrix));

		if (this->isFirstPerson) {

			glm::vec3 dir2 = glm::normalize(this->transform.orientation*glm::vec3(0.0, 0.0, -1.0));

			this->matrices.view = glm::lookAt(
				this->transform.translation, // camera position
				this->transform.translation + dir2, // point to look at
				glm::vec3(0, 0, 1)  // up vector
			);
		} else {

			float theta = this->sphericalCoords.theta;
			float phi = this->sphericalCoords.phi;
			float distance = this->sphericalCoords.distance;

			
			float orbitX = distance*cos(theta);
			float orbitY = distance*sin(theta);
			float orbitZ = distance*sin(phi);
			this->transform.translation = glm::vec3(orbitX, orbitY, orbitZ);

			this->matrices.view = glm::lookAt(
				this->followOpts.point + this->transform.translation, // camera position
				this->followOpts.point, // point to look at
				glm::vec3(0, 0, 1)  // up vector
			);

		}

	}



	void Camera::setProjection(float fov, float aspect, float znear, float zfar) {
		this->fov = fov;
		this->aspect = aspect;
		this->znear = znear;
		this->zfar = zfar;

		this->matrices.projection = glm::perspectiveRH(glm::radians(fov), aspect, znear, zfar);
		/*
		https://vulkan-tutorial.com/Uniform_buffers
		ubo.proj[1][1] *= -1;
		GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
		The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
		If you don't do this, then the image will be rendered upside down.
		*/
		this->matrices.projection[1][1] *= -1;
	}

	void Camera::setAspectRatio(float aspect) {
		this->setProjection(this->fov, aspect, this->znear, this->zfar);
	}

}