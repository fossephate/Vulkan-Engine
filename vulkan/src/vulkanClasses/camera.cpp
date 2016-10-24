


#include "camera.h"


glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest) {
	start = glm::normalize(start);
	dest = normalize(dest);

	float cosTheta = dot(start, dest);
	glm::vec3 rotationAxis;

	if (cosTheta < -1 + 0.001f) {
		// special case when vectors in opposite directions:
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
		if (glm::length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
			rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

		rotationAxis = normalize(rotationAxis);
		return glm::angleAxis(180.0f, rotationAxis);
	}

	rotationAxis = cross(start, dest);

	float s = sqrt((1 + cosTheta) * 2);
	float invs = 1 / s;

	return glm::quat(
		s * 0.5f,
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs
	);
}



glm::quat QLookAt(glm::vec3 direction, glm::vec3 desiredUp) {

	if (glm::length2(direction) < 0.0001f) {
		return glm::quat();
	}

	// Recompute desiredUp so that it's perpendicular to the direction
	// You can skip that part if you really want to force desiredUp
	glm::vec3 right = cross(direction, desiredUp);
	desiredUp = glm::cross(right, direction);

	// Find the rotation between the front of the object (that we assume towards +Z,
	// but this depends on your model) and the desired direction
	glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 0.0f, 1.0f), direction);
	// Because of the 1rst rotation, the up is probably completely screwed up. 
	// Find the rotation between the "up" of the rotated object, and the desired up
	glm::vec3 newUp = rot1 * glm::vec3(0.0f, 1.0f, 0.0f);
	glm::quat rot2 = RotationBetweenVectors(newUp, desiredUp);

	// Apply them
	return rot2 * rot1; // remember, in reverse order.
}







void Camera::updateViewMatrix() {

	// z-up directions
	glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

	glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(this->transform.rotation));
	//glm::mat4 rotationMatrix = glm::mat4();
	//glm::mat4 rotationMatrix = glm::rotate(glm::mat4(), glm::angle(this->transform.rotation), glm::axis(this->transform.rotation));

	//rotationMatrix = rotationMatrix * glm::mat4_cast(glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
	//rotationMatrix = glm::scale(rotationMatrix, glm::vec3(-1.0f, 1.0f, -1.0f));

	glm::mat4 translationMatrix = glm::translate(glm::mat4(), this->transform.translation);

	this->matrices.view = (rotationMatrix) * (translationMatrix);

	//glm::vec3 dir = this->transform.euler;

	//glm::vec3 direction(
	//	cos(dir.x) * sin(dir.z),
	//	cos(dir.x) * cos(dir.z),
	//	sin(dir.x)
	//);

	//this->matrices.view = glm::lookAtRH(
	//	this->transform.translation,
	//	this->transform.translation + direction,//glm::vec3(0.0f, 0.0f, 0.0f),//this->transform.translation + glm::normalize(this->transform.euler),//this->transform.translation + (glm::vec3(1.0f, 0.0f, 0.0f)*this->transform.rotation),
	//	glm::vec3(0.0f, 0.0f, 1.0f)
	//);

	//this->matrices.view = glm::mat4_cast(glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f))) * this->matrices.view;
	//this->matrices.view = glm::scale(this->matrices.view, glm::vec3(1.0f, 1.0f, -1.0f));

}


Camera::Camera() {
	glm::vec2 size = glm::vec2(1280, 720);// remove this

	this->matrices.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.x / (float)size.y, 0.001f, 256.0f);
	
	//this->matrices.projection = this->matrices.projection * glm::mat4_cast(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
	//this->matrices.projection = glm::scale(this->matrices.projection, glm::vec3(-1.0f, 1.0f, -1.0f));
	
}



void Camera::setProjection(float fov, float aspect, float znear, float zfar)
{
	this->fov = fov;
	this->znear = znear;
	this->zfar = zfar;

	this->matrices.projection = glm::perspectiveRH(glm::radians(fov), aspect, znear, zfar);
}

void Camera::setAspectRatio(float aspect)
{
	this->matrices.projection = glm::perspectiveRH(glm::radians(fov), aspect, znear, zfar);
}

/* translation */
void Camera::setTranslation(glm::vec3 point)
{
	this->translation = point;
	updateViewMatrix();
}


void Camera::translateWorld(glm::vec3 delta)
{
	this->translation += delta;
	updateViewMatrix();
}

void Camera::translateLocal(glm::vec3 delta) {

	this->translation += delta * this->rotation;
	updateViewMatrix();

}





/* rotation */

void Camera::setRotation(glm::quat rotation)
{
	this->rotation = rotation;
	updateViewMatrix();
}

void Camera::rotateWorld(glm::vec3 delta)
{
	this->rotation = glm::normalize(glm::quat(delta) * this->rotation);
	//this->transform.euler = glm::normalize(this->transform.euler + delta);// remove
	this->transform.euler += delta;// remove
	updateViewMatrix();
}

void Camera::rotateWorldX(float r)
{
	rotateWorld(glm::vec3(r, 0.0f, 0.0f));
}
void Camera::rotateWorldY(float r)
{
	rotateWorld(glm::vec3(0.0f, r, 0.0f));
}
void Camera::rotateWorldZ(float r)
{
	rotateWorld(glm::vec3(0.0f, 0.0f, r));
}


void Camera::rotateLocal(glm::vec3 delta) {


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
		//if (moving())
		//{
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

			//updateViewMatrix();
		//}
	}
}
