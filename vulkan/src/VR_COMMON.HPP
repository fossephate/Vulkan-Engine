#include "common.hpp"
#include "vulkanShapes.hpp"
//#include "vulkanGL.hpp"

#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iomanip>
#include <list>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <thread>
#include <vector>


#ifdef GLEW_STATIC
#include <GL/glew.h>
#endif

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include "vulkanandroid.h"
#endif

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

using glm::ivec2;
using glm::uvec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using glm::quat;

class Rotations {
public:
	static const quat IDENTITY;
	static const quat Y_180;

	//  Helper function returns the positive angle (in radians) between two 3D vectors
	static inline float angleBetween(const glm::vec3& v1, const glm::vec3& v2) {
		return acosf((glm::dot(v1, v2)) / (glm::length(v1) * glm::length(v2)));
	}
};

class Vectors {
public:
	static const vec3 UNIT_X;
	static const vec3 UNIT_Y;
	static const vec3 UNIT_Z;
	static const vec3 UNIT_NEG_X;
	static const vec3 UNIT_NEG_Y;
	static const vec3 UNIT_NEG_Z;
	static const vec3 UNIT_XY;
	static const vec3 UNIT_XZ;
	static const vec3 UNIT_YZ;
	static const vec3 UNIT_XYZ;
	static const vec3 MAX;
	static const vec3 MIN;
	static const vec3 ZERO;
	static const vec3 ONE;
	static const vec3 TWO;
	static const vec3 HALF;
	static const vec3& RIGHT;
	static const vec3& UP;
	static const vec3& FRONT;
	static const vec3 ZERO4;
};


// Image loading 
#include <gli/gli.hpp>

// Vulkan!
#include <vulkan/vulkan.hpp>


class VrExampleBase {
public:
    vkx::Context vulkanContext;
    vkx::ShapesRenderer vulkanRenderer;
    GLFWwindow* window{ nullptr };
    float fpsTimer{ 0 };
    float lastFPS{ 0 };
    uint32_t frameCounter{ 0 };
    glm::uvec2 size{ 1280, 720 };
    glm::uvec2 renderTargetSize;
    std::array<glm::mat4, 2> eyeViews;
    std::array<glm::mat4, 2> eyeProjections;
    GLuint _fbo{ 0 };
    GLuint _colorBuffer{ 0 };


    VrExampleBase() : vulkanRenderer{ vulkanContext, true } {
        glfwInit();
    }

    ~VrExampleBase() {
        if (nullptr != window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    virtual void setupVrFramebuffer() {
        // Set up the framebuffer object
        glCreateFramebuffers(1, &_fbo);
        glCreateTextures(GL_TEXTURE_2D, 1, &_colorBuffer);
        glTextureStorage2D(_colorBuffer, 1, GL_RGBA8, renderTargetSize.x, renderTargetSize.y);
        glTextureParameteri(_colorBuffer, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_colorBuffer, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(_colorBuffer, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_colorBuffer, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorBuffer, 0);
    };

    virtual void bindVrFramebuffer() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    }

    virtual void unbindVrFramebuffer() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    virtual void submitVrFrame() = 0;
    virtual void renderMirror() = 0;


    void render() {
        glfwMakeContextCurrent(window);
        // Tell the 
        gl::nv::vk::SignalSemaphore(vulkanRenderer.semaphores.renderStart);
        glFlush();
        vulkanRenderer.render();

        bindVrFramebuffer();
        gl::nv::vk::WaitSemaphore(vulkanRenderer.semaphores.renderComplete);
        gl::nv::vk::DrawVkImage(vulkanRenderer.framebuffer.colors[0].image, 0, vec2(0), renderTargetSize, 0, glm::vec2(0), glm::vec2(1));
        unbindVrFramebuffer();
        submitVrFrame();
        renderMirror();
        glfwSwapBuffers(window);
    }

    virtual void prepare() {
        // Make the on screen window 1/4 the resolution of the render target
        size = renderTargetSize;
        size /= 4;
        vulkanContext.createContext(false);
        vulkanRenderer.framebufferSize = renderTargetSize;
        vulkanRenderer.prepare();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_DEPTH_BITS, 16);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
        window = glfwCreateWindow(size.x, size.y, "glfw", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Unable to create rendering window");
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(0);
        glewExperimental = true;
        glewInit();
        glGetError();
        gl::nv::vk::init();
        setupVrFramebuffer();
    }

    virtual void update(float delta) {
        vulkanRenderer.update(delta, eyeProjections, eyeViews);
    }

    virtual void run() final {
        prepare();
        auto tStart = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            auto tEnd = std::chrono::high_resolution_clock::now();
            auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
            glfwPollEvents();
            update((float)tDiff / 1000.0f);
            render();
            ++frameCounter;
            fpsTimer += (float)tDiff;
            if (fpsTimer > 1000.0f) {
                std::string windowTitle = getWindowTitle();
                glfwSetWindowTitle(window, windowTitle.c_str());
                lastFPS = (float)frameCounter;
                fpsTimer = 0.0f;
                frameCounter = 0;
            }
            tStart = tEnd;
        }
    }

    virtual std::string getWindowTitle() {
        std::string device(vulkanContext.deviceProperties.deviceName);
        return "OpenGL Interop - " + device + " - " + std::to_string(frameCounter) + " fps";
    }
};
