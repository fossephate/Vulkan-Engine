

#include "bulletClasses/PhysicsObject.h"

namespace vkx {






	PhysicsObject::PhysicsObject() {

	}

	PhysicsObject::PhysicsObject(vkx::PhysicsManager *physicsManager, std::shared_ptr<vkx::Object3D> object3D, btRigidBody *rigidBody) {

		this->physicsManager = physicsManager;
		this->object3D = object3D;
		this->rigidBody = rigidBody;
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


	}

}