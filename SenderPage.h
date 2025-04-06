#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <msclr/marshal_cppstd.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace FileSharing {

	/// <summary>
	/// Summary for SenderPage
	/// </summary>
	public ref class SenderPage : public System::Windows::Forms::UserControl
	{
	public:
		SenderPage(System::Windows::Forms::Panel^ Frame, System::Action^ goBack)
		{
			this->Frame = Frame;
			this->goBack = goBack;
			InitializeComponent();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~SenderPage()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		System::ComponentModel::Container ^components;
		System::Windows::Forms::Panel^ Frame;
		System::Windows::Forms::Button^ BackButton;
		System::Windows::Forms::Label^ label1;
		System::Windows::Forms::Panel^ DragFilePanel;
		System::Action^ goBack;
		SOCKET clientSocket = INVALID_SOCKET;
		SOCKET serverSocket = INVALID_SOCKET;
		volatile bool isActive = true;
		System::Windows::Forms::Label^ label2;
		System::Windows::Forms::ProgressBar^ progressBar1;
		String^ fPath = nullptr;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->BackButton = (gcnew System::Windows::Forms::Button());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->DragFilePanel = (gcnew System::Windows::Forms::Panel());
			this->progressBar1 = (gcnew System::Windows::Forms::ProgressBar());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->DragFilePanel->SuspendLayout();
			this->SuspendLayout();
			// 
			// BackButton
			// 
			this->BackButton->Location = System::Drawing::Point(16, 250);
			this->BackButton->Name = L"BackButton";
			this->BackButton->Size = System::Drawing::Size(244, 23);
			this->BackButton->TabIndex = 2;
			this->BackButton->Text = L"Назад";
			this->BackButton->UseVisualStyleBackColor = true;
			this->BackButton->Click += gcnew System::EventHandler(this, &SenderPage::BackButton_Click);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(3, 0);
			this->label1->Size = System::Drawing::Size(193, 13);
			this->label1->Text = L"Запрос отправлен, ожидание ответа";
			// 
			// DragFilePanel
			// 
			this->DragFilePanel->Enabled = false;
			this->DragFilePanel->Location = System::Drawing::Point(0, 16);
			this->DragFilePanel->Size = System::Drawing::Size(276, 228);
			this->DragFilePanel->Click += gcnew System::EventHandler(this, &SenderPage::DragFilePanel_Click);
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
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(74, 13);
			this->label2->TabIndex = 1;
			this->label2->Text = L"Отправление";
			this->label2->Visible = false;
			// 
			// SenderPage
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->Controls->Add(this->DragFilePanel);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->BackButton);
			this->Controls->Add(this->progressBar1);
			this->Controls->Add(this->label2);
			this->Name = L"SenderPage";
			this->Size = System::Drawing::Size(276, 276);
			this->Load += gcnew System::EventHandler(this, &SenderPage::WaitingResponseHandle);
			this->DragFilePanel->ResumeLayout(false);
			this->DragFilePanel->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: int chooseBufferSize(uint64_t fileSize) {
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
		isActive = false;
		if (clientSocket != INVALID_SOCKET) {
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;
		}
		if (serverSocket != INVALID_SOCKET) {
			closesocket(serverSocket);
			serverSocket = INVALID_SOCKET;
		}
		for each (Control ^ ctrl in this->Controls) {
			this->Controls->Remove(ctrl);
		}
		
		goBack();
	}
	private: System::Void WaitingResponseHandle(System::Object^ sender, System::EventArgs^ e) {
		// Ожидаем подключение клиента-получателя
		Task::Run(gcnew Action(this, &SenderPage::WaitingResponseRoutine));
		
	}
	private: System::Void DragFilePanel_Click(System::Object^ sender, System::EventArgs^ e) {
		OpenFileDialog^ openFileDialog = gcnew OpenFileDialog();
		openFileDialog->Filter = "All files (*.*)|*.*";
		openFileDialog->Title = "Выберите файл";
		if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			fPath = openFileDialog->FileName;
		}
		
	}
	private: ULONG GetBroadcastForGatewayPrefix() {
		IP_ADAPTER_INFO adapter[16];
		DWORD buflen = sizeof(adapter);

		if (GetAdaptersInfo(adapter, &buflen) != NO_ERROR)
			return INADDR_NONE;

		IP_ADAPTER_INFO* p = adapter;

		while (p) {
			const char* ipStr = p->IpAddressList.IpAddress.String;
			const char* gwStr = p->GatewayList.IpAddress.String;

			if (gwStr[0] == '\0' || strcmp(gwStr, "0.0.0.0") == 0) {
				p = p->Next;
				continue;
			}

			char ipPrefix[16] = { 0 };
			char gwPrefix[16] = { 0 };

			int a, b;
			if (sscanf(ipStr, "%d.%d", &a, &b) == 2)
				sprintf_s(ipPrefix, "%d.%d", a, b);

			if (sscanf(gwStr, "%d.%d", &a, &b) == 2)
				sprintf_s(gwPrefix, "%d.%d", a, b);

			if (strcmp(ipPrefix, gwPrefix) == 0) {
				DWORD ip = inet_addr(ipStr);
				DWORD mask = inet_addr(p->IpAddressList.IpMask.String);
				DWORD broadcast = (ip & mask) | ~mask;
				return broadcast;
			}

			p = p->Next;
		}

		return INADDR_NONE;
	}

	private: void WaitingResponseRoutine() {
		
		clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (clientSocket == INVALID_SOCKET) {
			MessageBox::Show(L"UDP socket creation error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			
			return;
		}

		int broadcastEnable = 1;
		if (setsockopt(clientSocket,
			SOL_SOCKET,
			SO_BROADCAST,
			reinterpret_cast<char*>(&broadcastEnable),
			sizeof(broadcastEnable)) == SOCKET_ERROR)
		{
			MessageBox::Show(L"setsockopt(SO_BROADCAST) error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(clientSocket);
			
			return;
		}

		ULONG broadcast = GetBroadcastForGatewayPrefix();
		if (broadcast == INADDR_NONE) {
			MessageBox::Show(L"GetBroadcast() error", L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(clientSocket);
			
			return;
		}

		sockaddr_in broadcastAddr{};
		broadcastAddr.sin_family = AF_INET;
		broadcastAddr.sin_port = htons(12345);
		broadcastAddr.sin_addr.s_addr = broadcast;

		char hostname[256];
		if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
			MessageBox::Show(L"gethostname() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(clientSocket);
			
			return;
		}

		if (sendto(clientSocket,
			hostname,
			static_cast<int>(strlen(hostname)),
			0,
			reinterpret_cast<sockaddr*>(&broadcastAddr),
			sizeof(broadcastAddr)) == INVALID_SOCKET)
		{
			MessageBox::Show(L"sendto() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(clientSocket);
			
			return;
		}
		memset(hostname, 0, sizeof(hostname));
		sockaddr_in fromAddr{};
		int fromLen = sizeof(fromAddr);
		int bytes = 0;

		bytes = recvfrom(clientSocket,
			hostname,
			sizeof(hostname) - 1,
			0,
			reinterpret_cast<sockaddr*>(&fromAddr),
			&fromLen);

		if (bytes == SOCKET_ERROR) {
			if (!isActive) return;
			MessageBox::Show(L"recvfrom() hostname (Sender) error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(clientSocket);
			
			return;
		}
		hostname[bytes] = '\0';

		MessageBox::Show(L"Установлено соединение с " + gcnew System::String(hostname), "Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);

		// Подключение установлено. Переход к отправке файла
		Task::Run(gcnew Action(this, &SenderPage::SendFileRoutine));
	}
	private: void SendFileRoutine() {
		Thread::Sleep(200);

		sockaddr_in fromAddr{};
		int fromLen = sizeof(fromAddr);
		unsigned short port;
		int bytes = recvfrom(clientSocket,
			reinterpret_cast<char*>(&port),
			sizeof(port),
			0,
			reinterpret_cast<sockaddr*>(&fromAddr),
			(socklen_t*)&fromLen);
		if (bytes == SOCKET_ERROR) {
			if (!isActive) return;
			MessageBox::Show(L"recvfrom() port error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(clientSocket);
			
			return;
		}

		sockaddr_in tcpServerAddr{};
		tcpServerAddr.sin_family = AF_INET;
		tcpServerAddr.sin_port = htons(port);
		tcpServerAddr.sin_addr.s_addr = fromAddr.sin_addr.s_addr;

		serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (serverSocket == INVALID_SOCKET) {
			MessageBox::Show(L"TCP socket creation error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			
			return;
		}

		if (connect(serverSocket,
			reinterpret_cast<sockaddr*>(&tcpServerAddr),
			sizeof(tcpServerAddr)) == SOCKET_ERROR)
		{
			MessageBox::Show(L"Connect to TCP server error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			
			return;
		}

		this->DragFilePanel->Enabled = true;
		this->DragFilePanel->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
		this->label1->Text = L"Нажмите на поле и выберите файл";
		isActive = true;
		while (fPath == nullptr) continue;
		this->DragFilePanel->Enabled = false;
		this->DragFilePanel->BorderStyle = System::Windows::Forms::BorderStyle::None;

		char fName[512];
		msclr::interop::marshal_context ctx;
		strcpy_s(fName, ctx.marshal_as<const char*>(fPath));
		fName[sizeof(fName) - 1] = '\0';

		MessageBox::Show(L"Добавлен файл\n" + gcnew System::String(fName), "Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);

		std::ifstream sendFile(fName, std::ios::binary);
		if (!sendFile) {
			MessageBox::Show(L"Error opening file for reading" , L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			
			return;
		}

		sendFile.seekg(0, std::ios::end);
		uint64_t fileSize = static_cast<uint64_t>(sendFile.tellg());
		sendFile.seekg(0, std::ios::beg);

		if (send(serverSocket, reinterpret_cast<const char*>(&fileSize), sizeof(fileSize), 0) == SOCKET_ERROR) {
			MessageBox::Show(L"send() fileSize error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			
			return;
		}

		if (send(serverSocket, fName, static_cast<int>(strlen(fName)) + 1, 0) == SOCKET_ERROR) {
			MessageBox::Show(L"send() filename error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(serverSocket);
			
			return;
		}
		this->Invoke(gcnew Action(this, &SenderPage::ShowProgress));
		auto bufferSize = chooseBufferSize(fileSize);
		char* buffer = (char*)malloc(bufferSize);
		int64_t totalSent = 0, lastProgress = -1;
		while (!sendFile.eof())
		{
			sendFile.read(buffer, bufferSize);
			std::streamsize bytesRead = sendFile.gcount();
			if (bytesRead > 0)
			{
				int sentBytes = send(serverSocket, buffer, static_cast<int>(bufferSize), 0);
				if (sentBytes == SOCKET_ERROR) {
					if (!isActive) {
						free(buffer);
						return;
					}
					MessageBox::Show(L"send() buffer failed: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
					closesocket(serverSocket);
					
					free(buffer);
					sendFile.close();
					return;
				}
				totalSent += sentBytes;
				int progress = static_cast<int>((totalSent * 100) / fileSize);
				if (progress > lastProgress) {
					lastProgress = progress;
					this->BeginInvoke(gcnew Action<int>(this, &SenderPage::UpdateProgress), progress);
				}
			}
		}
		free(buffer);
		sendFile.close();
		closesocket(serverSocket);

		MessageBox::Show(L"Успешно! (отправитель)", "Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);

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
		DragFilePanel->Visible = false;
		label2->Visible = true;
		progressBar1->Visible = true;
	}
	};
}