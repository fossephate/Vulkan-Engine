


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

//Creates a quat that turns U to V
//glm::quat CreateQuatFromTwoVectors(cvec3 U, cvec3 V)
//{
//	cvec3 w = glm::cross(U, V);
//	glm::quat q = glm::quat(glm::dot(U, V), w.x, w.y, w.z);
//	q.w += sqrt(q.x*q.x + q.w*q.w + q.y*q.y + q.z*q.z);
//	return glm::normalize(q);
//}

//glm::vec3 getForwardVector(glm::quat q) {
//
//	glm::vec3 v;
//	v.x = (2 * (q.x * q.z + q.w * q.y));
//	v.y = (2 * (q.y * q.x - q.w * q.x));
//	v.z = (1 - 2 * (q.x * q.x + q.y * q.y));
//
//	return v;
//
//	/*return Vector3(2 * (x * z + w * y),
//		2 * (y * x - w * x),
//		1 - 2 * (x * x + y * y));*/
//}
//
//glm::vec3 getUpVector(glm::quat q) {
//	glm::vec3 v;
//	/*return Vector3(2 * (x * y - w * z),
//		1 - 2 * (x * x + z * z),
//		2 * (y * z + w * x));*/
//}
//
//glm::vec3 getRightVector(glm::quat q) {
//{
//
//	/*return Vector3(1 - 2 * (y * y + z * z),
//		2 * (x * y + w * z),
//		2 * (x * z - w * y));*/
//}







void Camera::updateViewMatrix() {


	glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

	/*tvec3<T, P> const f(normalize(center - eye));
	tvec3<T, P> const s(normalize(cross(f, up)));
	tvec3<T, P> const u(cross(s, f));

	tmat4x4<T, P> Result(1);
	Result[0][0] = s.x;
	Result[1][0] = s.y;
	Result[2][0] = s.z;
	Result[0][1] = u.x;
	Result[1][1] = u.y;
	Result[2][1] = u.z;

	Result[0][2] = -f.x;
	Result[1][2] = -f.y;
	Result[2][2] = -f.z;

	Result[3][0] = -dot(s, eye);
	Result[3][1] = -dot(u, eye);
	Result[3][2] = dot(f, eye);*/


	// define your up vector
	/*glm::vec3 up = glm::vec3(0, 1, 0);
	// rotate around to a given bearing: yaw
	glm::mat4 camera = glm::rotate(glm::mat4(), floathere, up);
	// Define the 'look up' axis, should be orthogonal to the up axis
	glm::vec3 pitchVector = glm::vec3(1, 0, 0);
	// rotate around to the required head tilt: pitch
	camera = glm::rotate(camera, tilt, pitchVector);*/

	glm::vec3 camera_direction = glm::eulerAngles(this->transform.rotation);

	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 cameraRight = glm::vec3(0.0f, 0.0f, 1.0f);


	// y up to z up rotation
	// z is inverted
	//glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//float root22 = std::sqrt(2) / 2.0f;
	//glm::quat rot1 = glm::quat(root22, root22, 0.0f, 0.0f);

	// convert to euler to flip the z axis (// todo: find a way to do this without euler angles)
	// //http://stackoverflow.com/questions/17749241/how-to-negate-glm-quaternion-rotation-on-any-single-axis
	//glm::quat rot3 = glm::quat(glm::eulerAngles(this->transform.rotation)*glm::vec3(-1.0f, -1.0f, -1.0f));

	// rotate some axes into place
	//glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// rotate around x axis 90 degrees
	//glm::quat rot1 = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	// rotate around y axis 180 degrees
	//glm::quat rot2 = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//glm::quat rot2 = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	//glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(glm::normalize(/*rot2 **/ rot1) * this->transform.rotation));

	glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(this->transform.rotation));

	// invert z bc left hand to right hand
	//glm::vec3 invertedZ = (this->transform.translation)*glm::vec3(-1.0f, 1.0f, -1.0f);
	//glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), invertedZ);
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), this->transform.translation);


	//glm::vec3 direction = glm::eulerAngles(glm::conjugate(this->transform.rotation));
	/*glm::vec3 direction = this->transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 desiredUp = glm::vec3(0.0f, 0.0f, 1.0f);

	// Find the rotation between the front of the object (that we assume towards +Z,
	// but this depends on your model) and the desired direction
	glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 1.0f, 0.0f), direction);


	// Recompute desiredUp so that it's perpendicular to the direction
	// You can skip that part if you really want to force desiredUp
	glm::vec3 rightV = glm::cross(direction, desiredUp);
	desiredUp = glm::cross(rightV, direction);

	// Because of the 1rst rotation, the up is probably completely screwed up.
	// Find the rotation between the "up" of the rotated object, and the desired up
	glm::vec3 newUp = rot1 * glm::vec3(0.0f, 1.0f, 0.0f);
	glm::quat rot2 = RotationBetweenVectors(newUp, desiredUp);

	glm::quat targetOrientation = rot2 * rot1; // remember, in reverse order.

	glm::mat4 rotationMatrix = glm::mat4_cast(targetOrientation);*/


	//glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), this->transform.scale);

	//glm::lookAtRH

	//glm::mat4 tranfMatrix = translationMatrix * rotationMatrix;

	//if (type == CameraType::firstperson) {
		// rotate then translate
		//this->matrices.transform = rotationMatrix * translationMatrix;
	//} else {
		// translate then rotate
		//this->matrices.transform = translationMatrix * rotationMatrix * scaleMatrix;
	//}

	// inverse of transformation matrix
	//this->matrices.view = glm::inverse(rotationMatrix) * glm::inverse(translationMatrix);

	this->matrices.view = (rotationMatrix) * (translationMatrix);

	//this->matrices.view = glm::inverse((translationMatrix) * (rotationMatrix));

	//this->matrices.view = glm::inverse(rotationMatrix * translationMatrix);

	//this->matrices.view = this->matrices.transform;

	/*glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

	glm::vec3 cameraForward = glm::vec3(this->rotation * glm::vec4(up, 0.0f) * glm::conjugate(this->rotation));


	glm::vec3 cameraForward2 = glm::rotate(this->rotation, forward);
	glm::vec3 cameraForward3 = glm::vec3(glm::rotate(this->rotation, glm::vec4(forward, 0.0f)));*/

	//glm::vec3 cameraForward4 = glm::normalize(cross(eulerAngles, forward));



	/*this->matrices.view = glm::lookAt(
		this->transform.translation,
		this->transform.translation + camera_direction,
		glm::vec3(0.0f, 0.0f, 1.0f)
	);*/
	

	//glm::angleAxis(this->transform.rotation);






}


Camera::Camera() {
	//glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//this->transform.rotation = rot1;
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

glm::mat4 Camera::getViewMatrix()
{
	this->matrices.view = glm::inverse(this->matrices.transform);
	return this->matrices.view;
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

	/*glm::vec3 camFront;
	camFront.x = -cos(glm::radians(eulerAngles.x)) * sin(glm::radians(eulerAngles.y));
	camFront.y = sin(glm::radians(eulerAngles.x));
	camFront.z = cos(glm::radians(eulerAngles.x)) * cos(glm::radians(eulerAngles.y));
	camFront = glm::normalize(camFront);*/



	glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 forward = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	
	//glm::vec3 cameraUp = forward * this->rotation;

	//glm::vec3 cameraUp = glm::rotate(this->rotation, up);

	//glm::vec3 cameraUp = glm::vec3(glm::vec4(up, 0.0f) * glm::mat4_cast(this->rotation));

	//glm::vec3 cameraUp = glm::vec3(this->rotation * glm::vec4(up, 0.0f) * glm::conjugate(this->rotation));


	//glm::vec3 cameraUp2 = glm::rotate(this->rotation, up);// works // is y up
	//glm::vec3 cameraUp3 = glm::vec3(glm::rotate(this->rotation, glm::vec4(up, 0.0f)));

	//glm::vec3 cameraUp4 = glm::normalize(glm::cross(eulerAngles, up));


	glm::vec3 cameraUp = up * this->rotation;
	glm::vec3 cameraRight = right * this->rotation;
	glm::vec3 cameraForward= forward * this->rotation;

	//glm::quat inv = glm::inverse(this->rotation);

	//glm::vec3 delta2 = glm::vec3(this->rotation * glm::vec4(delta, 0.0f) * glm::conjugate(this->rotation));
	//glm::vec3 delta3 = glm::vec3(inv * glm::vec4(delta, 0.0f) * glm::conjugate(inv));

	//this->translation += delta3;

	//this->translation += cameraUp * delta.z;

	//this->translation += delta * this->rotation;// works // is y up

	this->translation += delta.z * cameraUp;
	this->translation += delta.x * cameraRight;
	this->translation += delta.y * cameraForward;
	//this->translation += delta.x * camera


	// Find the rotation between the front of the object (that we assume towards +Z,
	// but this depends on your model) and the desired direction
	//glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 1.0f, 0.0f), direction);


	//force z up
	//glm::vec3 newUp = rot1 * glm::vec3(0.0f, 0.0f, 1.0f);
	//glm::quat rot2 = RotationBetweenVectors(newUp, glm::vec3(0.0f, 0.0f, 1.0f));

	//this->rotation = rot2 * this->rotation;


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
	
	//glm::quat q = glm::quat(delta, glm::vec3(0, 0, 1));
	
	//glm::quat q = glm::quat(delta);

	//glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	//delta = delta * rot1;

	this->rotation = glm::normalize(glm::quat(delta) * this->rotation);
	////this->rotation = glm::normalize(this->rotation * glm::quat(delta));

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

// Update camera passing separate axis data (gamepad)
// Returns true if view or position has been changed

bool Camera::updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
{
	bool retVal = false;

	/*if (type == CameraType::firstperson)
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
	}*/

	return retVal;
}
