#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#include "Application.h"
#include "Demo.h"
#include "DirectX12Headers.h"

#include "Libs\imgui-master\examples\directx12_example\imgui_impl_dx12.h"

#include <initguid.h>
#include <dxgidebug.h>
#include <memory>


void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

#define USE_CONSOLE			1

#if USE_CONSOLE
int main()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
#else
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#endif
	int retCode = 0;

	// Set the working directory to the path of the executable.
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandleW(NULL);
	if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

	Application::Create(hInstance);
	{
		std::shared_ptr<Demo> demo = std::make_shared<Demo>(L"DirectX 12 PBR Demo", 1280, 720);

		retCode = Application::Get().Run(demo);
	}
	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}