
#include <random>
#include <utility>
#include <algorithm>

#include "tools/input.h"
#include "tools/mesh_loader.h"

#include "system/oct_tree.h"

#include "imgui_dx12/imgui.h"
#include "imgui_dx12/imgui_impl_win32.h"
#include "imgui_dx12/imgui_impl_dx12.h"

#include "App.hpp"

App::App() {

}

App::~App() {

}

bool App::initialize(HWND hwnd) {


	m_renderPipeline.initialize(hwnd);

	return true;
}

void App::shutdown() {
	m_renderPipeline.destroy();
}

void App::render() {

	m_renderPipeline.render();
}


void App::run(UINT curImageCount) {
	

	//ImGui_ImplDX12_NewFrame();
	//ImGui_ImplWin32_NewFrame();
	//ImGui::NewFrame();

	//ImGui::Begin("");

	//ImGui::End();
	//
	//ImGui::Render();

}
