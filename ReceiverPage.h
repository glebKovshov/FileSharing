#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")

using namespace System::Windows::Forms;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace System::Drawing;
using namespace System::Collections::Generic;
using namespace System;

ref struct DeviceInfo
{
	String^ Name;
	IntPtr Addr;
};

namespace FileSharing {

	/// <summary>
	/// Summary for ReceiverPage
	/// </summary>
	public ref class ReceiverPage : public System::Windows::Forms::UserControl
	{
	public:
		ReceiverPage(System::Windows::Forms::Panel^ Frame, System::Action^ goBack)
		{
			this->Frame = Frame;
			this->goBack = goBack;
			InitializeComponent();
		}

	protected:
		~ReceiverPage()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		System::ComponentModel::Container ^components;
		System::Windows::Forms::Label^ label1;
		System::Windows::Forms::Button^ BackButton;
		System::Windows::Forms::Panel^ FindDevices;
		System::Windows::Forms::Panel^ Frame;
		System::Action^ goBack;
		SortedSet<String^>^ Devices = gcnew SortedSet<String^>();
		volatile bool isActive = true;
		SOCKET serverSocket = INVALID_SOCKET;
		SOCKET tcpListenSock = INVALID_SOCKET;
		SOCKET tcpClientSock = INVALID_SOCKET;
		Button^ deviceButton = nullptr;
		System::Windows::Forms::Label^ label2;
		System::Windows::Forms::ProgressBar^ progressBar1;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->BackButton = (gcnew System::Windows::Forms::Button());
			this->FindDevices = (gcnew System::Windows::Forms::Panel());
			this->progressBar1 = (gcnew System::Windows::Forms::ProgressBar());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(-3, 0);
			this->label1->Size = System::Drawing::Size(125, 13);
			this->label1->Text = L"Найденные устройства";
			this->label1->TextAlign = System::Drawing::ContentAlignment::TopCenter;
			// 
			// BackButton
			// 
			this->BackButton->Location = System::Drawing::Point(17, 250);
			this->BackButton->Size = System::Drawing::Size(244, 23);
			this->BackButton->UseVisualStyleBackColor = true;
			this->BackButton->Text = L"Назад";
			this->BackButton->Click += gcnew System::EventHandler(this, &ReceiverPage::BackButton_Click);
			// 
			// FindDevices
			// 
			this->FindDevices->AutoScroll = true;
			this->FindDevices->Location = System::Drawing::Point(0, 16);
			this->FindDevices->Size = System::Drawing::Size(276, 228);
			// 
			// progressBar1
			// 
			this->progressBar1->Location = System::Drawing::Point(16, 105);
			this->progressBar1->Size = System::Drawing::Size(244, 23);
			this->progressBar1->Visible = false;
			this->progressBar1->Maximum = 100;
			this->progressBar1->Minimum = 0;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(16, 86);
			this->label2->Size = System::Drawing::Size(74, 13);
			this->label2->Text = L"Получение";
			this->label2->Visible = false;
			// 
			// ReceiverPage
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->Controls->Add(this->FindDevices);
			this->Controls->Add(this->BackButton);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->progressBar1);
			this->Controls->Add(this->label2);
			this->Name = L"ReceiverPage";
			this->Size = System::Drawing::Size(276, 276);
			this->Load += gcnew System::EventHandler(this, &ReceiverPage::FindDeviceHandle);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: int chooseBufferSize(std::streampos fileSize) {
		if (fileSize < 1 * 1024 * 1024) {
			return 1024;
		}
		else if (fileSize < 10 * 1024 * 1024) {
			return 4096;
		}
		else {
			return 16384;
		}
	}

	private: System::Void BackButton_Click(System::Object^ sender, System::EventArgs^ e) {
		//this->FindDevices->PerformLayout();
		isActive = false;
		if (serverSocket != INVALID_SOCKET) {
			closesocket(serverSocket);
			serverSocket = INVALID_SOCKET;
		}
		if (tcpListenSock != INVALID_SOCKET) {
			closesocket(tcpListenSock);
			tcpListenSock = INVALID_SOCKET;
		}
		if (tcpClientSock != INVALID_SOCKET) {
			closesocket(tcpClientSock);
			tcpClientSock = INVALID_SOCKET;
		}
		deviceButton = nullptr;
		Clear();
		for each (Control ^ ctrl in this->Controls) {
			this->Controls->Remove(ctrl);
		}
		WSACleanup();
		goBack();
	}
	private: System::Void FindDeviceHandle(System::Object^ sender, System::EventArgs^ e) {
		Task::Run(gcnew Action(this, &ReceiverPage::FindDeviceRoutine));
	}
	private: void FindDeviceRoutine()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			MessageBox::Show(L"Failed to initialize Winsock", L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			return;
		}

		serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (serverSocket == INVALID_SOCKET) {
			MessageBox::Show(L"UDP socket creation error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			WSACleanup();
			return;
		}

		sockaddr_in udpServerAddr{};
		udpServerAddr.sin_family = AF_INET;
		udpServerAddr.sin_port = htons(12345);
		udpServerAddr.sin_addr.s_addr = INADDR_ANY;

		if (bind(serverSocket, reinterpret_cast<sockaddr*>(&udpServerAddr), sizeof(udpServerAddr)) == SOCKET_ERROR) {
			MessageBox::Show(L"UDP bind error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			WSACleanup();
			return;
		}

		const int hostnameSize = 256;
		char* buffer = (char*)malloc(hostnameSize);
		sockaddr_in clientAddr{};
		int clientLen = sizeof(clientAddr);
		
		int bytesReceived = 0;
		while (isActive && (bytesReceived = recvfrom(
			serverSocket,
			buffer,
			hostnameSize - 1,
			0,
			reinterpret_cast<sockaddr*>(&clientAddr),
			(socklen_t*)&clientLen)) > 0) 
		{
			buffer[bytesReceived] = '\0';
			String^ deviceName = gcnew System::String(buffer);
			if (!Devices->Contains(deviceName))
			{
				Devices->Add(deviceName);
				DeviceInfo^ di = gcnew DeviceInfo();
				clientAddr.sin_family = AF_INET;
				sockaddr_in* addCopy = (sockaddr_in*)malloc(sizeof(sockaddr_in));
				memcpy(addCopy, &clientAddr, sizeof(sockaddr_in));
				di->Addr = IntPtr(addCopy);
				di->Name = deviceName;

				this->BeginInvoke(gcnew Action<DeviceInfo^ >(this, &ReceiverPage::AddDeviceButton), di);
			}
		}

		if (bytesReceived == SOCKET_ERROR && isActive) {
			if (!isActive) {
				free(buffer);
				return;
			}
			MessageBox::Show(L"recvfrom() hostname (Receiver) error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			WSACleanup();
			free(buffer);
			return;
		}

		free(buffer);
	}
	private: void AddDeviceButton(DeviceInfo^ di)
		   {
			Button^ btn = gcnew Button();
			btn->Tag = di->Addr;
			btn->Text = di->Name;
			btn->Size = Drawing::Size(270, 40);
			btn->Location = Drawing::Point(3, 3 + 50 * FindDevices->Controls->Count);
			
			btn->Click += gcnew EventHandler(this, &ReceiverPage::DeviceButton_Click);
			FindDevices->Controls->Add(btn);
		   }

	private: System::Void DeviceButton_Click(System::Object^ sender, System::EventArgs^ e) {
		Button^ btn = safe_cast<Button^>(sender);
		String^ deviceName = btn->Text;

		this->FindDevices->Enabled = false;
		this->BackButton->Enabled = false;

		System::Windows::Forms::DialogResult result = MessageBox::Show(
			L"Подключиться к устройству " + deviceName + "?", L"Подтверждение",
			MessageBoxButtons::YesNo, MessageBoxIcon::Question);

		this->FindDevices->Enabled = true;
		this->BackButton->Enabled = true;

		if (result == DialogResult::No) return;

		isActive = false;
		deviceButton = btn;

		Clear();

		Task::Run(gcnew Action(this, &ReceiverPage::RecvFileRoutine));
	}
	private: System::Void RecvFileRoutine(){
		sockaddr_in* clientAddr = static_cast<sockaddr_in*>(safe_cast<IntPtr>(deviceButton->Tag).ToPointer());
		
		char* buffer;
		int bufferSize = 256;
		buffer = (char*)malloc(bufferSize);
		memset(buffer, 0, bufferSize);
		if (gethostname(buffer, bufferSize) == SOCKET_ERROR) {
			MessageBox::Show(L"gethostname() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			WSACleanup();
			free(buffer);
			return;
		}
		int clientLen = sizeof(sockaddr_in);
		int nameLen = strlen(buffer) + 1;
		int sent = sendto(serverSocket,
			buffer,
			nameLen,
			0,
			reinterpret_cast<sockaddr*>(clientAddr),
			clientLen);

		free(buffer);

		if (sent == SOCKET_ERROR) {
			if (!isActive) return;
			MessageBox::Show(L"sendto() hostname error (Receiver) " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			WSACleanup();
			return;
		}

		tcpListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tcpListenSock == INVALID_SOCKET) {
			MessageBox::Show(L"TCP listen socket creation error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			WSACleanup();
			return;
		}

		sockaddr_in tcpServerBind{};
		tcpServerBind.sin_family = AF_INET;
		tcpServerBind.sin_port = 0; // Порт выберется ОС
		tcpServerBind.sin_addr.s_addr = INADDR_ANY;

		if (bind(tcpListenSock, reinterpret_cast<sockaddr*>(&tcpServerBind), sizeof(tcpServerBind)) == SOCKET_ERROR) {
			MessageBox::Show(L"TCP bind error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(tcpListenSock);
			closesocket(serverSocket);
			WSACleanup();
			return;
		}

		unsigned short port;
		sockaddr_in assignedAddr{};
		int addrLen = sizeof(assignedAddr);
		if (getsockname(tcpListenSock, reinterpret_cast<sockaddr*>(&assignedAddr), (socklen_t*)&addrLen) == 0) {
			port = ntohs(assignedAddr.sin_port);
		}
		else {
			MessageBox::Show(L"getsockname() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			closesocket(tcpListenSock);
			WSACleanup();
			return;
		}

		int sentBytes = sendto(serverSocket, // server port
			reinterpret_cast<const char*>(&port),
			sizeof(port),
			0,
			reinterpret_cast<sockaddr*>(clientAddr),
			clientLen);
		if (sentBytes == SOCKET_ERROR) {
			if (!isActive) return;
			MessageBox::Show(L"sendto() port error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			closesocket(tcpListenSock);
			WSACleanup();
			return;
		}
		this->label1->Text = L"Подключено. Ожидание файла";

		closesocket(serverSocket);
		free(clientAddr);
		clientAddr = nullptr;
		
		if (listen(tcpListenSock, 1) == SOCKET_ERROR) {
			MessageBox::Show(L"listen() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(tcpListenSock);
			WSACleanup();
			return;
		}

		sockaddr_in tcpClientAddr{};
		int tcpClientAddrLen = sizeof(tcpClientAddr);
		tcpClientSock = accept(tcpListenSock,
			reinterpret_cast<sockaddr*>(&tcpClientAddr),
			(socklen_t*)&tcpClientAddrLen);
		if (tcpClientSock == INVALID_SOCKET) {
			if (!isActive) return;
			MessageBox::Show(L"accept() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(tcpListenSock);
			WSACleanup();
			return;
		}

		uint64_t fileSize;
		int totalBytes = 0;
		char* fileSizePtr = reinterpret_cast<char*>(&fileSize);
		while (totalBytes < sizeof(fileSize)) {
			int bytes = recv(tcpClientSock, fileSizePtr + totalBytes, sizeof(fileSize) - totalBytes, 0);
			if (bytes <= 0) {
				if (!isActive) return;
				MessageBox::Show(L"recv() error for fileSize: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
				closesocket(tcpClientSock);
				closesocket(tcpListenSock);
				WSACleanup();
				return;
			}
			totalBytes += bytes;
		}

		char fPath[512];
		totalBytes = 0;
		while (totalBytes < sizeof(fPath) - 1) {
			int bytes = recv(tcpClientSock, fPath + totalBytes, 1, 0);
			if (bytes <= 0) {
				if (!isActive) return;
				MessageBox::Show(L"recv() error file name: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
				closesocket(tcpClientSock);
				closesocket(tcpListenSock);
				WSACleanup();
				return;
			}
			if (fPath[totalBytes] == '\0')
				break;
			totalBytes += bytes;
		}
		if (totalBytes == sizeof(fPath) - 1)
			fPath[sizeof(fPath) - 1] = '\0';

		std::string fullPath(fPath);
		size_t pos = fullPath.find_last_of("\\/");
		std::string fileNameOnly = (pos != std::string::npos) ? fullPath.substr(pos + 1) : fullPath;

		std::ofstream recvFile(fileNameOnly, std::ios::binary);
		if (!recvFile)
		{
			MessageBox::Show(L"Error opening file for writing", L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(tcpClientSock);
			closesocket(tcpListenSock);
			WSACleanup();
			return;
		}

		bufferSize = chooseBufferSize(fileSize);
		buffer = (char*)malloc(bufferSize);

		this->Invoke(gcnew Action(this, &ReceiverPage::ShowProgress));

		int recvBytes = 0, totalRecv = 0, lastProgress = -1;
		while ((recvBytes = recv(tcpClientSock, buffer, bufferSize, 0)) > 0) {
			recvFile.write(buffer, recvBytes);
			totalRecv += recvBytes;
			int progress = static_cast<int>((totalRecv * 100) / fileSize);
			if (progress > lastProgress) {
				lastProgress = progress;
				this->BeginInvoke(gcnew Action<int>(this, &ReceiverPage::UpdateProgress), progress);
			}
		}

		if (recvBytes == SOCKET_ERROR)
		{
			MessageBox::Show(L"recv() failed or connection closed prematurely: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		}

		MessageBox::Show(L"Успешно! (получатель)", "Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);

		free(buffer);
		recvFile.close();
		closesocket(tcpClientSock);
		closesocket(tcpListenSock);
		WSACleanup();

		label2->Visible = false;
		progressBar1->Visible = false;

		goBack();
	}

	private: void UpdateProgress(int percent) {
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;
		progressBar1->Value = percent;
	}
	private: void ShowProgress() {
		FindDevices->Visible = false;
		label2->Visible = true;
		progressBar1->Visible = true;
	}
	private: void Clear() {
		for each(Control ^ ctrl in FindDevices->Controls)
		{
			Button^ btn = dynamic_cast<Button^>(ctrl);
			if (btn && btn->Tag != nullptr )
			{
				IntPtr ptr = safe_cast<IntPtr>(btn->Tag);
				sockaddr_in* addr = static_cast<sockaddr_in*>(ptr.ToPointer());
				FindDevices->Controls->Remove(ctrl);
				if (btn != deviceButton) free(addr);
			}
		}
	}
};
}