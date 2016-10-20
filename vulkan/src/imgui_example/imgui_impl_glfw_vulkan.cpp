// ImGui GLFW binding with Vulkan + shaders
// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 5 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXX_CreateFontsTexture(), ImGui_ImplXXXX_NewFrame(), ImGui_ImplXXXX_Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui\imgui.h>
#include <vulkan\vulkan.hpp>

// GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "imgui_impl_glfw_vulkan.h"

// GLFW Data
static GLFWwindow*  g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;

// Vulkan Data
static vk::AllocationCallbacks* g_Allocator = NULL;
static vk::PhysicalDevice       g_Gpu = VK_NULL_HANDLE;
static vk::Device               g_Device = VK_NULL_HANDLE;
static vk::RenderPass           g_RenderPass = VK_NULL_HANDLE;
static vk::PipelineCache        g_PipelineCache = VK_NULL_HANDLE;
static vk::DescriptorPool       g_DescriptorPool = VK_NULL_HANDLE;
static void (*g_CheckVkResult)(vk::Result err) = NULL;

static vk::CommandBuffer        g_CommandBuffer = VK_NULL_HANDLE;
static size_t                 g_BufferMemoryAlignment = 256;
static vk::PipelineCreateFlags  g_PipelineCreateFlags;// = 0;
static int                    g_FrameIndex = 0;

static vk::DescriptorSetLayout  g_DescriptorSetLayout = VK_NULL_HANDLE;
static vk::PipelineLayout       g_PipelineLayout = VK_NULL_HANDLE;
static vk::DescriptorSet        g_DescriptorSet = VK_NULL_HANDLE;
static vk::Pipeline             g_Pipeline = VK_NULL_HANDLE;

static vk::Sampler              g_FontSampler = VK_NULL_HANDLE;
static vk::DeviceMemory         g_FontMemory = VK_NULL_HANDLE;
static vk::Image                g_FontImage = VK_NULL_HANDLE;
static vk::ImageView            g_FontView = VK_NULL_HANDLE;

static vk::DeviceMemory         g_VertexBufferMemory[IMGUI_VK_QUEUED_FRAMES] = {};
static vk::DeviceMemory         g_IndexBufferMemory[IMGUI_VK_QUEUED_FRAMES] = {};
static size_t                 g_VertexBufferSize[IMGUI_VK_QUEUED_FRAMES] = {};
static size_t                 g_IndexBufferSize[IMGUI_VK_QUEUED_FRAMES] = {};
static vk::Buffer               g_VertexBuffer[IMGUI_VK_QUEUED_FRAMES] = {};
static vk::Buffer               g_IndexBuffer[IMGUI_VK_QUEUED_FRAMES] = {};

static vk::DeviceMemory         g_UploadBufferMemory = VK_NULL_HANDLE;
static vk::Buffer               g_UploadBuffer = VK_NULL_HANDLE;

static unsigned char __glsl_shader_vert_spv[] = 
{
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
  0x6c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x11, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64,
  0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0a, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x1f, 0x16, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x41, 0x14, 0x00, 0x00,
  0x6a, 0x16, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00, 0x80, 0x14, 0x00, 0x00,
  0x47, 0x00, 0x03, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x41, 0x14, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x6a, 0x16, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0xb1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0xb1, 0x02, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x48, 0x00, 0x05, 0x00, 0xb1, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0xb1, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0xb1, 0x02, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x80, 0x14, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0x06, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x06, 0x04, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x03, 0x00, 0x06, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0xfa, 0x16, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x03, 0x00, 0x02, 0x05, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x16, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
  0x1a, 0x04, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1a, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00,
  0x47, 0x11, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x9a, 0x02, 0x00, 0x00, 0x41, 0x14, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x9b, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x0e, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x90, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00, 0x6a, 0x16, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x91, 0x02, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0d, 0x0a, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x7f, 0x02, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x0d, 0x0a, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00,
  0xb1, 0x02, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x7f, 0x02, 0x00, 0x00, 0x7f, 0x02, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x2e, 0x05, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xb1, 0x02, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x2e, 0x05, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00,
  0x80, 0x14, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
  0x06, 0x04, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x83, 0x06, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x06, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x83, 0x06, 0x00, 0x00,
  0xfa, 0x16, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x92, 0x02, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x36, 0x00, 0x05, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x1f, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x05, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x6b, 0x60, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x71, 0x4e, 0x00, 0x00,
  0x41, 0x14, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x9b, 0x02, 0x00, 0x00,
  0xaa, 0x26, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x3e, 0x00, 0x03, 0x00, 0xaa, 0x26, 0x00, 0x00, 0x71, 0x4e, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xda, 0x35, 0x00, 0x00,
  0x6a, 0x16, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x91, 0x02, 0x00, 0x00,
  0xea, 0x50, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00,
  0x3e, 0x00, 0x03, 0x00, 0xea, 0x50, 0x00, 0x00, 0xda, 0x35, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xc7, 0x35, 0x00, 0x00,
  0x80, 0x14, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x92, 0x02, 0x00, 0x00,
  0xef, 0x56, 0x00, 0x00, 0xfa, 0x16, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xe0, 0x29, 0x00, 0x00,
  0xef, 0x56, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00,
  0xa0, 0x22, 0x00, 0x00, 0xc7, 0x35, 0x00, 0x00, 0xe0, 0x29, 0x00, 0x00,
  0x41, 0x00, 0x05, 0x00, 0x92, 0x02, 0x00, 0x00, 0x42, 0x2c, 0x00, 0x00,
  0xfa, 0x16, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x09, 0x60, 0x00, 0x00, 0x42, 0x2c, 0x00, 0x00,
  0x81, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00,
  0xa0, 0x22, 0x00, 0x00, 0x09, 0x60, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0xa1, 0x41, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x84, 0x36, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x50, 0x00, 0x07, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x54, 0x47, 0x00, 0x00,
  0xa1, 0x41, 0x00, 0x00, 0x84, 0x36, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00,
  0x8a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x9b, 0x02, 0x00, 0x00,
  0x17, 0x2f, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x3e, 0x00, 0x03, 0x00, 0x17, 0x2f, 0x00, 0x00, 0x54, 0x47, 0x00, 0x00,
  0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};
static unsigned int __glsl_shader_vert_spv_len = 1172;

static unsigned char __glsl_shader_frag_spv[] = 
{
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
  0x6c, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
  0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x1f, 0x16, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x7a, 0x0c, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x1f, 0x16, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x7a, 0x0c, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x7a, 0x0c, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x1a, 0x04, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0xec, 0x14, 0x00, 0x00,
  0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0xec, 0x14, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
  0x02, 0x05, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00,
  0x7a, 0x0c, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x04, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x97, 0x06, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x15, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x0b, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x9b, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x19, 0x00, 0x09, 0x00, 0x96, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x00, 0x03, 0x00, 0xfe, 0x01, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x7b, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfe, 0x01, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x7b, 0x04, 0x00, 0x00,
  0xec, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x1f, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00,
  0xf8, 0x00, 0x02, 0x00, 0x6b, 0x5d, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
  0x9b, 0x02, 0x00, 0x00, 0x8d, 0x1b, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00,
  0x0b, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x0b, 0x40, 0x00, 0x00, 0x8d, 0x1b, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0xfe, 0x01, 0x00, 0x00, 0xc0, 0x36, 0x00, 0x00, 0xec, 0x14, 0x00, 0x00,
  0x41, 0x00, 0x05, 0x00, 0x90, 0x02, 0x00, 0x00, 0xc2, 0x43, 0x00, 0x00,
  0x35, 0x16, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x02, 0x4e, 0x00, 0x00, 0xc2, 0x43, 0x00, 0x00,
  0x57, 0x00, 0x05, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xb9, 0x46, 0x00, 0x00,
  0xc0, 0x36, 0x00, 0x00, 0x02, 0x4e, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0xe4, 0x23, 0x00, 0x00, 0x0b, 0x40, 0x00, 0x00,
  0xb9, 0x46, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x7a, 0x0c, 0x00, 0x00,
  0xe4, 0x23, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};
static unsigned int __glsl_shader_frag_spv_len = 660;

static uint32_t ImGui_ImplGlfwVulkan_MemoryType(vk::MemoryPropertyFlags properties, uint32_t type_bits)
{
    vk::PhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(g_Gpu, &prop);
    
	for (uint32_t i = 0; i < prop.memoryTypeCount; i++) {
		if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i)) {
			return i;
		}
	}
    return 0xffffffff; // Unable to find memoryType
}

static void ImGui_ImplGlfwVulkan_VkResult(vk::Result err)
{
	if (g_CheckVkResult) {
		g_CheckVkResult(err);
	}
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
void ImGui_ImplGlfwVulkan_RenderDrawLists(ImDrawData* draw_data)
{
    vk::Result err;
    ImGuiIO& io = ImGui::GetIO();

    // Create the Vertex Buffer:
    size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    if (!g_VertexBuffer[g_FrameIndex] || g_VertexBufferSize[g_FrameIndex] < vertex_size) {

		if (g_VertexBuffer[g_FrameIndex]) {
			g_Device.destroyBuffer(g_VertexBuffer[g_FrameIndex], g_Allocator);
		}
		if (g_VertexBufferMemory[g_FrameIndex]) {
			g_Device.freeMemory(g_VertexBufferMemory[g_FrameIndex], g_Allocator);
		}
        size_t vertex_buffer_size = ((vertex_size-1) / g_BufferMemoryAlignment+1) * g_BufferMemoryAlignment;
        
		vk::BufferCreateInfo buffer_info = {};
        buffer_info.size = vertex_buffer_size;
        buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        buffer_info.sharingMode = vk::SharingMode::eExclusive;
        err = g_Device.createBuffer(&buffer_info, g_Allocator, &g_VertexBuffer[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);
        
		vk::MemoryRequirements req;
		g_Device.getBufferMemoryRequirements(g_VertexBuffer[g_FrameIndex], &req);
        g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment : req.alignment;
        
		vk::MemoryAllocateInfo alloc_info = {};
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits);
        err = g_Device.allocateMemory(&alloc_info, g_Allocator, &g_VertexBufferMemory[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);
        /*err = */g_Device.bindBufferMemory(g_VertexBuffer[g_FrameIndex], g_VertexBufferMemory[g_FrameIndex], 0);
        //ImGui_ImplGlfwVulkan_VkResult(err);
        g_VertexBufferSize[g_FrameIndex] = vertex_buffer_size;
    }

    // Create the Index Buffer:
    size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if (!g_IndexBuffer[g_FrameIndex] || g_IndexBufferSize[g_FrameIndex] < index_size)
    {
		if (g_IndexBuffer[g_FrameIndex]) {
			g_Device.destroyBuffer(g_IndexBuffer[g_FrameIndex], g_Allocator);
		}
		if (g_IndexBufferMemory[g_FrameIndex]) {
			g_Device.freeMemory(g_IndexBufferMemory[g_FrameIndex], g_Allocator);
		}

        size_t index_buffer_size = ((index_size-1) / g_BufferMemoryAlignment+1) * g_BufferMemoryAlignment;
        vk::BufferCreateInfo buffer_info = {};
        buffer_info.size = index_buffer_size;
        buffer_info.usage = vk::BufferUsageFlagBits::eIndexBuffer;
        buffer_info.sharingMode = vk::SharingMode::eExclusive;
        err = g_Device.createBuffer(&buffer_info, g_Allocator, &g_IndexBuffer[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);

        vk::MemoryRequirements req;
		g_Device.getBufferMemoryRequirements(g_IndexBuffer[g_FrameIndex], &req);
        g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment : req.alignment;
        vk::MemoryAllocateInfo alloc_info = {};
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits);
        err = g_Device.allocateMemory(&alloc_info, g_Allocator, &g_IndexBufferMemory[g_FrameIndex]);

        ImGui_ImplGlfwVulkan_VkResult(err);

        /*err = */g_Device.bindBufferMemory(g_IndexBuffer[g_FrameIndex], g_IndexBufferMemory[g_FrameIndex], 0);
        ImGui_ImplGlfwVulkan_VkResult(err);

        g_IndexBufferSize[g_FrameIndex] = index_buffer_size;
    }

    // Upload Vertex and index Data:
    {
        ImDrawVert* vtx_dst;
        ImDrawIdx* idx_dst;
        /*err = */g_Device.mapMemory(g_VertexBufferMemory[g_FrameIndex], 0, vertex_size, 0, (void**)(&vtx_dst));
        //ImGui_ImplGlfwVulkan_VkResult(err);
        /*err = */g_Device.mapMemory(g_IndexBufferMemory[g_FrameIndex], 0, index_size, 0, (void**)(&idx_dst));


        //ImGui_ImplGlfwVulkan_VkResult(err);
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }

		vk::MappedMemoryRange range[2] = {};
        range[0].memory = g_VertexBufferMemory[g_FrameIndex];
        range[0].size = vertex_size;
        range[1].memory = g_IndexBufferMemory[g_FrameIndex];
        range[1].size = index_size;
        err = g_Device.flushMappedMemoryRanges(2, range);
        ImGui_ImplGlfwVulkan_VkResult(err);
		g_Device.unmapMemory(g_VertexBufferMemory[g_FrameIndex]);
		g_Device.unmapMemory(g_IndexBufferMemory[g_FrameIndex]);
    }

    // Bind pipeline and descriptor sets:
    {
		g_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, g_Pipeline);
		vk::DescriptorSet desc_set[1] = {g_DescriptorSet};
		g_CommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, g_PipelineLayout, 0, 1, desc_set, 0, NULL);
    }

    // Bind Vertex And Index Buffer:
    {
		vk::Buffer vertex_buffers[1] = {g_VertexBuffer[g_FrameIndex]};
		vk::DeviceSize vertex_offset[1] = {0};
		g_CommandBuffer.bindVertexBuffers(0, 1, vertex_buffers, vertex_offset);
		g_CommandBuffer.bindIndexBuffer(g_IndexBuffer[g_FrameIndex], 0, vk::IndexType::eUint16);
    }

    // Setup viewport:
    {
		vk::Viewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = ImGui::GetIO().DisplaySize.x;
        viewport.height = ImGui::GetIO().DisplaySize.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
		g_CommandBuffer.setViewport(0, 1, &viewport);
    }

    // Setup scale and translation:
    {
        float scale[2];
        scale[0] = 2.0f/io.DisplaySize.x;
        scale[1] = 2.0f/io.DisplaySize.y;
        float translate[2];
        translate[0] = -1.0f;
        translate[1] = -1.0f;
		g_CommandBuffer.pushConstants(g_PipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 0, sizeof(float) * 2, scale);
		g_CommandBuffer.pushConstants(g_PipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(float) * 2, sizeof(float) * 2, translate);
    }

    // Render the command lists:
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
				vk::Rect2D scissor;
                scissor.offset.x = (int32_t)(pcmd->ClipRect.x);
                scissor.offset.y = (int32_t)(pcmd->ClipRect.y);
                scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // TODO: + 1??????
				g_CommandBuffer.setScissor(0, 1, &scissor);
				g_CommandBuffer.drawIndexed(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

static const char* ImGui_ImplGlfwVulkan_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfwVulkan_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void ImGui_ImplGlfwVulkan_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
	if (action == GLFW_PRESS && button >= 0 && button < 3) {
		g_MousePressed[button] = true;
	}
}

void ImGui_ImplGlfwVulkan_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void ImGui_ImplGlfwVulkan_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void ImGui_ImplGlfwVulkan_CharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

bool ImGui_ImplGlfwVulkan_CreateFontsTexture(vk::CommandBuffer command_buffer)
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width*height*4*sizeof(char);

	vk::Result err;

    // Create the Image:
    {
		vk::ImageCreateInfo info = {};
        info.imageType = vk::ImageType::e2D;
        info.format = vk::Format::eR8G8B8A8Unorm;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = vk::SampleCountFlagBits::e1;
        info.tiling = vk::ImageTiling::eOptimal;
        info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        info.sharingMode = vk::SharingMode::eExclusive;
        info.initialLayout = vk::ImageLayout::eUndefined;
        err = g_Device.createImage(&info, g_Allocator, &g_FontImage);
        ImGui_ImplGlfwVulkan_VkResult(err);
		vk::MemoryRequirements req;
		g_Device.getImageMemoryRequirements(g_FontImage, &req);

		vk::MemoryAllocateInfo alloc_info = {};
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(vk::MemoryPropertyFlagBits::eDeviceLocal, req.memoryTypeBits);
        err = g_Device.allocateMemory(&alloc_info, g_Allocator, &g_FontMemory);
        ImGui_ImplGlfwVulkan_VkResult(err);
        /*err = */g_Device.bindImageMemory(g_FontImage, g_FontMemory, 0);
        //ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Create the Image View:
    {
		vk::Result err;
		vk::ImageViewCreateInfo info = {};
        info.image = g_FontImage;
        info.viewType = vk::ImageViewType::e2D;
        info.format = vk::Format::eR8G8B8A8Unorm;
        info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        err = g_Device.createImageView(&info, g_Allocator, &g_FontView);
        
		ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Update the Descriptor Set:
    {
		vk::DescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = g_FontSampler;
        desc_image[0].imageView = g_FontView;
        desc_image[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet write_desc[1] = {};
        write_desc[0].dstSet = g_DescriptorSet;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        write_desc[0].pImageInfo = desc_image;
		g_Device.updateDescriptorSets(1, write_desc, 0, NULL);
    }

    // Create the Upload Buffer:
    {
		vk::BufferCreateInfo buffer_info = {};
        buffer_info.size = upload_size;
        buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
        buffer_info.sharingMode = vk::SharingMode::eExclusive;
        err = g_Device.createBuffer(&buffer_info, g_Allocator, &g_UploadBuffer);
        ImGui_ImplGlfwVulkan_VkResult(err);
		
		vk::MemoryRequirements req;
		g_Device.getBufferMemoryRequirements(g_UploadBuffer, &req);
        g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment : req.alignment;
		vk::MemoryAllocateInfo alloc_info = {};
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits);
        err = g_Device.allocateMemory(&alloc_info, g_Allocator, &g_UploadBufferMemory);
        
		ImGui_ImplGlfwVulkan_VkResult(err);
        /*err = */g_Device.bindBufferMemory(g_UploadBuffer, g_UploadBufferMemory, 0);
		//ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Upload to Buffer:
    {
        char* map = NULL;
        /*err = */g_Device.mapMemory(g_UploadBufferMemory, 0, upload_size, 0, (void**)(&map));
        //ImGui_ImplGlfwVulkan_VkResult(err);
        memcpy(map, pixels, upload_size);

		vk::MappedMemoryRange range[1] = {};
        range[0].memory = g_UploadBufferMemory;
        range[0].size = upload_size;
        err = g_Device.flushMappedMemoryRanges(1, range);

        ImGui_ImplGlfwVulkan_VkResult(err);
		g_Device.unmapMemory(g_UploadBufferMemory);
    }
    // Copy to Image:
    {
		vk::ImageMemoryBarrier copy_barrier[1] = {};
        copy_barrier[0].dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        copy_barrier[0].oldLayout = vk::ImageLayout::eUndefined;
        copy_barrier[0].newLayout = vk::ImageLayout::eTransferDstOptimal;
        copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].image = g_FontImage;
        copy_barrier[0].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        copy_barrier[0].subresourceRange.levelCount = 1;
        copy_barrier[0].subresourceRange.layerCount = 1;
		command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, 0, 0, NULL, 0, NULL, 1, copy_barrier);

		vk::BufferImageCopy region = {};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
		command_buffer.copyBufferToImage(g_UploadBuffer, g_FontImage, vk::ImageLayout::eTransferDstOptimal, 1, &region);

		vk::ImageMemoryBarrier use_barrier[1] = {};
        use_barrier[0].srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        use_barrier[0].dstAccessMask = vk::AccessFlagBits::eShaderRead;
        use_barrier[0].oldLayout = vk::ImageLayout::eTransferDstOptimal;
        use_barrier[0].newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].image = g_FontImage;
        use_barrier[0].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        use_barrier[0].subresourceRange.levelCount = 1;
        use_barrier[0].subresourceRange.layerCount = 1;
		//command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, 0, 0, NULL, 0, NULL, 1, use_barrier);
		command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, 0, 0, NULL, 0, NULL, 1, use_barrier);
		
	}

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)(VkImage)g_FontImage;

    return true;
}

bool ImGui_ImplGlfwVulkan_CreateDeviceObjects()
{
	vk::Result err;
	vk::ShaderModule vert_module;
	vk::ShaderModule frag_module;

    // Create The Shader Modules:
    {
        vk::ShaderModuleCreateInfo vert_info = {};
        vert_info.codeSize = __glsl_shader_vert_spv_len;
        vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;
        err = g_Device.createShaderModule(&vert_info, g_Allocator, &vert_module);
        ImGui_ImplGlfwVulkan_VkResult(err);

		vk::ShaderModuleCreateInfo frag_info = {};
        frag_info.codeSize = __glsl_shader_frag_spv_len;
        frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;
        err = g_Device.createShaderModule(&frag_info, g_Allocator, &frag_module);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    if (!g_FontSampler)
    {
		vk::SamplerCreateInfo info = {};
        info.magFilter = vk::Filter::eLinear;
        info.minFilter = vk::Filter::eLinear;
		info.mipmapMode = vk::SamplerMipmapMode::eLinear;
        info.addressModeU = vk::SamplerAddressMode::eRepeat;
        info.addressModeV = vk::SamplerAddressMode::eRepeat;
        info.addressModeW = vk::SamplerAddressMode::eRepeat;
        info.minLod = -1000;
        info.maxLod = 1000;
        err = g_Device.createSampler(&info, g_Allocator, &g_FontSampler);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    if (!g_DescriptorSetLayout)
    {
		vk::Sampler sampler[1] = {g_FontSampler};
		vk::DescriptorSetLayoutBinding binding[1] = {};
        binding[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding[0].pImmutableSamplers = sampler;
		vk::DescriptorSetLayoutCreateInfo info = {};
        info.bindingCount = 1;
        info.pBindings = binding;
        err = g_Device.createDescriptorSetLayout(&info, g_Allocator, &g_DescriptorSetLayout);
        
		ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Create Descriptor Set:
    {
		vk::DescriptorSetAllocateInfo alloc_info = {};
        alloc_info.descriptorPool = g_DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &g_DescriptorSetLayout;
        err = g_Device.allocateDescriptorSets(&alloc_info, &g_DescriptorSet);
        
		ImGui_ImplGlfwVulkan_VkResult(err);
    }

    if (!g_PipelineLayout)
    {
		vk::PushConstantRange push_constants[1] = {};
        push_constants[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constants[0].offset = sizeof(float) * 0;
        push_constants[0].size = sizeof(float) * 4;
		vk::DescriptorSetLayout set_layout[1] = {g_DescriptorSetLayout};
		
		vk::PipelineLayoutCreateInfo layout_info = {};
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = set_layout;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = push_constants;
        err = g_Device.createPipelineLayout(&layout_info, g_Allocator, &g_PipelineLayout);
        
		ImGui_ImplGlfwVulkan_VkResult(err);
    }

	vk::PipelineShaderStageCreateInfo stage[2] = {};
    //stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[0].stage = vk::ShaderStageFlagBits::eVertex;
    stage[0].module = vert_module;
    stage[0].pName = "main";
    //stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[1].stage = vk::ShaderStageFlagBits::eFragment;
    stage[1].module = frag_module;
    stage[1].pName = "main";

	vk::VertexInputBindingDescription binding_desc[1] = {};
    binding_desc[0].stride = sizeof(ImDrawVert);
    binding_desc[0].inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription attribute_desc[3] = {};
    attribute_desc[0].location = 0;
    attribute_desc[0].binding = binding_desc[0].binding;
    attribute_desc[0].format = vk::Format::eR32G32Sfloat;
    attribute_desc[0].offset = (size_t)(&((ImDrawVert*)0)->pos);
    attribute_desc[1].location = 1;
    attribute_desc[1].binding = binding_desc[0].binding;
    attribute_desc[1].format = vk::Format::eR32G32Sfloat;
    attribute_desc[1].offset = (size_t)(&((ImDrawVert*)0)->uv);
    attribute_desc[2].location = 2;
    attribute_desc[2].binding = binding_desc[0].binding;
    attribute_desc[2].format = vk::Format::eR8G8B8A8Unorm;
    attribute_desc[2].offset = (size_t)(&((ImDrawVert*)0)->col);

	vk::PipelineVertexInputStateCreateInfo vertex_info = {};
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = binding_desc;
    vertex_info.vertexAttributeDescriptionCount = 3;
    vertex_info.pVertexAttributeDescriptions = attribute_desc;

	vk::PipelineInputAssemblyStateCreateInfo ia_info = {};
    ia_info.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.polygonMode = vk::PolygonMode::eFill;
    raster_info.cullMode = vk::CullModeFlagBits::eNone;
    raster_info.frontFace = vk::FrontFace::eCounterClockwise;
    raster_info.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo ms_info = {};
    ms_info.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState color_attachment[1] = {};
    color_attachment[0].blendEnable = VK_TRUE;
    color_attachment[0].srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    color_attachment[0].dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_attachment[0].colorBlendOp = vk::BlendOp::eAdd;
    color_attachment[0].srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_attachment[0].dstAlphaBlendFactor = vk::BlendFactor::eZero;
    color_attachment[0].alphaBlendOp = vk::BlendOp::eAdd;
    color_attachment[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineDepthStencilStateCreateInfo depth_info = {};

    vk::PipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = color_attachment;

    vk::DynamicState dynamic_states[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

	vk::GraphicsPipelineCreateInfo info = {};
    info.flags = g_PipelineCreateFlags;
    info.stageCount = 2;
    info.pStages = stage;
    info.pVertexInputState = &vertex_info;
    info.pInputAssemblyState = &ia_info;
    info.pViewportState = &viewport_info;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &ms_info;
    info.pDepthStencilState = &depth_info;
    info.pColorBlendState = &blend_info;
    info.pDynamicState = &dynamic_state;
    info.layout = g_PipelineLayout;
    info.renderPass = g_RenderPass;
    err = g_Device.createGraphicsPipelines(g_PipelineCache, 1, &info, g_Allocator, &g_Pipeline);

    ImGui_ImplGlfwVulkan_VkResult(err);

	g_Device.destroyShaderModule(vert_module, g_Allocator);
	g_Device.destroyShaderModule(frag_module, g_Allocator);

    return true;
}

void    ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects()
{
    if (g_UploadBuffer) {
		g_Device.destroyBuffer(g_UploadBuffer, g_Allocator);
        g_UploadBuffer = VK_NULL_HANDLE;
    }
    if (g_UploadBufferMemory) {
		g_Device.freeMemory(g_UploadBufferMemory, g_Allocator);
        g_UploadBufferMemory = VK_NULL_HANDLE;
    }
}

void    ImGui_ImplGlfwVulkan_InvalidateDeviceObjects()
{
    ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();
    for (int i=0; i<IMGUI_VK_QUEUED_FRAMES; i++)
    {
		if (g_VertexBuffer[i]) {
			g_Device.destroyBuffer(g_VertexBuffer[i], g_Allocator);
		}
		if (g_VertexBufferMemory[i]) {
			g_Device.freeMemory(g_VertexBufferMemory[i], g_Allocator);
		}
		if (g_IndexBuffer[i]) {
			g_Device.destroyBuffer(g_IndexBuffer[i], g_Allocator);
		}
		if (g_IndexBufferMemory[i]) {
			g_Device.freeMemory(g_IndexBufferMemory[i], g_Allocator);
		}
    }

	if (g_FontView) {
		g_Device.destroyImageView(g_FontView, g_Allocator);
	}
	if (g_FontImage) {
		g_Device.destroyImage(g_FontImage, g_Allocator);
	}
	if (g_FontMemory) {
		g_Device.freeMemory(g_FontMemory, g_Allocator);
	}
	if (g_FontSampler) {
		g_Device.destroySampler(g_FontSampler, g_Allocator);
	}

	if (g_DescriptorSetLayout) {
		g_Device.destroyDescriptorSetLayout(g_DescriptorSetLayout, g_Allocator);
	}
	if (g_PipelineLayout) {
		g_Device.destroyPipelineLayout(g_PipelineLayout, g_Allocator);
	}
	if (g_Pipeline) {
		g_Device.destroyPipeline(g_Pipeline, g_Allocator);
	}
}

bool    ImGui_ImplGlfwVulkan_Init(GLFWwindow* window, bool install_callbacks, ImGui_ImplGlfwVulkan_Init_Data *init_data)
{
    g_Allocator = init_data->allocator;
    g_Gpu = init_data->gpu;
    g_Device = init_data->device;
    g_RenderPass = init_data->render_pass;
    g_PipelineCache = init_data->pipeline_cache;
    g_DescriptorPool = init_data->descriptor_pool;
    g_CheckVkResult = init_data->check_vk_result;

    g_Window = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImGui_ImplGlfwVulkan_RenderDrawLists;       // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplGlfwVulkan_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfwVulkan_GetClipboardText;
    io.ClipboardUserData = g_Window;
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

    if (install_callbacks)
    {
        glfwSetMouseButtonCallback(window, ImGui_ImplGlfwVulkan_MouseButtonCallback);
        glfwSetScrollCallback(window, ImGui_ImplGlfwVulkan_ScrollCallback);
        glfwSetKeyCallback(window, ImGui_ImplGlfwVulkan_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplGlfwVulkan_CharCallback);
    }

    ImGui_ImplGlfwVulkan_CreateDeviceObjects();

    return true;
}

void ImGui_ImplGlfwVulkan_Shutdown()
{
    ImGui_ImplGlfwVulkan_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplGlfwVulkan_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED)) {
        double mouse_x, mouse_y;
        glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
        io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    } else {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++) {
        io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}
void ImGui_ImplGlfwVulkan_Render(vk::CommandBuffer command_buffer)
{
    g_CommandBuffer = command_buffer;
    ImGui::Render();
    g_CommandBuffer = VK_NULL_HANDLE;
    g_FrameIndex = (g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
}
