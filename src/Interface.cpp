#include "Interface.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"

// std
#include <iostream>


namespace WGPUBoids {
	

Interface::~Interface()
{
	destroy();
}

bool Interface::init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat targetFormat)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
    bool success = ImGui_ImplGlfw_InitForOther(window, true) && ImGui_ImplWGPU_Init(device.Get(), 3, static_cast<WGPUTextureFormat>(targetFormat), WGPUTextureFormat_Undefined);
	if (!success) return false;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();

	float xScale, yScale;
	glfwGetWindowContentScale(window, &xScale, &yScale);
	currentScale = xScale;

	io.Fonts->AddFontDefault();

    io.FontGlobalScale = currentScale;

	ImGui::GetStyle().ScaleAllSizes(currentScale * 2.f);
	ImGui::GetStyle().ButtonTextAlign = ImVec2(0.5f, 0.5f);

	return true;
}

void Interface::destroy()
{
	if (ImGui::GetCurrentContext()) {
		ImGui_ImplWGPU_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

void Interface::beginFrame()
{
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Interface::buildUI()
{
    std::string selectedPath = "";
    
    ImGui::Begin("Boids Simulation");
    
    ImGui::Text("Control Panel");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SeparatorText("Distance");
    ImGui::SliderFloat("Visual Range", &params.visualRange, 0.5f, 10.0f);
    ImGui::SliderFloat("Protected Range", &params.protectedRange, 0.05f, 3.0f);
    ImGui::SliderFloat("Vision Angle (Deg)", &params.visionRadius, 0.0f, 360.f);

    ImGui::SeparatorText("Cube Params");
    ImGui::SliderFloat("Cube Size", &params.cubeSize, 2.0f, 20.0f);
    ImGui::SliderFloat("Margin Cube", &params.margin, 0.1f, 2.f);

    ImGui::SeparatorText("Boids Speed");
    ImGui::SliderFloat("Max Speed", &params.maxSpeed, 1.0f, 15.0f);
    ImGui::SliderFloat("Min Speed", &params.minSpeed, 0.0f, 5.0f);

    ImGui::SeparatorText("Factors");
    ImGui::SliderFloat("Cohesion Factor", &params.cohesionFactor, 0.0f, 0.05f, "%.4f");
    ImGui::SliderFloat("Alignment Factor", &params.alignmentFactor, 0.0f, 0.2f, "%.4f");
    ImGui::SliderFloat("Separation Factor", &params.separationFactor, 0.0f, 0.2f, "%.4f");
    ImGui::SliderFloat("Turn Factor", &params.turnFactor, 0.01f, 1.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SeparatorText("Simulation Rules");
    uint32_t minBoids = 0;
    uint32_t maxBoids = 100000;
    ImGui::SliderScalar("Active Boids", ImGuiDataType_U32, &params.activeBoidsCount, &minBoids, &maxBoids, "%u");

    ImGui::Checkbox("Divide into Flocks", &divideFlocks);
    params.divideFlocks = divideFlocks ? 1u : 0u;

    ImGui::Spacing();
    ImGui::SeparatorText("Debug Options");
    ImGui::Checkbox("Show Velocity", &showVelocity);
    ImGui::Checkbox("Show Center of Mass", &showCoM);


    ImGui::End();
}

void Interface::draw(wgpu::RenderPassEncoder renderPass)
{
	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
}

} // namespace WGPUBoids