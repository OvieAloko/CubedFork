#pragma once

#include "Vulkan.h"

#include "glm/glm.hpp"

#include <filesystem>

namespace Cubed {

	struct Buffer
	{
		VkBuffer Handle = nullptr;
		VkDeviceMemory Memory = nullptr;
		VkDeviceSize Size = 0;
		VkBufferUsageFlagBits Usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	};

	class Renderer
	{
	public:
		void Init();
		void Shutdown();

		void Render();
		void RenderCube(const glm::vec3& position);
		void RenderUI();
	private:
		void InitPipeline();
		void InitBuffers();

		void CreateOrResizeBuffer(Buffer& buffer, uint64_t newSize);

		VkShaderModule LoadShader(const std::filesystem::path& path);
	private:
		VkPipeline m_GraphicsPipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;

		Buffer m_VertexBuffer, m_IndexBuffer;

		struct PushConstants
		{
			glm::mat4 ViewProjection;
			glm::mat4 Transform;
		} m_PushConstants;

		glm::vec3 m_CubePosition{ 0 };
		glm::vec3 m_CubeRotation{ 0 };

		glm::vec3 m_CameraPosition{ 0, 0, 3 };
		glm::vec3 m_CameraRotation{ 0 };
		float m_CubeScale = 0.1f;
	};

}