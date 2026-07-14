#include "Interface.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"
#include "Camera.hpp"

#include <iostream>


namespace WGPUBoids {
	

Interface::~Interface()
{
	destroy();
}

bool Interface::init(GLFWwindow* window, Camera* pCamera, wgpu::Device device, wgpu::TextureFormat targetFormat)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
    bool success = ImGui_ImplGlfw_InitForOther(window, true) && ImGui_ImplWGPU_Init(device.Get(), 3, static_cast<WGPUTextureFormat>(targetFormat), WGPUTextureFormat_Undefined);
	if (!success) return false;

    glfwWindow = window;
    camera = pCamera;

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
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.3f); 
    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | 
                                    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::Begin("ToggleWindow", nullptr, overlayFlags);

    if (ImGui::Button(showControlPanel ? "Hide Panel" : "Show Panel")) {
        showControlPanel = !showControlPanel;
    }

    ImGui::End();

    if (showControlPanel) {
        ImGui::Begin("Control Panel", &showControlPanel);

        ImGui::TextColored(ImVec4(0.83, 0.64, 0.2, 1),"Hold down CTRL and click any parameter to set a custom value");

        ImGui::Spacing();
        ImGui::SeparatorText("Main Options");
        ImGui::Spacing();


        if (ImGui::CollapsingHeader("Distance")) {
            ImGui::SliderFloat("Visual Range", &params.visualRange, 0.5f, 10.0f);
            ImGui::SliderFloat("Protected Range", &params.protectedRange, 0.05f, 3.0f);
            ImGui::SliderFloat("Vision Angle (Deg)", &params.visionRadius, 0.0f, 360.f);
        }
        
        if (ImGui::CollapsingHeader("Cube Params")) {
            ImGui::SliderFloat("Cube Size", &params.cubeSize, 2.0f, 20.0f);
            ImGui::SliderFloat("Margin Cube", &params.margin, 0.1f, 2.f);
        }
        
        if (ImGui::CollapsingHeader("Boids Speed")) {
            ImGui::SliderFloat("Max Speed", &params.maxSpeed, 1.0f, 15.0f);
            ImGui::SliderFloat("Min Speed", &params.minSpeed, 0.0f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::SliderFloat("Cohesion Factor", &params.cohesionFactor, 0.0f, 0.05f, "%.4f");
            ImGui::SliderFloat("Alignment Factor", &params.alignmentFactor, 0.0f, 0.2f, "%.4f");
            ImGui::SliderFloat("Separation Factor", &params.separationFactor, 0.0f, 0.2f, "%.4f");
            ImGui::SliderFloat("Turn Factor", &params.turnFactor, 0.01f, 1.0f);
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Advanced Options");
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Simulation Rules")) {
            uint32_t minBoids = 0;
            uint32_t maxBoids = 100000;
            ImGui::SliderScalar("Active Boids", ImGuiDataType_U32, &params.activeBoidsCount, &minBoids, &maxBoids, "%u");
            ImGui::Checkbox("Divide into Flocks", &divideFlocks);
            params.divideFlocks = divideFlocks ? 1u : 0u;

            if (params.divideFlocks) {
                ImGui::SeparatorText("Divide flocks Options");
                ImGui::SliderFloat("Stranger Protected Range", &params.strangerProtectedRange, 2.f, 5.f);
                ImGui::SliderFloat("Stranger Force Factor", &params.strangeForceFactor, 5.f, 20.f);
            }
        }

        

        if (ImGui::CollapsingHeader("Debug Options")) {
            ImGui::Checkbox("Show Velocity", &showVelocity);
            ImGui::Checkbox("Show Center of Mass", &showCoM);
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Camera Settings")) {
            if (camera != nullptr) {
                int currentMode = (camera->getMode() == CameraMode::Free) ? 0 : 1;
                int previousMode = currentMode;

                ImGui::RadioButton("Free Camera", &currentMode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Orbital", &currentMode, 1);

                if (currentMode != previousMode) {
                    if (currentMode == 0) camera->setMode(CameraMode::Free);
                    else camera->setMode(CameraMode::Orbital);
                }

                ImGui::Spacing();

                float currentSpeed = camera->getMovementSpeed();
                if (ImGui::SliderFloat("Camera Speed", &currentSpeed, 1.0f, 100.0f)) {
                    camera->setMovementSpeed(currentSpeed);
                }

                if (camera->getMode() == CameraMode::Orbital) {
                    float currentRadius = camera->getRadius();
                    if (ImGui::SliderFloat("Orbital Distance", &currentRadius, 1.0f, 300.0f)) {
                        camera->setRadius(currentRadius);
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(1,0,0,1), "Camera pointer is missing!");
            }
        };


        if (ImGui::CollapsingHeader("System & Performance")) {
            if (targetFPS == 0) {
                ImGui::Text("FPS Limit: Uncapped");
            } else {
                ImGui::Text("FPS Limit: %d", targetFPS);
            }
            static int tempFPS = targetFPS;

            ImGui::SliderInt("FPS Limit", &tempFPS, 0, 240);
            
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                targetFPS = tempFPS;
            }
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Exit Simulation", ImVec2(-1, 0))) {
            glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
        }
        ImGui::PopStyleColor(2);

        ImGui::End();
    }
}

void Interface::draw(wgpu::RenderPassEncoder renderPass)
{
	ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass.Get());
}

} // namespace WGPUBoids