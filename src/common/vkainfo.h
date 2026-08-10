#pragma once

// SoutheastRR: Bende senden nefret ediyorum.

#include <iostream>

// -- VKA ERROR MACROS

#ifndef VKA_GLFW_ERROR
	#define VKA_GLFW_ERROR(reason) \
		std::cout << "[GLFW_ERROR] (" << __FILE__ << ":" << __LINE__ << ") " << reason << std::endl;
#endif

#ifndef VKA_XML_PARSER_ERROR
	#define VKA_XML_PARSER_ERROR(reason) \
		std::cout << "[XML_PARSER_ERROR] (" << __FILE__ << ":" << __LINE__ << ") " << reason << std::endl;
#endif

// -- VKA INFO MACROS

#ifndef VKA_DEBUG_MSG
	#define VKA_DEBUG_MSG(message) \
		std::cout << "[VKA_DEBUG] (" << __FILE__ << ":" << __LINE__ << ") " << message << std::endl;
#endif