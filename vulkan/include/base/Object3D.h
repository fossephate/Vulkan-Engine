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

namespace vkx {

	class Object3D {
		public:

			struct {
				glm::quat orientation = glm::quat();
				//glm::quat &rotation = orientation;

				glm::vec4 angleAxis;

				// no euler!
				glm::vec3 euler;

				glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
				glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
			} transform;

			// set refs for convenience
			//glm::quat &orientation = transform.orientation;
			//glm::quat &rotation = transform.orientation;

			//glm::vec3 &translation = transform.translation;
			//glm::vec3 &scale = transform.scale;


			glm::mat4 transfMatrix;
			glm::mat4 viewMatrix;



			/* translation */
			void setTranslation(glm::vec3 point) {
				this->transform.translation = point;
				updateTransform();
			}


			/* translate along world axes */
			void translateWorld(glm::vec3 delta) {
				// world space, not screen space
				this->transform.translation += delta;
				updateTransform();
			}

			/* translate along local axes */
			void translateLocal(glm::vec3 delta) {
				// screen space, not world space
				this->transform.translation += this->transform.orientation * delta;// needs work
				updateTransform();
			}




			/* rotation */
			void setRotation(glm::quat orientation) {
				this->transform.orientation = orientation;
				updateTransform();
			}


			void rotateWorld(glm::quat q) {
				
				//this->orientation = glm::normalize(glm::quat(delta) * this->rotation);
				this->transform.orientation = glm::normalize(q*this->transform.orientation);

				updateTransform();
			}

			void rotateWorldX(float angle) {
				glm::vec3 axis(1.0, 0.0, 0.0);
				//this->transform.euler += angle*axis;
				glm::quat q = glm::angleAxis(angle, axis);
				this->rotateWorld(q);
			}
			void rotateWorldY(float angle) {
				glm::vec3 axis(0.0, 1.0, 0.0);
				//this->transform.euler += angle*axis;
				glm::quat q = glm::angleAxis(angle, axis);
				this->rotateWorld(q);
			}
			void rotateWorldZ(float angle) {
				glm::vec3 axis(0.0, 0.0, 1.0);
				//this->transform.euler += angle*axis;
				glm::quat q = glm::angleAxis(angle, axis);
				this->rotateWorld(q);
			}



			void rotateLocal(glm::quat q) {

				//this->orientation = glm::normalize(glm::quat(delta) * this->rotation);
				this->transform.orientation = glm::normalize(this->transform.orientation*q);

				updateTransform();
			}

			void rotateLocalX(float angle) {
				glm::vec3 axis(1.0, 0.0, 0.0);
				glm::quat q = glm::angleAxis(angle, axis);
				this->rotateLocal(q);
			}
			void rotateLocalY(float angle) {
				glm::vec3 axis(0.0, 1.0, 0.0);
				glm::quat q = glm::angleAxis(angle, axis);
				this->rotateLocal(q);
			}
			void rotateLocalZ(float angle) {
				glm::vec3 axis(0.0, 0.0, 1.0);
				glm::quat q = glm::angleAxis(angle, axis);
				this->rotateLocal(q);
			}



			void updateTransform() {


				glm::mat4 rotationMatrix = glm::mat4_cast(this->transform.orientation);

				glm::mat4 translationMatrix = glm::translate(glm::mat4(), this->transform.translation);

				glm::mat4 scaleMatrix = glm::scale(this->transform.scale);



				//this->transfMatrix = rotationMatrix * translationMatrix /** scaleMatrix*/;
				this->transfMatrix = translationMatrix * rotationMatrix /** scaleMatrix*/;

				this->updateViewMatrix();


			}

			virtual void updateViewMatrix() {

			}





	};

}