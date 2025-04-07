#include "MainPage.h"

using namespace FileSharing;

System::Void MainPage::ReceiveButton_Click(System::Object^ sender, System::EventArgs^ e) {
	Frame->Controls->Clear();
	Frame->Controls->Add(gcnew ReceiverPage(Frame, gcnew System::Action(this, &MainPage::ShowSelf)));
}
void MainPage::ShowSelf()
{
	Frame->Controls->Clear();
	Frame->Controls->Add(this);
}
System::Void MainPage::SendButton_Click(System::Object ^ sender, System::EventArgs ^ e) {
	Frame->Controls->Clear();
	Frame->Controls->Add(gcnew SenderPage(Frame, gcnew System::Action(this, &MainPage::ShowSelf)));
}
System::Void MainPage::pictureBox1_Click(System::Object^ sender, System::EventArgs^ e) {
	this->Frame->Enabled = false;
	MessageBox::Show(L"Это приложение позволяет обмениваться файлами между устройствами в локальной сети. \n" +
		"Выберите сценарий взаимодействия с устройством",
		"Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);
	this->Frame->Enabled = true;
}
