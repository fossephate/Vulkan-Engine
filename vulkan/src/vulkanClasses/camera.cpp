


#include "camera.h"

void Camera::updateViewMatrix() {


	//this->transform.rotation = glm::normalize(this->transform.rotation);

	//glm::mat4 rotationMatrix = glm::toMat4(this->transform.rotation);
	glm::mat4 rotationMatrix = glm::mat4_cast(this->transform.rotation);
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), this->transform.translation);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), this->transform.scale);

	
	//glm::mat4 full = glm::mat4(1.0f);
	//glm::translate(full, this->transform.translation);
	//full *= glm::toMat4(this->transform.rotation);
	//glm::scale(full, this->transform.scale);

	//glm::mat4 tranfMatrix = translationMatrix * rotationMatrix;

	//if (type == CameraType::firstperson) {
		// rotate then translate
		this->transfMatrix = rotationMatrix * translationMatrix * scaleMatrix;
	//} else {
		// translate then rotate
		//this->transfMatrix = translationMatrix * rotationMatrix * scaleMatrix;
	//}
}



void Camera::updateViewMatrixFromVals() {


	glm::mat4 rotationMatrix = glm::toMat4(this->transform.rotation);
	//glm::mat4 rotationMatrix = glm::mat4_cast(this->transform.rotation);
	//glm::mat4 rotationMatrix = glm::mat4(1.0f)*glm::mat4_cast(this->transform.rotation);
	//glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), );

	//glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), this->transform.translation);
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), this->transform.translation);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), this->transform.scale);


	//glm::mat4 full = glm::mat4(1.0f);
	//glm::translate(full, this->transform.translation);
	//full *= glm::toMat4(this->transform.rotation);
	//glm::scale(full, this->transform.scale);

	//glm::mat4 tranfMatrix = translationMatrix * rotationMatrix;

	//if (type == CameraType::firstperson) {
		// rotate then translate
		this->transfMatrix = rotationMatrix * translationMatrix * scaleMatrix;
	//} else {
		// translate then rotate
		//this->transfMatrix = translationMatrix * rotationMatrix * scaleMatrix;
	//}
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

void Camera::setAspectRatio(float aspect)
{
	this->matrices.projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

/* TRANSLATION */
void Camera::translate(glm::vec3 delta)
{
	this->translation += delta;
	//setTranslation(this->position);
	//glm::translate(this->transfMatrix, delta);// which to do?
	//this->position += delta;
	updateViewMatrix();
}


void Camera::strafe(glm::vec3 delta) {


	glm::vec3 eulerAngles = glm::eulerAngles(this->rotation);

	glm::vec3 camFront;
	camFront.x = -cos(glm::radians(eulerAngles.x)) * sin(glm::radians(eulerAngles.y));
	camFront.y = sin(glm::radians(eulerAngles.x));
	camFront.z = cos(glm::radians(eulerAngles.x)) * cos(glm::radians(eulerAngles.y));
	camFront = glm::normalize(camFront);

	//float moveSpeed = deltaTime * movementSpeed;

	//glm::vec3 upVector = glm::vec3(0.0f, 0.0f, 1.0f) * this->rotation;

	glm::quat conjugate = glm::conjugate(this->rotation);

	glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

	//glm::vec3 upVector = glm::normalize(glm::vec3(this->rotation * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) * conjugate));
	//glm::vec3 cameraForward = glm::normalize(glm::vec3(this->rotation * glm::vec4(forward, 0.0f) * conjugate));
	//glm::vec3 rightVector = glm::normalize(glm::vec3(this->rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) * conjugate));
	//glm::vec3 cameraRight = glm::normalize(glm::vec3(this->rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) * conjugate));



	//glm::vec3 cameraForward = this->rotation * glm::vec3(0.0f, 1.0f, 0.0f);
	//glm::vec3 cameraUp = glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 0.0f, 1.0f)));

	//glm::mat4 rotation = glm::mat4_cast(this->rotation);
	//glm::vec3 cameraForward = glm::rotate(this->rotation, glm::vec3(1.0f, 0.0f, 0.0f));
	

	//glm::vec3 horizontal = glm::normalize(glm::cross(eulerAngles, upVector));




	//glm::vec3 upVector = glm::normalize(glm::vec3(this->rotation * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) * conjugate));
	
	glm::vec3 cameraUp = forward * this->rotation;

	//glm::vec3 cameraUp = glm::rotate(this->rotation, up);




	//this->translation += cameraUp * delta.y;

	this->translation += delta * this->rotation;// works // is y up



	//this->rotation

	//this->translation += cameraForward * delta.y;
	//this->translation += rightVector * delta.x;

	//this->translation += cameraForward;
	//glm::quat rot = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::inverse(this->rotation);

	//glm::quat rot = glm::inverse(this->rotation);

	//this->translation += rot * delta;

	//this->translation += cameraForward * delta.y;
	//this->translation += cameraUp * delta.x;

	//this->translation += camFront * delta.y;
	//this->translation += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 0.0f, 1.0f))) * delta.x;
	//this->translation += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 0.0f, 1.0f))) * delta.z;// figure out the rotation
	updateViewMatrix();

}


/*inline */void Camera::setTranslation(glm::vec3 point/*, bool update = false*/)
{
	this->translation = point;
	//if (update) {
		updateViewMatrix();
	//}
}



inline glm::vec3 Camera::getTranslation()
{
	return this->translation;
}






/* ROTATION */
void Camera::rotate(glm::quat delta)
{
	this->rotation *= delta;
	updateViewMatrix();
}

void Camera::rotateWorld(glm::vec3 delta)
{
	//glm::vec3 current = glm::eulerAngles(this->rotation);

	//delta += current;

	//glm::quat q
	


	/*glm::quat q = glm::angleAxis(delta.x, glm::vec3(1.0f, 0.0f, 0.0f));
			q *= glm::angleAxis(delta.y, glm::vec3(0.0f, 1.0f, 0.0f));
			q *= glm::angleAxis(delta.z, glm::vec3(0.0f, 0.0f, 1.0f));*/
	
	glm::quat q = glm::quat(delta);
	
	//glm::quat q = glm::quat(delta);

	this->rotation = q * this->rotation;
	this->rotation = glm::normalize(this->rotation);
	//this->rotation = q2;
	updateViewMatrix();
}

void Camera::rotateWorldX(float r)
{

	rotateWorld(glm::vec3(r, 0, 0));
	//glm::vec3 dir = { 1, 0, 0 };
	//glm::quat q = glm::angleAxis(r, glm::vec3(1, 0, 0));
	//glm::quat q = glm::quat(r, glm::vec3(1, 0, 0));
	//glm::quat q = glm::quat(glm::vec3(r, 0, 0));
	//this->rotation *= q;
	//this->rotation = glm::normalize(this->rotation);
	//updateViewMatrix();
}

void Camera::rotateWorldY(float r)
{
	//glm::vec3 dir = { 0, 1, 0 };
	//glm::quat q = glm::angleAxis(r, glm::vec3(0, 1, 0));
	//glm::quat q = glm::quat(r, glm::vec3(0, 1, 0));
	glm::quat q = glm::quat(glm::vec3(0, r, 0));
	this->rotation *= q;
	this->rotation = glm::normalize(this->rotation);
	updateViewMatrix();
}

void Camera::rotateWorldZ(float r)
{
	//glm::vec3 dir = { 0, 0, 1 };
	//glm::quat q = glm::angleAxis(r, glm::vec3(0, 0, 1));
	//glm::quat q = glm::quat(r, glm::vec3(0, 0, 1));
	glm::quat q = glm::quat(glm::vec3(0, 0, r));
	this->rotation *= q;
	this->rotation = glm::normalize(this->rotation);
	updateViewMatrix();
}



/*inline */void Camera::setRotation(glm::quat rotation)
{
	this->rotation = rotation;
	updateViewMatrix();
}






inline glm::quat Camera::getRotation() {
	return this->rotation;
}


void Camera::decompMatrix() {

	glm::mat4 transformation; // transformation matrix.
	//glm::vec3 scale;
	//glm::quat rotation;
	//glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(transformation, scale, this->rotation, this->translation, skew, perspective);

}





void Camera::update(float deltaTime)
{
	if (type == CameraType::firstperson)
	{
		if (moving())
		{
			//glm::vec3 eulerAngles = glm::eulerAngles(this->rotation);
			//
			//
			//glm::vec3 camFront;
			//camFront.x = -cos(glm::radians(eulerAngles.x)) * sin(glm::radians(eulerAngles.y));
			//camFront.y = sin(glm::radians(eulerAngles.x));
			//camFront.z = cos(glm::radians(eulerAngles.x)) * cos(glm::radians(eulerAngles.y));
			//camFront = glm::normalize(camFront);

			//float moveSpeed = deltaTime * movementSpeed;

			//if (keys.up)
			//	this->translation += camFront * moveSpeed;
			//if (keys.down)
			//	this->translation -= camFront * moveSpeed;
			//if (keys.left)
			//	this->translation -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 0.0f, 1.0f))) * moveSpeed;
			//if (keys.right)
			//	this->translation += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 0.0f, 1.0f))) * moveSpeed;

			updateViewMatrix();
		}
	}
}

// Update camera passing separate axis data (gamepad)
// Returns true if view or position has been changed

bool Camera::updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
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
			this->translation -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
			retVal = true;
		}
		if (fabsf(axisLeft.x) > deadZone)
		{
			float pos = (fabsf(axisLeft.x) - deadZone) / range;
			this->translation += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
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
