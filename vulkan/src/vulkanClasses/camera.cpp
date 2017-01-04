


#include "camera.h"
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
		if (glm::length2(rotationAxis) < 0.01) { // bad luck, they were parallel, try again!
			rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
		}

		rotationAxis = normalize(rotationAxis);
		return glm::angleAxis(glm::radians(180.0f), rotationAxis);
	}

	rotationAxis = glm::cross(start, dest);

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

	//glm::vec3 dir = glm::vec3(0.0f, 0.0f, 1.0f) * this->transform.rotation;
	glm::vec3 dir = this->transform.euler;
	glm::vec3 &euler = this->transform.euler;

	// Constrain pitch to ~-PI/2 to ~PI/2
	//if (abs(euler.x) > MAX_PITCH) {
	//	euler.x = std::max(std::min(euler.x, MAX_PITCH), -MAX_PITCH);
	//}
	//while (abs(euler.z) > M_PI) {
	//	euler.z += 2.0f * (float)((euler.z > 0) ? -M_PI : M_PI);
	//}

	//if (abs(euler.x) > MAX_PITCH) {
	//	euler.x = std::max(std::min(euler.x, MAX_PITCH), -MAX_PITCH);
	//}
	//while (abs(euler.z) > M_PI) {
	//	euler.z += 2.0f * (float)((euler.z > 0) ? -M_PI : M_PI);
	//}





	// rotate around up axis and then right axis
	glm::quat rot = glm::angleAxis(dir.x, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(dir.z, glm::vec3(0.0f, 1.0f, 0.0f));
	//glm::quat rot = glm::angleAxis(dir.x, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(dir.z, glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(rot));

	//glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(this->orientation));

	glm::mat4 translationMatrix = glm::translate(glm::mat4(), this->transform.translation);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(), this->transform.scale);

	
	this->matrices.transform = (translationMatrix) * (rotationMatrix) /** (scaleMatrix)*/;

	//this->matrices.view = (rotationMatrix) * (glm::inverse(translationMatrix));
	this->matrices.view = glm::inverse(this->matrices.transform);

	glm::vec3 dir2 = this->orientation*glm::vec3(0.0, 0.0, -1.0);

	this->matrices.view = glm::lookAt(
		this->transform.translation, // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 0, 1)  // Head is up (set to 0,-1,0 to look upside-down)
	);

}


Camera::Camera() {
	glm::vec2 size = glm::vec2(1280, 720);// remove this

	this->matrices.projection = glm::perspectiveRH(glm::radians(60.0f), (float)size.x / (float)size.y, 0.0001f, 256.0f);
	/*
	// //not// fixed by GLM_FORCE_DEPTH_ZERO_TO_ONE
	https://vulkan-tutorial.com/Uniform_buffers
	ubo.proj[1][1] *= -1;
	GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
	The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
	If you don't do this, then the image will be rendered upside down.
	*/

	//this->matrices.projection = glm::scale(this->matrices.projection, glm::vec3(1.0f, -1.0f, 1.0f));
	this->matrices.projection[1][1] *= -1;// faster
	
	//this->matrices.projection = glm::mat4_cast(glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f))) * this->matrices.projection;
	//this->matrices.projection = glm::rotate(this->matrices.projection, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	//this->matrices.projection = glm::scale(this->matrices.projection, glm::vec3(-1.0f, 1.0f, -1.0f));





	
}



void Camera::setProjection(float fov, float aspect, float znear, float zfar) {
	this->fov = fov;
	this->znear = znear;
	this->zfar = zfar;

	this->matrices.projection = glm::perspectiveRH(glm::radians(fov), aspect, znear, zfar);
}

void Camera::setAspectRatio(float aspect) {
	this->matrices.projection = glm::perspectiveRH(glm::radians(fov), aspect, znear, zfar);
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





void Camera::update(float deltaTime) {
	if (type == CameraType::firstperson) {
	}
}
