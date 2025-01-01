#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;
	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferWidth = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;
	
	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{};

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}



	}return 0;

	}

	return DefWindowProcW(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(
	const char* windowName,
	const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(0);
	windowClass.hIcon= 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(
		className,
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;
	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();

}

void gui::DestroyDevice() noexcept
{
	if (device) {
		device->Release();
		device = nullptr;
	}

	if (d3d) {
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);

}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// handle loss of device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

///////////////////////////////
// game logic and variables
///////////////////////////////


std::vector<std::vector<char>> board{ {' ', ' ', ' '}, 
									  {' ', ' ', ' '}, 
									  {' ', ' ', ' '} };
char currPlayer = 'X';
bool gameEnded = false;
int xWins = 0;
int oWins = 0;



bool checkWinner() {
	// check rows
	for (int row = 0; row < 3; row++) {
		if (board[row][0] == board[row][1] && board[row][1] == board[row][2] && board[row][0] != ' ') {
			gameEnded = true;
			if (board[row][0] == 'X')
				xWins++;
			if (board[row][0] == 'O')
				oWins++;
			return true;
		}
	}

	// check col
	for (int col = 0; col < 3; col++) {
		if (board[0][col] == board[1][col] && board[1][col] == board[2][col] && board[0][col] != ' ') {
			gameEnded = true;
			if (board[0][col] == 'X')
				xWins++;
			if (board[0][col] == 'O')
				oWins++;
			return true;
		}
	}

	// check diagnals
	if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') {
		gameEnded = true;
		if (board[0][0] == 'X')
			xWins++;
		if (board[0][0] == 'O')
			oWins++;
		return true;
	}
	if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ') {
		gameEnded = true;
		if (board[1][1] == 'X')
			xWins++;
		if (board[1][1] == 'O')
			oWins++;
		return true;
	}

	// handle tie
	bool tie = true;
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			if (board[row][col] == ' ') {
				tie = false;
				break;
			}

		}
	}
	if (tie) {
		gameEnded = true;
		return true;
	}


	return false;
}

void ResetGame() {
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++)
			board[row][col] = ' ';
	}
	currPlayer = 'X';
	gameEnded = false;
}




///////////////////////////////
// function call to call all buttons
///////////////////////////////

void ButtonCall()
{
	ImVec2 stanSize{ 100, 100 };

	float buttonDim{ 100.0f };
	ImVec2 buttonSize{ 100, 100 };
	float spaceBtwn{ 10.0f };
	ImGui::SetWindowFontScale(5.0f);

	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			float xPos = 90 + col * (buttonDim + spaceBtwn);
			float yPos = 90 + row * (buttonDim + spaceBtwn);

			// establish board
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			std::string bLable = std::string(1, board[row][col]) + "##" + std::to_string(row) + std::to_string(col);
			if (ImGui::Button(bLable.c_str(), buttonSize)) {
				if (board[row][col] == ' ') {
					board[row][col] = currPlayer;

					if (checkWinner()) {
						gameEnded = true;
					}
					else {
						currPlayer = (currPlayer == 'X') ? 'O' : 'X';			// alternate between X and O
					}
				}
			}
		}
	}

	ImGui::SetWindowFontScale(1.5f);
	ImGui::SetCursorPos(ImVec2(5, 25));
	ImGui::Text("Player X Wins: %d", xWins);
	ImGui::SetCursorPos(ImVec2(5, 45));
	ImGui::Text("Player O Wins: %d", oWins);


	if (gameEnded) {
		ImGui::SetNextWindowSize(ImVec2(200, 200));
		ImGui::OpenPopup("Game Over");
		if (ImGui::BeginPopupModal("Game Over", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::Text("Game Over");

			if (ImGui::Button("Play Again")) {
				ResetGame();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();

		}

	}
}




void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Play TicTacToe",
		&exit,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	ButtonCall();
	ImGui::SetWindowFontScale(1.0f);

	ImGui::End();
}