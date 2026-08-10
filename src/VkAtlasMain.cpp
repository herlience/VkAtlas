#include "ui/window.h"
#include <imgui.h>
#include "core/VkAtlasData.h"
#include "parser/XMLParsing.h"
#include <iostream>

int main() {
	VKA::UI::Window window;

	while (!window.shoudlclose()) {
		window.beginframe();
		
		ImGui::ShowDemoWindow();

		window.endframe();
	}

	window.~Window();
	return 0;
}