#pragma once
#include <Windows.h>
#include <Dwmapi.h> 
#include <d3d11.h>
#include <string>
#include <chrono>
#include <thread>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

#define NNR0 255 
#define NNR1 65535 
#define NNR2 0
#define NNR3 8421376 
#define NNR4 33023 
#define NNR5 16746496 
#define NNR6 16711680 
#define NNR7 65280 
#define NNR8 16777215 
#define NNR9 8409343

class Overlay
{
public:
	void Start();
	DWORD CreateOverlay();
	void BeginDraw();
	void EndDraw();
	void ClickThrough(bool v);
	void DrawNewText(int X, int Y, int Color, float Size, const char* Str, ...);
	void DrawCircleFilled(int X, int Y, float Radius, int Color, int Segments, int tmz);
	void DrawCircle(int X, int Y, float Radius, int Color, int Segments, float thickness);
	void DrawRect(int X, int Y, int W, int H, int Color, float thickness, float rounding);
	void DrawFilledRect(int X, int Y, int W, int H, int Color, int tmz, float rounding);
	void DrawLine(int X1, int Y1, int X2, int Y2, int Color, float thickness);
	
public:
	bool running;
	HWND overlayHWND;
	HWND GameWindow;
	WINDOWINFO pwi;
	ULONG WindowX;
	ULONG WindowY;
	ULONG GameCenterW;
	ULONG GameCenterH;
	ULONG GameCenterX;
	ULONG GameCenterY;
};