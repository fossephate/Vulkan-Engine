#pragma once

// bullet physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"
//#include "../CommonInterfaces/CommonRigidBodyBase.h"





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

#include <chrono>






namespace vkx {



	class PhysicsManager {

		public:

			std::chrono::high_resolution_clock::time_point tLastTimeStep = std::chrono::high_resolution_clock::now();

			//collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
			btDefaultCollisionConfiguration* collisionConfiguration = nullptr;

			//use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
			btCollisionDispatcher* dispatcher = nullptr;

			//btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
			btBroadphaseInterface* overlappingPairCache = nullptr;

			//the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
			btSequentialImpulseConstraintSolver* solver = nullptr;

			// dynamics World
			btDiscreteDynamicsWorld* dynamicsWorld = nullptr;

			//keep track of the shapes, we release memory at exit.
			//make sure to re-use collision shapes among rigid bodies whenever possible!
			btAlignedObjectArray<btCollisionShape*> collisionShapes;

			PhysicsManager();


	};
















}