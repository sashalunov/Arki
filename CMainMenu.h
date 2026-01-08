#pragma once
#include "imgui.h"

#include "stdafx.h"

enum MenuResult
{
    MENU_NO_ACTION,
    MENU_START_GAME_ARKI,
	MENU_START_GAME_FPS,
    MENU_OPEN_EDITOR,   
    MENU_OPEN_SETTINGS, 
    MENU_EXIT_GAME
};

class CMainMenu
{
public:
    CMainMenu() {}
    ~CMainMenu() {}

    MenuResult Render()
    {
        MenuResult result = MENU_NO_ACTION;

        // Centered Window
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 350), ImGuiCond_Always); // Made taller for more buttons

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        if (ImGui::Begin("Main Menu", nullptr, flags))
        {
            // Title
            ImGui::SetWindowFontScale(2.0f);
            const char* title = "ARKI ENGINE";
            float textW = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textW) * 0.5f);
            ImGui::TextColored(ImVec4(1, 1, 0, 1), title);
            ImGui::SetWindowFontScale(2.0f);

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

            // --- BUTTONS ---
            if (ImGui::Button("PLAY Arkanoid", ImVec2(-1, 45)))      result = MENU_START_GAME_ARKI;
            ImGui::Spacing();
            if (ImGui::Button("PLAY FPS", ImVec2(-1, 45)))      result = MENU_START_GAME_FPS;
            ImGui::Spacing();

            if (ImGui::Button("EDITOR", ImVec2(-1, 45))) result = MENU_OPEN_EDITOR;
            ImGui::Spacing();

            if (ImGui::Button("SETTINGS", ImVec2(-1, 45)))  result = MENU_OPEN_SETTINGS;
            ImGui::Spacing();

            if (ImGui::Button("EXIT", ImVec2(-1, 45)))      result = MENU_EXIT_GAME;

            ImGui::End();
        }

        return result;
    }
};