


#include "camera.h"

void Camera::updateViewMatrix() {


	glm::mat4 rotationMatrix = glm::toMat4(this->transform.rotation);
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), this->transform.position);


	//glm::mat4 tranfMatrix = translationMatrix * rotationMatrix;

	if (type == CameraType::firstperson) {
		// rotate then translate
		this->matrices.view = rotationMatrix * translationMatrix;
	} else {
		// translate then rotate
		this->matrices.view = translationMatrix * rotationMatrix;
	}
}

bool Camera::moving()
{
	return keys.left || keys.right || keys.up || keys.down;
}

void Camera::setProjection(float fov, float aspect, float znear, float zfar)
{
	this->fov = fov;
	this->znear = znear;
	this->zfar = zfar;

	this->matrices.projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

void Camera::updateAspectRatio(float aspect)
{
	this->matrices.projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

/* TRANSLATION */
inline void Camera::translate(glm::vec3 delta)
{
	glm::translate(this->transfMatrix, delta);// which to do?
	//this->position += delta;
	//updateViewMatrix();
}



inline void Camera::setTranslation(glm::vec3 translation)
{
	this->position = translation;
	updateViewMatrix();
}

/* ROTATION */
inline void Camera::rotate(glm::quat delta)
{
	this->rotation += delta;
	updateViewMatrix();
}

inline void Camera::setRotation(glm::quat rotation)
{
	this->rotation = rotation;
	updateViewMatrix();
}





void Camera::update(float deltaTime)
{
	if (type == CameraType::firstperson)
	{
		if (moving())
		{
			glm::vec3 camFront;
			camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
			camFront.y = sin(glm::radians(rotation.x));
			camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
			camFront = glm::normalize(camFront);

			float moveSpeed = deltaTime * movementSpeed;

			if (keys.up)
				position += camFront * moveSpeed;
			if (keys.down)
				position -= camFront * moveSpeed;
			if (keys.left)
				position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
			if (keys.right)
				position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

			updateViewMatrix();
		}
	}
}

// Update camera passing separate axis data (gamepad)
// Returns true if view or position has been changed

inline bool Camera::updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
{
	bool retVal = false;

	if (type == CameraType::firstperson)
	{
		// Use the common console thumbstick layout		
		// Left = view, right = move

		const float deadZone = 0.0015f;
		const float range = 1.0f - deadZone;

		glm::vec3 camFront;
		camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
		camFront.y = sin(glm::radians(rotation.x));
		camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
		camFront = glm::normalize(camFront);

		float moveSpeed = deltaTime * movementSpeed * 2.0f;
		float rotSpeed = deltaTime * rotationSpeed * 50.0f;

		// Move
		if (fabsf(axisLeft.y) > deadZone)
		{
			float pos = (fabsf(axisLeft.y) - deadZone) / range;
			position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
			retVal = true;
		}
		if (fabsf(axisLeft.x) > deadZone)
		{
			float pos = (fabsf(axisLeft.x) - deadZone) / range;
			position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
			retVal = true;
		}

		// Rotate
		if (fabsf(axisRight.x) > deadZone)
		{
			float pos = (fabsf(axisRight.x) - deadZone) / range;
			rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
			retVal = true;
		}
		if (fabsf(axisRight.y) > deadZone)
		{
			float pos = (fabsf(axisRight.y) - deadZone) / range;
			rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
			retVal = true;
		}
	} else
	{
		// todo: move code from example base class for look-at
	}

	if (retVal)
	{
		updateViewMatrix();
	}

	return retVal;
}
