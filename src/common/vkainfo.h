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

enum class ErrorType {
	VKA_PARSER_ERROR,
	VKA_GLFW_ERROR,
	VKA_XML_PARSER_ERROR
};

inline const char* ErrorTypeToString(ErrorType e) noexcept {
	switch (e) {
	case ErrorType::VKA_PARSER_ERROR: return "VKA_PARSER_ERROR";
	case ErrorType::VKA_GLFW_ERROR:   return "VKA_GLFW_ERROR";
	case ErrorType::VKA_XML_PARSER_ERROR: return "VKA_XML_PARSER_ERROR";
	default: return "UNKNOWN_ERROR";
	}
}

inline std::ostream& operator<<(std::ostream& os, ErrorType e) {
	os << ErrorTypeToString(e);
	return os;
}

/////////////

#ifndef VKA_ERROR
	#define VKA_ERROR(err, reason) \
		std::cout << VKA_COLOR_RED << "[" << err << "] (" << __FILE__ << ":" << __LINE__ << ") " << reason << VKA_COLOR_RESET << std::endl;
#endif

// -- VKA INFO MACROS

#ifndef VKA_DEBUG_MSG
	#define VKA_DEBUG_MSG(message) \
		std::cout << VKA_COLOR_BOLD << VKA_COLOR_CYAN << "[VKA_DEBUG] (" << __FILE__ << ":" << __LINE__ << ") " << message << VKA_COLOR_RESET << std::endl;
#endif