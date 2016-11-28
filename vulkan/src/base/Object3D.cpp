#include "Object3D.h";

namespace vkx {

	/* translation */
	void Object3D::setTranslation(glm::vec3 point)
	{
		this->translation = point;
		updateTransform();
	}

	void Object3D::translateWorld(glm::vec3 delta) {
		this->translation += delta;
		updateTransform();
	}

	void Object3D::translateLocal(glm::vec3 delta) {

		this->translation += delta * this->rotation;
		updateTransform();
	}


	/* rotation */

	void Object3D::setRotation(glm::quat orientation)
	{
		this->orientation = orientation;
		updateTransform();
	}



	void Object3D::updateTransform() {


		glm::mat4 rotationMatrix = glm::mat4_cast(this->orientation);

		glm::mat4 translationMatrix = glm::translate(glm::mat4(), this->translation);

		glm::mat4 scaleMatrix = glm::scale(this->scale);



		//this->transfMatrix = rotationMatrix * translationMatrix /** scaleMatrix*/;
		this->transfMatrix = translationMatrix /** rotationMatrix /** scaleMatrix*/;

	}


}