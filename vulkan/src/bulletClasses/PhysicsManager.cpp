

#include "bulletClasses/PhysicsManager.h"



namespace vkx {



	PhysicsManager::PhysicsManager() {

		// init

		//collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		this->collisionConfiguration = new btDefaultCollisionConfiguration();

		//use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		this->dispatcher = new	btCollisionDispatcher(this->collisionConfiguration);

		//btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		this->overlappingPairCache = new btDbvtBroadphase();

		//the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		this->solver = new btSequentialImpulseConstraintSolver;

		this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher, this->overlappingPairCache, this->solver, this->collisionConfiguration);

		dynamicsWorld->setGravity(btVector3(0, 0, -10));

	}





}