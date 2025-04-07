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
using namespace System::Threading::Tasks;
using namespace System::Threading;

namespace FileSharing {
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
	private: 
		int chooseBufferSize(uint64_t fileSize);
		System::Void BackButton_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void WaitingResponseHandle(System::Object^ sender, System::EventArgs^ e);
		System::Void DragFilePanel_Click(System::Object^ sender, System::EventArgs^ e);
		ULONG GetBroadcastByGateway();
		void WaitingResponseRoutine();
		void SendFileRoutine();
		void UpdateProgress(int percent);
		void ShowProgress();
	};
}