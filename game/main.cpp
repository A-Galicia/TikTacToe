#pragma once
#include "../game/gui.h"
#include <thread>

int _stdcall wWinMain(
	HINSTANCE instace,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	// create gui
	gui::CreateHWindow("TicTacToe", "TicTacToe class");
	gui::CreateDevice();
	gui::CreateImGui();

	while (gui::exit)
	{
		gui::BeginRender();
		gui::Render();
		gui::EndRender();


		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// destroy gui
	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}