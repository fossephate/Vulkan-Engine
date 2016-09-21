#pragma once

#include <GL/glew.h>

namespace gl {
    void DebugCallbackHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, GLvoid* data) {
        OutputDebugStringA(msg);
        std::cout << "debug call: " << msg << std::endl;
    }

    const std::set<std::string>& getExtensions() {
        static std::once_flag once;
        static std::set<std::string> extensions;
        std::call_once(once, [&] {
            GLint n;
            glGetIntegerv(GL_NUM_EXTENSIONS, &n);
            for (GLint i = 0; i < n; i++) {
                const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
                extensions.insert(extension);
            }
        });
        return extensions;
    }

    namespace nv {
        namespace vk {
            typedef void (GLAPIENTRY * PFN_glWaitVkSemaphoreNV) (GLuint64 vkSemaphore);
            typedef void (GLAPIENTRY * PFN_glSignalVkSemaphoreNV) (GLuint64 vkSemaphore);
            typedef void (GLAPIENTRY * PFN_glSignalVkFenceNV) (GLuint64 vkFence);
            typedef void (GLAPIENTRY * PFN_glDrawVkImageNV) (GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);

            PFN_glDrawVkImageNV __glDrawVkImageNV = nullptr;
            PFN_glWaitVkSemaphoreNV __glWaitVkSemaphoreNV = nullptr;
            PFN_glSignalVkSemaphoreNV __glSignalVkSemaphoreNV = nullptr;

            void init() {
                // Ensure the extension is available
                if (!getExtensions().count("GL_NV_draw_vulkan_image")) {
                    throw std::runtime_error("GL_NV_draw_vulkan_image not supported");
                }

                __glDrawVkImageNV = (PFN_glDrawVkImageNV)wglGetProcAddress("glDrawVkImageNV");
                __glWaitVkSemaphoreNV = (PFN_glWaitVkSemaphoreNV)wglGetProcAddress("glWaitVkSemaphoreNV");
                __glSignalVkSemaphoreNV = (PFN_glSignalVkSemaphoreNV)wglGetProcAddress("glSignalVkSemaphoreNV");
                if (nullptr == __glDrawVkImageNV || nullptr == __glWaitVkSemaphoreNV || nullptr == __glSignalVkSemaphoreNV) {
                    throw std::runtime_error("Could not load required extension");
                }
            }
            void WaitSemaphore(const ::vk::Semaphore& semaphore) {
                __glWaitVkSemaphoreNV((GLuint64)(VkSemaphore)semaphore);
            }
            void SignalSemaphore(const ::vk::Semaphore& semaphore) {
                __glSignalVkSemaphoreNV((GLuint64)(VkSemaphore)semaphore);
            }
            void DrawVkImage(const ::vk::Image& image, GLuint sampler, const vec2& origin, const vec2& size, float z = 0, const vec2& tex1 = vec2(0, 1), const vec2& tex2 = vec2(1, 0)) {
                __glDrawVkImageNV((GLuint64)(VkImage)(image), 0, origin.x, origin.y, size.x, size.y, z, tex1.x, tex1.y, tex2.x, tex2.y);
            }
        }
    }
}
