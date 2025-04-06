#pragma once
#include "ReceiverPage.h"
#include "SenderPage.h"

namespace FileSharing {

	public ref class MainPage : public System::Windows::Forms::UserControl
	{
	public:
		MainPage(System::Windows::Forms::Panel^ Frame)
		{
			this->Frame = Frame;
			InitializeComponent();
		}

	protected:
		~MainPage()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		System::Windows::Forms::Panel^ Frame;
		System::ComponentModel::Container^ components;
		System::Windows::Forms::Button^ ReceiveButton;
		System::Windows::Forms::PictureBox^ pictureBox1;
		System::Windows::Forms::Button^ SendButton;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(MainPage::typeid));
			this->ReceiveButton = (gcnew System::Windows::Forms::Button());
			this->SendButton = (gcnew System::Windows::Forms::Button());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
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
			// pictureBox1
			// 
			this->pictureBox1->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pictureBox1.Image")));
			this->pictureBox1->Location = System::Drawing::Point(4, 4);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(30, 30);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::Zoom;
			this->pictureBox1->TabIndex = 0;
			this->pictureBox1->TabStop = false;
			this->pictureBox1->Click += gcnew System::EventHandler(this, &MainPage::pictureBox1_Click);
			// 
			// MainPage
			// 
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->SendButton);
			this->Controls->Add(this->ReceiveButton);
			this->Name = L"MainPage";
			this->Size = System::Drawing::Size(276, 276);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
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
	private: System::Void pictureBox1_Click(System::Object^ sender, System::EventArgs^ e) {
		this->Frame->Enabled = false;
		MessageBox::Show(L"Это приложение позволяет обмениваться файлами между устройствами в локальной сети. \n" +
			"Выберите сценарий взаимодействия с устройством", 
			"Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);
		this->Frame->Enabled = true;
	}
	};
}
