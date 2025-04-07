#include "ReceiverPage.h"

using namespace FileSharing;

int ReceiverPage::chooseBufferSize(std::streampos fileSize) {
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

System::Void ReceiverPage::BackButton_Click(System::Object^ sender, System::EventArgs^ e) {
	
	isDiscovering = false;
	isReceiving = false;
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
	this->Controls->Clear();
	goBack();
}

System::Void ReceiverPage::FindDeviceHandle(System::Object^ sender, System::EventArgs^ e) {
	Task::Run(gcnew Action(this, &ReceiverPage::FindDeviceRoutine));
}

void ReceiverPage::FindDeviceRoutine()
{
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET) {
		MessageBox::Show(L"UDP socket creation error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);

		return;
	}

	sockaddr_in udpServerAddr{};
	udpServerAddr.sin_family = AF_INET;
	udpServerAddr.sin_port = htons(12345);
	udpServerAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&udpServerAddr), sizeof(udpServerAddr)) == SOCKET_ERROR) {
		MessageBox::Show(L"UDP bind error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(serverSocket);

		return;
	}

	const int hostnameSize = 256;
	char* buffer = (char*)malloc(hostnameSize);
	sockaddr_in clientAddr{};
	int clientLen = sizeof(clientAddr);

	int bytesReceived = 0;
	while (isDiscovering && (bytesReceived = recvfrom(
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

	if (bytesReceived == SOCKET_ERROR && isDiscovering) {
		if (!isDiscovering) {
			free(buffer);
			return;
		}
		MessageBox::Show(L"recvfrom() hostname (Receiver) error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(serverSocket);

		free(buffer);
		return;
	}
}

void ReceiverPage::AddDeviceButton(DeviceInfo^ di)
{
	Button^ btn = gcnew Button();
	btn->Tag = di->Addr;
	btn->Text = di->Name;
	btn->Size = Drawing::Size(270, 40);
	btn->Location = Drawing::Point(3, 3 + 50 * FindDevices->Controls->Count);

	btn->Click += gcnew EventHandler(this, &ReceiverPage::DeviceButton_Click);
	FindDevices->Controls->Add(btn);
}

System::Void ReceiverPage::DeviceButton_Click(System::Object^ sender, System::EventArgs^ e) {
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

	isDiscovering = false;
	deviceButton = btn;

	Clear();

	Task::Run(gcnew Action(this, &ReceiverPage::RecvFileRoutine));
}

System::Void ReceiverPage::RecvFileRoutine() {

	Thread::Sleep(200);

	sockaddr_in* clientAddr = static_cast<sockaddr_in*>(safe_cast<IntPtr>(deviceButton->Tag).ToPointer());

	char* buffer;
	int bufferSize = 256;
	buffer = (char*)malloc(bufferSize);
	memset(buffer, 0, bufferSize);
	if (gethostname(buffer, bufferSize) == SOCKET_ERROR) {
		MessageBox::Show(L"gethostname() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(serverSocket);

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
		if (!isReceiving) return;
		MessageBox::Show(L"sendto() hostname error (Receiver) " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(serverSocket);

		return;
	}

	tcpListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcpListenSock == INVALID_SOCKET) {
		MessageBox::Show(L"TCP listen socket creation error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(serverSocket);

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

		return;
	}

	int sentBytes = sendto(serverSocket, // server port
		reinterpret_cast<const char*>(&port),
		sizeof(port),
		0,
		reinterpret_cast<sockaddr*>(clientAddr),
		clientLen);
	if (sentBytes == SOCKET_ERROR) {
		if (!isReceiving) return;
		MessageBox::Show(L"sendto() port error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(serverSocket);
		closesocket(tcpListenSock);

		return;
	}
	this->label1->Text = L"Подключено. Ожидание файла";

	closesocket(serverSocket);
	free(clientAddr);
	clientAddr = nullptr;

	if (listen(tcpListenSock, 1) == SOCKET_ERROR) {
		MessageBox::Show(L"listen() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(tcpListenSock);

		return;
	}

	sockaddr_in tcpClientAddr{};
	int tcpClientAddrLen = sizeof(tcpClientAddr);
	tcpClientSock = accept(tcpListenSock,
		reinterpret_cast<sockaddr*>(&tcpClientAddr),
		(socklen_t*)&tcpClientAddrLen);
	if (tcpClientSock == INVALID_SOCKET) {
		if (!isReceiving) return;
		MessageBox::Show(L"accept() error: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		closesocket(tcpListenSock);

		return;
	}

	uint64_t fileSize;
	int totalBytes = 0;
	char* fileSizePtr = reinterpret_cast<char*>(&fileSize);
	while (totalBytes < sizeof(fileSize)) {
		int bytes = recv(tcpClientSock, fileSizePtr + totalBytes, sizeof(fileSize) - totalBytes, 0);
		if (bytes <= 0) {
			if (!isReceiving) return;
			MessageBox::Show(L"recv() error for fileSize: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(tcpClientSock);
			closesocket(tcpListenSock);

			return;
		}
		totalBytes += bytes;
	}

	char fPath[512];
	totalBytes = 0;
	while (totalBytes < sizeof(fPath) - 1) {
		int bytes = recv(tcpClientSock, fPath + totalBytes, 1, 0);
		if (bytes <= 0) {
			if (!isReceiving) return;
			MessageBox::Show(L"recv() error file name: " + Convert::ToString(WSAGetLastError()), L"Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
			closesocket(tcpClientSock);
			closesocket(tcpListenSock);

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


	label2->Visible = false;
	progressBar1->Visible = false;

	goBack();
}

void ReceiverPage::UpdateProgress(int percent) {
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;
	progressBar1->Value = percent;
}

void ReceiverPage::ShowProgress() {
	FindDevices->Visible = false;
	label2->Visible = true;
	progressBar1->Visible = true;
}

void ReceiverPage::Clear() {
	for each (Control ^ ctrl in FindDevices->Controls)
	{
		Button^ btn = dynamic_cast<Button^>(ctrl);
		if (btn && btn->Tag != nullptr)
		{
			IntPtr ptr = safe_cast<IntPtr>(btn->Tag);
			sockaddr_in* addr = static_cast<sockaddr_in*>(ptr.ToPointer());
			FindDevices->Controls->Remove(ctrl);
			if (btn != deviceButton) free(addr);
		}
	}
}