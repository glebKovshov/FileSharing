#include "MyForm.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

using namespace FileSharing;

[STAThreadAttribute]
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox::Show(L"Не удалось инициализировать Winsock", L"Ошибка",
            MessageBoxButtons::OK, MessageBoxIcon::Error);
        return -1;
    }

    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    Application::Run(gcnew MyForm());

    WSACleanup();

    return 0;
}
