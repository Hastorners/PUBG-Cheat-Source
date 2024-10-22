#include "overlay.h"

const MARGINS margins = { -1 ,-1, -1, -1 };
const wchar_t g_szClassName[] = L"Overlay";
WNDCLASSEX wc;

LONG nv_default = WS_POPUP | WS_CLIPSIBLINGS;
LONG nv_default_in_game = nv_default | WS_DISABLED;
LONG nv_edit = nv_default_in_game | WS_VISIBLE;

LONG nv_ex_default = WS_EX_TOOLWINDOW;
LONG nv_ex_edit = nv_ex_default | WS_EX_LAYERED | WS_EX_TRANSPARENT;
LONG nv_ex_edit_menu = nv_ex_default | WS_EX_TRANSPARENT;

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

std::string AnisToUTF8(const std::string& Str)
{
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, Str.c_str(), -1, NULL, 0);

	wchar_t* pwBuf = new wchar_t[(size_t)nwLen + 1];
	ZeroMemory(pwBuf, (size_t)nwLen * 2 + 2);
	::MultiByteToWideChar(CP_ACP, 0, Str.c_str(), Str.length(), pwBuf, nwLen);
	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char* pBuf = new char[(size_t)nLen + 1];
	ZeroMemory(pBuf, (size_t)nLen + 1);
	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
	std::string retStr(pBuf);
	delete[]pwBuf;
	delete[]pBuf;
	pwBuf = NULL;
	pBuf = NULL;
	return retStr;
}

static DWORD WINAPI StaticMessageStart(void* Param)
{
	Overlay* ov = (Overlay*)Param;
	ov->CreateOverlay();
	return 0;
}

void Overlay::Start()
{
	DWORD ThreadID;
	CreateThread(NULL, 0, StaticMessageStart, (void*)this, 0, &ThreadID);
}

void Overlay::ClickThrough(bool v)
{
	if (v) {
		nv_edit = nv_default_in_game | WS_VISIBLE;
		if (GetWindowLong(overlayHWND, GWL_EXSTYLE) != nv_ex_edit)
			SetWindowLong(overlayHWND, GWL_EXSTYLE, nv_ex_edit);
	}
	else {
		nv_edit = nv_default | WS_VISIBLE;
		if (GetWindowLong(overlayHWND, GWL_EXSTYLE) != nv_ex_edit_menu)
			SetWindowLong(overlayHWND, GWL_EXSTYLE, nv_ex_edit_menu);
	}
}

DWORD Overlay::CreateOverlay()
{
	pwi.cbSize = 60;
	GetWindowInfo(GameWindow, &pwi);
	WindowX = pwi.rcClient.left;
	WindowY = pwi.rcClient.top;
	GameCenterW = pwi.rcClient.right - WindowX;
	GameCenterH = pwi.rcClient.bottom - WindowY;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = NULL;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)(RGB(0, 0, 0));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = NULL;

	RegisterClassEx(&wc);

	overlayHWND = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, g_szClassName, g_szClassName, WS_POPUP | WS_VISIBLE, WindowX, WindowY, GameCenterW, GameCenterH, NULL, NULL, NULL, NULL);

	if (overlayHWND == 0)
		return 0;

	SetLayeredWindowAttributes(overlayHWND, RGB(0, 0, 0), 255, LWA_ALPHA);

	DwmExtendFrameIntoClientArea(overlayHWND, &margins);

	// Initialize Direct3D
	if (!CreateDeviceD3D(overlayHWND))
	{
		CleanupDeviceD3D();
		return 0;
	}

	// Show the window
	::ShowWindow(overlayHWND, SW_SHOWDEFAULT);
	::UpdateWindow(overlayHWND);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	//io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowMinSize = ImVec2(1, 1);

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(overlayHWND);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	running = true;

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ClickThrough(true);

	while (running)
	{
		HWND wnd = GetWindow(GetForegroundWindow(), GW_HWNDPREV);

		if (wnd != overlayHWND)
		{
			::SetWindowPos(overlayHWND, wnd, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);
			::UpdateWindow(overlayHWND);
		}

		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}
		GetWindowInfo(GameWindow, &pwi);
		WindowX = pwi.rcClient.left;
		WindowY = pwi.rcClient.top;
		GameCenterW = pwi.rcClient.right - WindowX;
		GameCenterH = pwi.rcClient.bottom - WindowY;
		GameCenterX = GameCenterW / 2;
		GameCenterY = GameCenterH / 2;
		MoveWindow(overlayHWND, WindowX, WindowY, GameCenterW, GameCenterH, false);

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ClickThrough(true);
	CleanupDeviceD3D();
	::DestroyWindow(overlayHWND);
	return 0;
}

void Overlay::BeginDraw()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//»æÖÆFPS
	char dist[64];
	sprintf_s(dist, "(%.1f FPS)\n", ImGui::GetIO().Framerate);
	DrawNewText(15, 15, NNR7, 18, dist);
}

void Overlay::EndDraw()
{
	ImGui::EndFrame();
	ImGui::Render();
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	g_pSwapChain->Present(1, 0);
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer)
	{
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

bool CreateDeviceD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 240;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void Overlay::DrawNewText(int X, int Y, int Color, float Size, const char* txt, ...)
{
	char str[128];

	va_list va_alist;
	va_start(va_alist, txt);
	_vsnprintf_s(str, sizeof(str), sizeof(str) - strlen(str), txt, va_alist);
	va_end(va_alist);

	std::string UTF8 = AnisToUTF8(std::string(str));

	ImGui::GetForegroundDrawList()->AddTextEx(ImVec2(X, Y), Size, ImGui::ImAlphaBlendColors(Color, 255), UTF8.c_str());
}


void Overlay::DrawCircleFilled(int X, int Y, float Radius, int Color, int Segments, int tmz)
{
	ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(X, Y), Radius, ImGui::ImAlphaBlend(Color, 255, tmz), Segments);
}

void Overlay::DrawCircle(int X, int Y, float Radius, int Color, int Segments, float thickness)
{
	ImGui::GetForegroundDrawList()->AddCircle(ImVec2(X, Y), Radius, ImGui::ImAlphaBlendColors(Color, 255), Segments, thickness);
}

void Overlay::DrawRect(int X, int Y, int W, int H, int Color, float thickness, float rounding)
{
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(X, Y), ImVec2(X + W, Y + H), ImGui::ImAlphaBlendColors(Color, 255), rounding, 15, thickness);
}

void Overlay::DrawFilledRect(int X, int Y, int W, int H, int Color, int tmz, float rounding)
{
	ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(X, Y), ImVec2(X + W, Y + H), ImGui::ImAlphaBlend(Color, 255, tmz), rounding);
}

void Overlay::DrawLine(int X1, int Y1, int X2, int Y2, int Color, float thickness)
{
	ImGui::GetForegroundDrawList()->AddLine(ImVec2(X1, Y1), ImVec2(X2, Y2), ImGui::ImAlphaBlendColors(Color, 255), thickness);
}