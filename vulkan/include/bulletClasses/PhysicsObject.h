#pragma once

#include "Object3D.h"
#include "vulkanMesh.h"
#include "vulkanModel.h"
#include "vulkanSkinnedMesh.h"


#include "bulletClasses/PhysicsManager.h"



// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <algorithm>


namespace vkx {



	class PhysicsObject {

		public:

			// pointer to object3D (derived)
			//vkx::Object3D *object3D = nullptr;
			std::shared_ptr<vkx::Object3D> object3D = nullptr;

			// pointer to physics manager
			vkx::PhysicsManager *physicsManager = nullptr;

			// pointer to rigid body
			btRigidBody *rigidBody = nullptr;

			PhysicsObject();

			//PhysicsObject(vkx::PhysicsManager *physicsManager, vkx::Object3D *object3D);
			//PhysicsObject(vkx::PhysicsManager *physicsManager, std::shared_ptr<vkx::Object3D> object3D);
			PhysicsObject(vkx::PhysicsManager *physicsManager, std::shared_ptr<vkx::Object3D> object3D/*, btRigidBody *rigidBody = nullptr*/);

			void addRigidBody(btRigidBody *body);

			void createRigidBody(btCollisionShape *collisionShape, float mass);

			void sync();

			void destroy();


	};
















}