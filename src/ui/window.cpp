#include <iostream>

#include "../common/vkainfo.h"
#include "../core/interface.hpp"

#include "window.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace VKA::UI::WINDOW {

	Window::Window(int width, int height, std::string title)
		: m_width(width), m_height(height), m_title(title) {
		if (!glfwInit()) {
			VKA_GLFW_ERROR("Failed to initialize GLFW!");
			return;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		windowhandle = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
		if (!windowhandle) {
			VKA_GLFW_ERROR("Failed to create window!");
			glfwTerminate();
			return;
		}

		glfwSetDropCallback(windowhandle, VKA::CORE::INTERFACE::drop_callback);

		glfwMakeContextCurrent(windowhandle);
		glfwSwapInterval(1);
		initImgui();
	}

	Window::~Window() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (windowhandle) {
			glfwDestroyWindow(windowhandle);
		}

		glfwTerminate();
	}

	void Window::initImgui() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(windowhandle, true);
		ImGui_ImplOpenGL3_Init("#version 330");
	}

	bool Window::shoudlclose() const {
		return glfwWindowShouldClose(windowhandle);
	}

	void Window::beginframe() {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Window::endframe() {
		ImGui::Render();

		int disp_w, disp_h;
		glfwGetFramebufferSize(windowhandle, &disp_w, &disp_h);
		glViewport(0, 0, disp_w, disp_h);
		glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(windowhandle);
	}
} // VKA::UI::WINDOW