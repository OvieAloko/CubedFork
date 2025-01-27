#include "Renderer.h"

#include "Walnut/Application.h"
#include "Walnut/Core/Log.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <array>
#include <fstream>

namespace Cubed {

	static uint32_t ImGui_ImplVulkan_MemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
	{
		VkPhysicalDevice physicalDevice = Cubed::GetVulkanInfo()->PhysicalDevice;

		VkPhysicalDeviceMemoryProperties prop;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &prop);
		for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
			if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
				return i;
		return 0xFFFFFFFF; // Unable to find memoryType
	}

	void Renderer::Init()
	{
		InitBuffers();
		InitPipeline();
	}

	void Renderer::Shutdown()
	{

	}

	void Renderer::Render()
	{
		VkCommandBuffer commandBuffer = Walnut::Application::GetActiveCommandBuffer();
		auto wd = Walnut::Application::GetMainWindowData();

		float viewportWidth = (float)wd->Width;
		float viewportHeight = (float)wd->Height;

		// Bind the graphics pipeline.
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 3));

		m_PushConstants.ViewProjection = glm::perspectiveFov(glm::radians(45.0f), viewportWidth, viewportHeight, 0.1f, 1000.0f)
			* glm::inverse(cameraTransform);

		m_PushConstants.Transform = glm::translate(glm::mat4(1.0f), m_CubePosition)
			* glm::eulerAngleXYZ(glm::radians(m_CubeRotation.x), glm::radians(m_CubeRotation.y), glm::radians(m_CubeRotation.z));

		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &m_PushConstants);

		VkDeviceSize offset{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer.Handle, &offset);

		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.Handle, offset, VK_INDEX_TYPE_UINT32);

		VkViewport vp{};
		vp.y = viewportHeight;
		vp.width = viewportWidth;
		vp.height = -viewportHeight;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &vp);

		VkRect2D scissor{};
		scissor.extent.width = wd->Width;
		scissor.extent.height = wd->Height;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
	}

	void Renderer::RenderCube(const glm::vec3& position)
	{
		glm::vec3 translation = position * m_CubeScale;

		VkCommandBuffer commandBuffer = Walnut::Application::GetActiveCommandBuffer();
		auto wd = Walnut::Application::GetMainWindowData();

		float viewportWidth = (float)wd->Width;
		float viewportHeight = (float)wd->Height;

		// Bind the graphics pipeline.
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f), m_CameraPosition)
			* glm::eulerAngleXYZ(glm::radians(m_CameraRotation.x), glm::radians(m_CameraRotation.y), glm::radians(m_CameraRotation.z));

		m_PushConstants.ViewProjection = glm::perspectiveFov(glm::radians(45.0f), viewportWidth, viewportHeight, 0.1f, 1000.0f)
			* glm::inverse(cameraTransform);

		m_PushConstants.Transform = glm::translate(glm::mat4(1.0f), translation)
			* glm::eulerAngleXYZ(glm::radians(m_CubeRotation.x), glm::radians(m_CubeRotation.y), glm::radians(m_CubeRotation.z));

		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &m_PushConstants);

		VkDeviceSize offset{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer.Handle, &offset);

		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.Handle, offset, VK_INDEX_TYPE_UINT32);

		VkViewport vp{};
		vp.y = viewportHeight;
		vp.width = viewportWidth;
		vp.height = -viewportHeight;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &vp);

		VkRect2D scissor{};
		scissor.extent.width = wd->Width;
		scissor.extent.height = wd->Height;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
	}

	void Renderer::RenderUI()
	{
		ImGui::Begin("Controls");

		// ImGui::DragFloat3("Position", glm::value_ptr(m_CubePosition), 0.05f);
		// ImGui::DragFloat3("Rotation", glm::value_ptr(m_CubeRotation), 0.05f);

		ImGui::DragFloat3("Position", glm::value_ptr(m_CameraPosition), 0.05f);
		ImGui::DragFloat3("Rotation", glm::value_ptr(m_CameraRotation), 0.05f);
		ImGui::DragFloat("Scale", &m_CubeScale, 0.0005f);

		ImGui::End();
	}

	void Renderer::InitPipeline()
	{
		VkDevice device = GetVulkanInfo()->Device;
		VkRenderPass renderPass = Walnut::Application::GetMainWindowData()->RenderPass;

		std::array<VkPushConstantRange, 1> pushConstantRanges;
		pushConstantRanges[0].offset = 0;
		pushConstantRanges[0].size = sizeof(glm::mat4) * 2;
		pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// Create a blank pipeline layout.
	    // We are not binding any resources to the pipeline in this first sample.
		VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layout_info.pPushConstantRanges = pushConstantRanges.data();
		layout_info.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
		VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &m_PipelineLayout));

		std::array<VkVertexInputBindingDescription, 1> binding_desc;
		binding_desc[0].binding = 0;
		binding_desc[0].stride = sizeof(glm::vec3);
		binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 1> attribute_desc;
		attribute_desc[0].location = 0;
		attribute_desc[0].binding = binding_desc[0].binding;
		attribute_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_desc[0].offset = 0;

		VkPipelineVertexInputStateCreateInfo vertex_input{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertex_input.vertexBindingDescriptionCount = (uint32_t)binding_desc.size();
		vertex_input.pVertexBindingDescriptions = binding_desc.data();
		vertex_input.vertexAttributeDescriptionCount = (uint32_t)attribute_desc.size();
		vertex_input.pVertexAttributeDescriptions = attribute_desc.data();

		// Specify we will use triangle lists to draw geometry.
		VkPipelineInputAssemblyStateCreateInfo input_assembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Specify rasterization state.
		VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		raster.cullMode = VK_CULL_MODE_BACK_BIT;
		raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
		raster.lineWidth = 1.0f;

		// Our attachment will write to all color channels, but no blending is enabled.
		VkPipelineColorBlendAttachmentState blend_attachment{};
		blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		blend.attachmentCount = 1;
		blend.pAttachments = &blend_attachment;

		// We will have one viewport and scissor box.
		VkPipelineViewportStateCreateInfo viewport{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewport.viewportCount = 1;
		viewport.scissorCount = 1;

		// Disable all depth testing.
		VkPipelineDepthStencilStateCreateInfo depth_stencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		// No multisampling.
		VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Specify that these states will be dynamic, i.e. not part of pipeline state object.
		std::array<VkDynamicState, 2> dynamics{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamic{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamic.pDynamicStates = dynamics.data();
		dynamic.dynamicStateCount = (uint32_t)dynamics.size();

		// Load our SPIR-V shaders.
		std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

		// Vertex stage of the pipeline
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = LoadShader("Assets/Shaders/bin/basic.vert.spirv");
		shader_stages[0].pName = "main";

		// Fragment stage of the pipeline
		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = LoadShader("Assets/Shaders/bin/basic.frag.spirv");
		shader_stages[1].pName = "main";

		VkGraphicsPipelineCreateInfo pipe{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipe.stageCount = (uint32_t)shader_stages.size();
		pipe.pStages = shader_stages.data();
		pipe.pVertexInputState = &vertex_input;
		pipe.pInputAssemblyState = &input_assembly;
		pipe.pRasterizationState = &raster;
		pipe.pColorBlendState = &blend;
		pipe.pMultisampleState = &multisample;
		pipe.pViewportState = &viewport;
		pipe.pDepthStencilState = &depth_stencil;
		pipe.pDynamicState = &dynamic;

		// We need to specify the pipeline layout and the render pass description up front as well.
		pipe.renderPass = renderPass;
		pipe.layout = m_PipelineLayout;

		VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, nullptr, &m_GraphicsPipeline));

		// Pipeline is baked, we can delete the shader modules now.
		vkDestroyShaderModule(device, shader_stages[0].module, nullptr);
		vkDestroyShaderModule(device, shader_stages[1].module, nullptr);
	}


	void Renderer::InitBuffers()
	{
		VkDevice device = GetVulkanInfo()->Device;

		glm::vec3 vertexData[8] = {
			glm::vec3(-0.5f, -0.5f,  0.5f),
			glm::vec3(-0.5f,  0.5f,  0.5f),
			glm::vec3( 0.5f,  0.5f,  0.5f),
			glm::vec3( 0.5f, -0.5f,  0.5f),

			glm::vec3( 0.5f, -0.5f, -0.5f),
			glm::vec3( 0.5f,  0.5f, -0.5f),
			glm::vec3(-0.5f,  0.5f, -0.5f),
			glm::vec3(-0.5f, -0.5f, -0.5f)
		};

		uint32_t indices[36] = {
			0, 1, 2, 2, 3, 0, // Front
			3, 2, 5, 5, 4, 3, // Right
			4, 5, 6, 6, 7, 4, // Back
			7, 6, 1, 1, 0, 7, // Side
			1, 6, 5, 5, 2, 1, // Top
			7, 0, 3, 3, 4, 7, // Bottom
		};

		m_VertexBuffer.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		CreateOrResizeBuffer(m_VertexBuffer, sizeof(vertexData));

		m_IndexBuffer.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		CreateOrResizeBuffer(m_IndexBuffer, sizeof(indices));

		glm::vec3* vbMemory;
		VK_CHECK(vkMapMemory(device, m_VertexBuffer.Memory, 0, sizeof(vertexData), 0, (void**)&vbMemory));
		memcpy(vbMemory, vertexData, sizeof(vertexData));

		uint32_t* ibMemory;
		VK_CHECK(vkMapMemory(device, m_IndexBuffer.Memory, 0, sizeof(indices), 0, (void**)&ibMemory));
		memcpy(ibMemory, indices, sizeof(indices));

		VkMappedMemoryRange range[2] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = m_VertexBuffer.Memory;
		range[0].size = VK_WHOLE_SIZE;
		range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[1].memory = m_IndexBuffer.Memory;
		range[1].size = VK_WHOLE_SIZE;

		VK_CHECK(vkFlushMappedMemoryRanges(device, 2, range));
		vkUnmapMemory(device, m_VertexBuffer.Memory);
		vkUnmapMemory(device, m_IndexBuffer.Memory);
	}

	VkShaderModule Renderer::LoadShader(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			WL_ERROR("Could not open file! {}", path.string());
			return nullptr;
		}

		stream.seekg(0, std::ios_base::end);
		std::streampos size = stream.tellg();
		stream.seekg(0, std::ios_base::beg);

		std::vector<char> buffer(size);
		if (!stream.read(buffer.data(), size))
		{
			WL_ERROR("Could not read file! {}", path.string());
			return nullptr;
		}

		stream.close();

		VkShaderModuleCreateInfo shaderModuleCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shaderModuleCI.pCode = (uint32_t*)buffer.data();
		shaderModuleCI.codeSize = buffer.size();

		VkDevice device = GetVulkanInfo()->Device;
		VkShaderModule result = nullptr;
		VK_CHECK(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &result));
		return result;
	}

	void Renderer::CreateOrResizeBuffer(Buffer& buffer, uint64_t newSize)
	{
		VkDevice device = GetVulkanInfo()->Device;

		if (buffer.Handle != VK_NULL_HANDLE)
			vkDestroyBuffer(device, buffer.Handle, nullptr);
		if (buffer.Handle != VK_NULL_HANDLE)
			vkFreeMemory(device, buffer.Memory, nullptr);

		VkBufferCreateInfo bufferCI = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCI.size = newSize;
		bufferCI.usage = buffer.Usage;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateBuffer(device, &bufferCI, nullptr, &buffer.Handle));

		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(device, buffer.Handle, &req);
		// bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;
		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = req.size;
		alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
		VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &buffer.Memory));

		VK_CHECK(vkBindBufferMemory(device, buffer.Handle, buffer.Memory, 0));
		buffer.Size = req.size;
	}

}