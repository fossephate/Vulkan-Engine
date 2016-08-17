#include "renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

int main() {

	renderer r;
	auto queue = r._queue;
	
	auto device = r._device;


	/* create fence */

	VkFence fence;
	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//fence_create_info.flags = ;
	vkCreateFence(device, &fence_create_info, nullptr, &fence);

	/* end fence creation */



	/* create semaphore */

	VkSemaphore semaphore;
	VkSemaphoreCreateInfo semaphore_create_info;
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore);

	/* end semaphore creation */




	/* create command pool */
	VkCommandPool command_pool{};
	VkCommandPoolCreateInfo pool_create_info{};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex = r._graphics_family_index;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool);

	/* end command pool creation */

	

	/* create command buffer */
	VkCommandBuffer command_buffer[2];// now a list of command buffers
	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.commandBufferCount = 1;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffer);
	/* end command buffer creation */


	/*
	Command buffers begin execution in the order they are submitted,
	the commands within the buffers also begin execution in the order they are submitted. 
	
	That doesn't mean that the work submitted later in the same queue isn't allowed to begin
	execution before the previous command buffers have finished.

	Semaphores make sure command buffer execution won't overlap, where needed.
	*/

	// semaphores make the gpu wait for the cpu
	// fences make the cpu wait for the gpu
	// concurrency is hard


	{
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//begin_info.flags = 

		// record stuff to do
		vkBeginCommandBuffer(command_buffer[0], &begin_info);

		vkCmdPipelineBarrier(command_buffer[0],
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			0, nullptr,
			0, nullptr,
			0, nullptr);


		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = 512;
		viewport.height = 512;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);

		vkEndCommandBuffer(command_buffer[0]);
	}




	{
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;


		// record stuff to do
		vkBeginCommandBuffer(command_buffer[1], &begin_info);

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = 512;
		viewport.height = 512;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer[0], 0, 1, &viewport);



		// stuff here?

		//vkCmdSetViewport(command_buffer, )
		vkEndCommandBuffer(command_buffer[1]);

	}


	{
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer[0];
		submit_info.signalSemaphoreCount = 1;// number of semaphores to signal
		submit_info.pSignalSemaphores = &semaphore;// list of semaphores to signal

		vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	}

	{
		// list of stages of pipeline to wait for semaphore at
		VkPipelineStageFlags flags[]{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };// wait for everything?


		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer[1];
		submit_info.signalSemaphoreCount = 1;// number of semaphores to signal
		submit_info.pSignalSemaphores = &semaphore;// list of semaphores to signal

		submit_info.pWaitDstStageMask = flags;

		vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	}

	// if VK_TRUE, wait for all fences, if VK_FALSE, wait for 1 fence // last param is timeout in nano seconds
	//vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);// where is the list?
	

	


	vkQueueWaitIdle(queue);

	vkDestroyCommandPool(device, command_pool, nullptr);
	vkDestroyFence(device, fence, nullptr);
	vkDestroySemaphore(device, semaphore, nullptr);



	return 0;
}