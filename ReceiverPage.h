#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <msclr/marshal_cppstd.h>
#pragma comment(lib, "ws2_32.lib")

using namespace System::Windows::Forms;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace System::Drawing;
using namespace System::Collections::Generic;
using namespace System;
using namespace System::IO;

ref struct DeviceInfo
{
	String^ Name;
	IntPtr Addr;
};

namespace FileSharing {

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
		volatile bool isDiscovering = true;
		volatile bool isReceiving = true;
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
			this->FindDevices->PerformLayout();
		}
#pragma endregion
	private: 
		int chooseBufferSize(std::streampos fileSize);
		System::Void BackButton_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void FindDeviceHandle(System::Object^ sender, System::EventArgs^ e);
		void FindDeviceRoutine();
		void AddDeviceButton(DeviceInfo^ di);
		System::Void DeviceButton_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void RecvFileRoutine();
		void UpdateProgress(int percent);
		void ShowProgress();
		void Clear();
	};
}