#pragma once

// SoutheastRR: Bende senden nefret ediyorum.

#include <iostream>

// -- VKA ANSI ESCAPE CODES

#define VKA_COLOR_RESET   "\033[0m"
#define VKA_COLOR_RED     "\033[31m"      // Error
#define VKA_COLOR_GREEN   "\033[32m"      // Success
#define VKA_COLOR_YELLOW  "\033[33m"      // Warning
#define VKA_COLOR_BLUE    "\033[34m"      // Info
#define VKA_COLOR_CYAN    "\033[36m"      // Debug
#define VKA_COLOR_BOLD    "\033[1m"


// -- VKA ERROR MACROS

#ifndef VKA_GLFW_ERROR
	#define VKA_GLFW_ERROR(reason) \
		std::cout << VKA_COLOR_RED << "[GLFW_ERROR] (" << __FILE__ << ":" << __LINE__ << ") " << reason << VKA_COLOR_RESET << std::endl;
#endif

#ifndef VKA_XML_PARSER_ERROR
	#define VKA_XML_PARSER_ERROR(reason) \
		std::cout << VKA_COLOR_RED << "[XML_PARSER_ERROR] (" << __FILE__ << ":" << __LINE__ << ") " << reason << VKA_COLOR_RESET << std::endl;
#endif

// -- VKA INFO MACROS

#ifndef VKA_DEBUG_MSG
	#define VKA_DEBUG_MSG(message) \
		std::cout << VKA_COLOR_BOLD << VKA_COLOR_CYAN << "[VKA_DEBUG] (" << __FILE__ << ":" << __LINE__ << ") " << message << VKA_COLOR_RESET << std::endl;
#endif