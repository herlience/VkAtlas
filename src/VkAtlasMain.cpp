#include "ui/window.h"
#include <imgui.h>
#include "common/VkAtlasData.h"
#include "parser/XMLParsing.h"
#include "core/interface.hpp"
#include <iostream>

int main() {
	VKA::UI::WINDOW::Window window;
	VKA::DATA::GraphContext context;
	VKA::PARSER::XML::XMLParsing xmlparser;

	xmlparser.initAndParse(context);

	while (!window.shoudlclose()) {
		window.beginframe();
		
		VKA::CORE::INTERFACE::renderUI(context);

		window.endframe();
	}

	return 0;
}