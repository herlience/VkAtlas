#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <filesystem>
#include "../common/VkAtlasData.h"

namespace VKA::CORE::INTERFACE {
	inline std::vector<std::filesystem::path> g_selectedfiles;
	
	// MAIN FUNCTIONS 
	void renderUI(VKA::DATA::GraphContext& context);
	void drop_callback(GLFWwindow* window, int count, const char** paths);

	// PRIVATE FUNCTIONS
	void openfileDialog();
	std::string readFileToString(const std::filesystem::path& filePath);
}