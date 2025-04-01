#pragma once
#include "ReceiverPage.h"
#include "SenderPage.h"

namespace FileSharing {

	/// <summary>
	/// Summary for MainPage
	/// </summary>
	public ref class MainPage : public System::Windows::Forms::UserControl
	{
	public:
		MainPage(System::Windows::Forms::Panel^ Frame)
		{
			this->Frame = Frame;
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MainPage()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::Windows::Forms::Panel^ Frame;
		System::ComponentModel::Container^ components;
		System::Windows::Forms::Button^ ReceiveButton;
		System::Windows::Forms::Button^ SendButton;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->ReceiveButton = (gcnew System::Windows::Forms::Button());
			this->SendButton = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// ReceiveButton
			// 
			this->ReceiveButton->Location = System::Drawing::Point(89, 56);
			this->ReceiveButton->Name = L"ReceiveButton";
			this->ReceiveButton->Size = System::Drawing::Size(97, 36);
			this->ReceiveButton->TabIndex = 0;
			this->ReceiveButton->Text = L"Получить";
			this->ReceiveButton->UseVisualStyleBackColor = true;
			this->ReceiveButton->Click += gcnew System::EventHandler(this, &MainPage::ReceiveButton_Click);
			// 
			// SendButton
			// 
			this->SendButton->Location = System::Drawing::Point(89, 169);
			this->SendButton->Name = L"SendButton";
			this->SendButton->Size = System::Drawing::Size(97, 36);
			this->SendButton->TabIndex = 1;
			this->SendButton->Text = L"Отправить";
			this->SendButton->UseVisualStyleBackColor = true;
			this->SendButton->Click += gcnew System::EventHandler(this, &MainPage::SendButton_Click);
			// 
			// MainPage
			// 
			this->Controls->Add(this->SendButton);
			this->Controls->Add(this->ReceiveButton);
			this->Name = L"MainPage";
			this->Size = System::Drawing::Size(276, 276);
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void ReceiveButton_Click(System::Object^ sender, System::EventArgs^ e) {
		Frame->Controls->Clear();
		Frame->Controls->Add(gcnew ReceiverPage(Frame, gcnew System::Action(this, &MainPage::ShowSelf)));
	}
	private: void ShowSelf()
	{
		Frame->Controls->Clear();
		Frame->Controls->Add(this);
	}
	private: System::Void SendButton_Click(System::Object^ sender, System::EventArgs^ e) {
		Frame->Controls->Clear();
		Frame->Controls->Add(gcnew SenderPage(Frame, gcnew System::Action(this, &MainPage::ShowSelf)));
	}
	};
}
