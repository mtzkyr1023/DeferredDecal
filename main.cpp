#define     NAME    "WINDOW"  //�^�C�g���o�[�ɕ\������e�L�X�g

#pragma comment(lib, "d3dcompiler.lib")
#define NOMINMAX
#include <windows.h>
#include <crtdbg.h>
#include <iostream>
#include <thread>

#include "tools/input.h"

#include "imgui_dx12/imgui.h"
#include "imgui_dx12/imgui_impl_win32.h"
#include "imgui_dx12/imgui_impl_dx12.h"

#include "App.hpp"


#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

using namespace std;


//�v���g�^�C�v�錾
LRESULT  CALLBACK   WndProc(HWND, UINT, WPARAM, LPARAM);
int      WINAPI     WinMain(HINSTANCE, HINSTANCE, LPSTR, int);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//Windws �C�x���g�p�֐�
LRESULT  CALLBACK  WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	//�n���ꂽ message ����A�C�x���g�̎�ނ���͂���
	switch (msg) {
		//----�I������----
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		//----�f�t�H���g�̏���----
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0L;
}

//Windows Main �֐�
int  WINAPI  WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {

	HWND        hWnd;
	MSG         msg;

	// Set up and register window class

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, NAME, NULL };
	if (RegisterClassEx(&wc) == 0) return FALSE;

	//�E�C���h�E����
	hWnd = CreateWindow(NAME,                  //�^�C�g���o�[�e�L�X�g
		NAME,                  //�N���X��
		WS_OVERLAPPEDWINDOW,   //�E�C���h�E�X�^�C��
		(GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2,         //�E�C���h�E����x���W
		(GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2,         //�E�C���h�E����y���W
		WINDOW_WIDTH,         //�E�C���h�E��
		WINDOW_HEIGHT,         //�E�C���h�E����
		NULL,                  //�e�E�C���h�E�̃n���h��
		NULL,
		hInstance,
		NULL);
	if (!hWnd) return FALSE;

	ShowWindow(hWnd, nCmdShow);     //Window ��\��
	UpdateWindow(hWnd);             //�\����������
	SetFocus(hWnd);                 //�t�H�[�J�X��ݒ�

	App app;
	if (!app.initialize(hWnd))
		return false;

	if (!Input::Instance().Initialize(hWnd))
		return false;

	while (1) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT) {
			break;
		}
		else {
			if (GetAsyncKeyState(VK_ESCAPE))
				break;

			app.render();

			Input::Instance().Updata();
		}
	}

	app.shutdown();

	return (int)msg.wParam;
}