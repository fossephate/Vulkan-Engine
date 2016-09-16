/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "glm/gtc/matrix_transform.hpp"

class Camera
{
private:
	float fov;
	float znear, zfar;

	void updateViewMatrix();

public:
	enum CameraType { lookat, firstperson };
	CameraType type = CameraType::lookat;

	//glm::vec3 rotation = glm::vec3();// remove this
	//glm::quat rotation = glm::quat();
	//glm::vec3 position = glm::vec3();

	float rotationSpeed = 1.0f;
	float movementSpeed = 1.0f;


	struct {
		glm::quat rotation = glm::quat();
		glm::vec3 position = glm::vec3();
		glm::mat4 transfM = glm::mat4();
	} transform;

	glm::quat & rotation = transform.rotation;
	glm::vec3 & position = transform.position;

	struct
	{
		glm::mat4 view;
		glm::mat4 projection;
	} matrices;

	glm::mat4 & transfMatrix = matrices.view;

	struct
	{
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	bool moving();

	void setProjection(float fov, float aspect, float znear, float zfar);


	void updateAspectRatio(float aspect);


	// translate
	void translate(glm::vec3 delta);
	void setTranslation(glm::vec3 translation);

	// rotate
	void rotate(glm::quat delta);
	void setRotation(glm::quat rotation);
	



	void update(float deltaTime);

	// Update camera passing separate axis data (gamepad)
	// Returns true if view or position has been changed
	bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime);

};