//
//  Created by Bradley Austin Davis on 2016/03/19
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <vector>
#include <algorithm>
#include <vulkan/vulkan.hpp>

#include "vulkanContext.h"

namespace vkx {
	struct Framebuffer {
		using Attachment = CreateImageResult;
		vk::Device device;
		vk::Framebuffer framebuffer;
		Attachment depth;
		std::vector<Attachment> colors;

		void destroy();

		// Prepare a new framebuffer for offscreen rendering
		// The contents of this framebuffer are then
		// blitted to our render target
		void create(const vkx::Context& context, const glm::uvec2& size, const std::vector<vk::Format>& colorFormats, vk::Format depthFormat, const vk::RenderPass& renderPass, vk::ImageUsageFlags colorUsage = vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlags depthUsage = vk::ImageUsageFlags());
	};
}
