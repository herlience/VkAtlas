#pragma once

#include <GLFW/glfw3.h>
#include <string>

namespace VKA::UI {

	class Window {
	public:
		Window(int width = 1200, int height = 800, std::string title = "VkAtlas");
		~Window();

		bool shoudlclose() const;
		void beginframe();
		void endframe();

		void initImgui();

		GLFWwindow* getNativeWindow() const { return windowhandle; }
	private:
		int m_width;
		int m_height;
		std::string m_title;

		GLFWwindow* windowhandle = nullptr;
	};
}