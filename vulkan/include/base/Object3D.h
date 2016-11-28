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

class Object3D {
public:
	struct {
		glm::quat orientation = glm::quat();
		glm::quat &rotation = orientation;
		glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	} transform;

	// set refs for convenience
	glm::quat &orientation = transform.orientation;
	glm::quat &rotation = orientation;

	glm::vec3 &translation = transform.translation;
	glm::vec3 &scale = transform.scale;


	glm::mat4 transfMatrix;

	//struct {
	//	glm::mat4 transform;
	//} matrices;









	/* translate */
	void translateWorld(glm::vec3 delta);
	void translateLocal(glm::vec3 delta);
	void setTranslation(glm::vec3 translation);


	/* rotate */
	void setRotation(glm::quat rotation);

	/* Rotate around world axes */
	void rotateWorld(glm::vec3 delta);

	/* Convienience functions */
	void rotateWorldX(float r);
	void rotateWorldY(float r);
	void rotateWorldZ(float r);

	/* Rotate around local axes */
	void rotateLocal(glm::vec3 delta);


	void updateTransform();


};