

#include "bulletClasses/PhysicsObject.h"

namespace vkx {






	PhysicsObject::PhysicsObject() {

	}

	PhysicsObject::PhysicsObject(vkx::PhysicsManager *physicsManager, std::shared_ptr<vkx::Object3D> object3D/*, btRigidBody *rigidBody*/) {

		this->physicsManager = physicsManager;
		this->object3D = object3D;
		
	}

	void PhysicsObject::addRigidBody(btRigidBody *body) {
		this->rigidBody = body;
		this->physicsManager->dynamicsWorld->addRigidBody(this->rigidBody);
	}

	void PhysicsObject::createRigidBody(btCollisionShape *collisionShape, float mass) {

		//btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(20.), btScalar(20.), btScalar(0.1)));
		// todo: re-use collision shapes from collisionshapes vector
		//if (this->physicsManager->collisionShapes.size() < 1) {
		this->physicsManager->collisionShapes.push_back(collisionShape);
		//}
		//collisionShape = this->physicsManager->collisionShapes[0];


		// set default translation
		btTransform defaultTransform;
		defaultTransform.setIdentity();
		defaultTransform.setOrigin(btVector3(0, 0, 0));

		// set mass
		btScalar rbMass(mass);

		btVector3 localInertia(0, 0, 0);
		// if the mass is not 0, the body is dynamic, calculate local inertia
		if (rbMass != 0.f) {
			collisionShape->calculateLocalInertia(rbMass, localInertia);
		}

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(defaultTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(rbMass, myMotionState, collisionShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		// add rigid body to self and physics manager
		this->addRigidBody(body);



	}

	void PhysicsObject::sync() {

		btTransform trans;
		// not needed here // todo: remove
		//if (this->rigidBody && this->rigidBody->getMotionState()) {
			this->rigidBody->getMotionState()->getWorldTransform(trans);
		//} else {
		//	//trans = p->rigidBody->getWorldTransform();
		//}

		// physics object position
		glm::vec3 pos = glm::vec3(float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
		// physics object rotation
		glm::quat rot = glm::quat(trans.getRotation().getW(), trans.getRotation().getX(), trans.getRotation().getY(), trans.getRotation().getZ());

		// update info at pointer in physics object
		this->object3D->setTranslation(pos);
		this->object3D->setRotation(rot);

	}


	void PhysicsObject::destroy() {
		this->physicsManager->dynamicsWorld->removeRigidBody(this->rigidBody);
	}

}