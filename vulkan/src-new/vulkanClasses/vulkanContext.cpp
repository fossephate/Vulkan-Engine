#include "vulkanContext.hpp"

using namespace vkx;

#ifdef WIN32
__declspec(thread) VkCommandPool Context::s_cmdPool;
#else
thread_local vk::CommandPool Context::s_cmdPool;
#endif

std::list<std::string> Context::requestedLayers{ { "VK_LAYER_LUNARG_standard_validation" } };
