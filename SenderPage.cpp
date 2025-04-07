#include "SenderPage.h"

using namespace FileSharing;

int SenderPage::chooseBufferSize(uint64_t fileSize) {
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

System::Void SenderPage::BackButton_Click(System::Object^ sender, System::EventArgs^ e) {
	isActive = false;
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}
	if (serverSocket != INVALID_SOCKET) {
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}
	this->Controls->Clear();
	goBack();
}

System::Void SenderPage::WaitingResponseHandle(System::Object^ sender, System::EventArgs^ e) {
	Task::Run(gcnew Action(this, &SenderPage::WaitingResponseRoutine));
}

System::Void SenderPage::DragFilePanel_Click(System::Object^ sender, System::EventArgs^ e) {
	OpenFileDialog^ openFileDialog = gcnew OpenFileDialog();
	openFileDialog->Filter = "All files (*.*)|*.*";
	openFileDialog->Title = "Выберите файл";
	if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
		fPath = openFileDialog->FileName;
	}
}

ULONG SenderPage::GetBroadcastByGateway() {
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

void SenderPage::WaitingResponseRoutine() {

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

	ULONG broadcast = GetBroadcastByGateway();
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

void SenderPage::SendFileRoutine() {
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
		MessageBox::Show(L"Error opening file for reading", L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
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

void SenderPage::UpdateProgress(int percent) {
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;
	progressBar1->Value = percent;
}

void SenderPage::ShowProgress() {
	DragFilePanel->Visible = false;
	label2->Visible = true;
	progressBar1->Visible = true;
}