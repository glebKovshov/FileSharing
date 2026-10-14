#include "mainwindow.h"
#include "core/protocol.h"
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace {
const char *Style = R"(
QWidget { background: #f4f7fb; color: #172c42; font-size: 14px; }
QLabel { background: transparent; }
QLabel#brand { font-size: 23px; font-weight: 700; }
QLabel#title { font-size: 28px; font-weight: 700; }
QLabel#section { font-size: 18px; font-weight: 600; }
QLabel#muted { color: #64758b; }
QLabel#eyebrow { color: #008b7a; font-size: 12px; font-weight: 700; }
QLabel#badge { background: #dff5ed; color: #087b61; border-radius: 14px; padding: 7px 14px; }
QLabel#logo { background: #0b9d88; color: white; border-radius: 14px; font-size: 26px; font-weight: 700; }
QFrame#card { background: white; border: 1px solid #e1e8f0; border-radius: 18px; }
QFrame#drop { background: #f4fbf9; border: 2px dashed #a5d8ce; border-radius: 16px; }
QFrame#drop[dragging="true"] { background: #d9f5ed; border-color: #0b9d88; }
QPushButton { background: white; border: 1px solid #d9e2ec; border-radius: 10px; padding: 11px 18px; font-weight: 600; }
QPushButton:hover { background: #edf5f4; border-color: #8dc5b9; }
QPushButton:pressed { background: #d5eee7; }
QPushButton:disabled { background: #edf1f5; color: #91a0b1; border-color: #e4e9ef; }
QPushButton#primary { background: #0b9d88; color: white; border: 1px solid #0b9d88; }
QPushButton#primary:hover { background: #078875; }
QPushButton#primary:disabled { background: #a2d5ca; color: white; border-color: #a2d5ca; }
QPushButton#tab { background: transparent; border: 0; color: #68798f; padding: 12px 28px; }
QPushButton#tab:checked { background: #172c42; color: white; }
QPushButton#danger { color: #b64c4c; }
QListWidget { background: white; border: 1px solid #e1e8f0; border-radius: 12px; padding: 6px; outline: 0; }
QListWidget::item { border-radius: 8px; padding: 13px; margin: 3px; }
QListWidget::item:selected { background: #e3f5ee; color: #172c42; }
QListWidget::item:hover { background: #f2f8f5; }
QProgressBar { border: 0; background: #e7edf3; border-radius: 5px; min-height: 10px; max-height: 10px; }
QProgressBar::chunk { border-radius: 5px; background: #0b9d88; }
QMessageBox { background: white; }
QToolTip { background: #172c42; color: white; border: 0; padding: 6px; }
)";
QString humanSize(double amount) {
    const QStringList units{QStringLiteral("Б"), QStringLiteral("КБ"), QStringLiteral("МБ"), QStringLiteral("ГБ"), QStringLiteral("ТБ"), QStringLiteral("ПБ")};
    int unit = 0;
    while (amount >= 1024 && unit < units.size() - 1) { amount /= 1024; ++unit; }
    return QString::number(amount, 'f', unit ? 1 : 0) + " " + units[unit];
}
QLabel *label(const QString &text, const QString &role = {}) {
    auto *widget = new QLabel(text);
    widget->setTextFormat(Qt::PlainText);
    widget->setObjectName(role);
    return widget;
}
QPushButton *button(const QString &text, const QString &role = {}) {
    auto *widget = new QPushButton(text);
    widget->setCursor(Qt::PointingHandCursor);
    widget->setObjectName(role);
    return widget;
}
QFrame *card(QVBoxLayout *&layout) {
    auto *widget = new QFrame;
    widget->setObjectName("card");
    layout = new QVBoxLayout(widget);
    layout->setContentsMargins(26, 24, 26, 24);
    layout->setSpacing(16);
    return widget;
}
QString droppedPath(const QMimeData *mime) {
    const auto urls = mime->urls();
    if (urls.size() != 1 || !urls[0].isLocalFile()) return {};
    const auto path = urls[0].toLocalFile();
    return QFileInfo(path).isFile() ? path : QString();
}
}
DropZone::DropZone(QWidget *parent) : QFrame(parent) {
    setObjectName("drop");
    setAcceptDrops(true);
    setMinimumHeight(175);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    auto *arrow = label(QStringLiteral("↑"));
    arrow->setStyleSheet("font-size: 38px; color: #0b9d88;");
    title = label(tr("Перетащите файл сюда"), "section");
    title->setWordWrap(true);
    detail = label(tr("или выберите его на компьютере"), "muted");
    for (auto *widget : {arrow, title, detail}) { widget->setAlignment(Qt::AlignCenter); layout->addWidget(widget); }
}
void DropZone::setFile(const QString &name, qint64 size) {
    title->setText(name);
    detail->setText(humanSize(size) + tr("  ·  Готов к отправке"));
}
void DropZone::highlight(bool enabled) {
    setProperty("dragging", enabled);
    style()->unpolish(this); style()->polish(this);
}
void DropZone::dragEnterEvent(QDragEnterEvent *event) {
    if (isEnabled() && !droppedPath(event->mimeData()).isEmpty()) { highlight(true); event->acceptProposedAction(); }
}
void DropZone::dragLeaveEvent(QDragLeaveEvent *event) { highlight(false); event->accept(); }
void DropZone::dropEvent(QDropEvent *event) {
    highlight(false);
    const auto path = droppedPath(event->mimeData());
    if (isEnabled() && !path.isEmpty()) { emit fileDropped(path); event->acceptProposedAction(); }
}

MainWindow::MainWindow(QWidget *parent, SharingService *provided) : QMainWindow(parent), service(provided ? provided : new SharingService(this)) {
    setWindowTitle(tr("FileSharing — файлы в локальной сети"));
    resize(980, 790);
    setMinimumSize(760, 720);
    setStyleSheet(QString::fromLatin1(Style));
    auto downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloads.isEmpty()) downloads = QDir::homePath();
    directory = settings.value("downloadDirectory", QDir(downloads).filePath("FileSharing")).toString();
    auto *central = new QWidget;
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(36, 28, 36, 24);
    root->setSpacing(20);
    auto *header = new QHBoxLayout;
    auto *logo = label("F", "logo");
    logo->setFixedSize(48, 48); logo->setAlignment(Qt::AlignCenter);
    header->addWidget(logo);
    auto *brand = new QVBoxLayout;
    brand->setSpacing(3);
    brand->addWidget(label("FileSharing", "brand"));
    brand->addWidget(label(tr("Файлы рядом. Всё просто."), "muted"));
    header->addLayout(brand); header->addStretch();
    header->addWidget(label(tr("●  Локальная сеть"), "badge"));
    root->addLayout(header);
    auto *navigation = new QHBoxLayout;
    sendTab = button(tr("Отправить"), "tab"); receiveTab = button(tr("Получить"), "tab");
    for (auto *tab : {sendTab, receiveTab}) { tab->setCheckable(true); navigation->addWidget(tab); }
    navigation->addStretch(); navigation->addWidget(label(tr("Без аккаунтов и облака"), "muted"));
    root->addLayout(navigation);
    pages = new QStackedWidget;
    root->addWidget(pages, 1);
    buildSend(); buildReceive();
    connect(sendTab, &QPushButton::clicked, this, [this] { setMode(0); });
    connect(receiveTab, &QPushButton::clicked, this, [this] { setMode(1); });
    QVBoxLayout *statusLayout;
    auto *statusCard = card(statusLayout);
    auto *row = new QHBoxLayout;
    statusTitle = label(tr("Готово к передаче"), "section");
    row->addWidget(statusTitle, 1);
    cancelButton = button(tr("Отменить"), "danger"); cancelButton->hide();
    openButton = button(tr("Открыть папку")); openButton->hide();
    connect(cancelButton, &QPushButton::clicked, this, [this] { if (current && !current->isDone()) current->cancel(); });
    connect(openButton, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(lastSaved.isEmpty() ? directory : QFileInfo(lastSaved).absolutePath()));
    });
    row->addWidget(openButton); row->addWidget(cancelButton); statusLayout->addLayout(row);
    statusDetail = label(tr("Передавайте файлы напрямую между компьютерами."), "muted");
    statusDetail->setWordWrap(true); statusLayout->addWidget(statusDetail);
    progress = new QProgressBar; progress->setRange(0, 100); progress->setTextVisible(false); progress->hide();
    statusLayout->addWidget(progress); root->addWidget(statusCard);
    auto *footer = label(tr("Оба устройства должны быть в одной сети Wi-Fi или Ethernet."), "muted");
    footer->setAlignment(Qt::AlignCenter); root->addWidget(footer);
    connect(service, &SharingService::peersChanged, this, &MainWindow::updatePeers);
    connect(service, &SharingService::sessionStarted, this, &MainWindow::startSession);
    connect(service, &SharingService::offerChanged, this, &MainWindow::offerChanged);
    connect(service, &SharingService::error, this, &MainWindow::showError);
    progressTimer.setInterval(100);
    connect(&progressTimer, &QTimer::timeout, this, &MainWindow::renderProgress);
    setMode(0);
}
MainWindow::~MainWindow() {
    disconnect(service, nullptr, this, nullptr);
    if (current) disconnect(current, nullptr, this, nullptr);
    service->stop();
}
void MainWindow::buildSend() {
    QVBoxLayout *layout;
    auto *page = card(layout);
    layout->addWidget(label(tr("01  /  ОТПРАВКА"), "eyebrow"));
    layout->addWidget(label(tr("Поделитесь одним файлом"), "title"));
    auto *hint = label(tr("Выберите файл и сделайте его доступным. На другом устройстве откройте «Получить»."), "muted");
    hint->setWordWrap(true); layout->addWidget(hint);
    drop = new DropZone; connect(drop, &DropZone::fileDropped, this, &MainWindow::selectFile);
    layout->addWidget(drop, 1);
    auto *row = new QHBoxLayout;
    chooseButton = button(tr("Выбрать файл"));
    connect(chooseButton, &QPushButton::clicked, this, [this] {
        const auto selected = QFileDialog::getOpenFileName(this, tr("Выберите файл для отправки"));
        if (!selected.isEmpty()) selectFile(selected);
    });
    shareButton = button(tr("Начать раздачу"), "primary");
    connect(shareButton, &QPushButton::clicked, this, &MainWindow::shareFile);
    stopButton = button(tr("Остановить раздачу")); stopButton->hide();
    connect(stopButton, &QPushButton::clicked, service, &SharingService::clearOffer);
    row->addWidget(chooseButton); row->addStretch(); row->addWidget(stopButton); row->addWidget(shareButton);
    layout->addLayout(row);
    offerStatus = label(tr("Файл доступен только после начала раздачи."), "muted");
    offerStatus->setWordWrap(true); layout->addWidget(offerStatus);
    pages->addWidget(page);
}
void MainWindow::buildReceive() {
    QVBoxLayout *layout;
    auto *page = card(layout);
    layout->addWidget(label(tr("02  /  ПОЛУЧЕНИЕ"), "eyebrow"));
    layout->addWidget(label(tr("Найдите устройство рядом"), "title"));
    layout->addWidget(label(tr("Выберите отправителя и подтвердите приём файла."), "muted"));
    auto *row = new QHBoxLayout;
    folderLabel = label(directory, "muted"); folderLabel->setWordWrap(true); folderLabel->setToolTip(directory);
    row->addWidget(folderLabel, 1);
    folderButton = button(tr("Папка сохранения"));
    connect(folderButton, &QPushButton::clicked, this, [this] {
        const auto selected = QFileDialog::getExistingDirectory(this, tr("Куда сохранять файлы"), directory);
        if (!selected.isEmpty()) { directory = selected; settings.setValue("downloadDirectory", directory); folderLabel->setText(directory); folderLabel->setToolTip(directory); }
    });
    row->addWidget(folderButton); layout->addLayout(row);
    empty = label(tr("Ищем устройства…\nНа другом компьютере выберите файл и нажмите «Начать раздачу»."), "muted");
    empty->setWordWrap(true); empty->setAlignment(Qt::AlignCenter); layout->addWidget(empty, 1);
    devices = new QListWidget; devices->setAccessibleName(tr("Доступные отправители"));
    connect(devices, &QListWidget::itemSelectionChanged, this, [this] { receiveButton->setEnabled(devices->currentItem() && !service->busy()); });
    layout->addWidget(devices, 1);
    row = new QHBoxLayout;
    refreshButton = button(tr("Обновить поиск"));
    connect(refreshButton, &QPushButton::clicked, service, [this] { service->search(); });
    receiveButton = button(tr("Получить файл"), "primary");
    connect(receiveButton, &QPushButton::clicked, this, &MainWindow::receiveFile);
    row->addWidget(refreshButton); row->addStretch(); row->addWidget(receiveButton); layout->addLayout(row);
    pages->addWidget(page);
}
void MainWindow::setMode(int index) {
    if (service->busy() || index < 0 || index > 1) return;
    pages->setCurrentIndex(index); sendTab->setChecked(index == 0); receiveTab->setChecked(index == 1);
    if (index == 1) service->clearOffer();
    service->search(index == 1);
    updateControls();
}
void MainWindow::selectFile(const QString &filePath) {
    if (service->busy()) return;
    try {
        const QFileInfo info(filePath);
        Protocol::fileName(info.fileName());
        if (!info.isFile()) throw Protocol::Error(tr("Выберите обычный файл"));
        const auto size = Protocol::fileSize(QJsonValue(info.size()));
        service->clearOffer();
        path = info.absoluteFilePath(); drop->setFile(info.fileName(), size); drop->setToolTip(path);
        statusTitle->setText(tr("Файл выбран"));
        statusDetail->setText(tr("Нажмите «Начать раздачу», чтобы другой компьютер увидел файл."));
        openButton->hide(); progress->hide(); updateControls();
    } catch (const std::exception &error) { showError(QString::fromUtf8(error.what())); }
}
void MainWindow::shareFile() {
    if (path.isEmpty()) return;
    try { service->setOffer(path); }
    catch (const std::exception &error) { showError(QString::fromUtf8(error.what())); }
}
void MainWindow::offerChanged(bool enabled) {
    offerStatus->setText(enabled ? tr("Файл доступен. На другом устройстве откройте «Получить» и выберите этот компьютер.") : tr("Файл доступен только после начала раздачи."));
    stopButton->setVisible(enabled);
    if (enabled) { statusTitle->setText(tr("Ожидаем получателя")); statusDetail->setText(tr("Можно остановить раздачу в любой момент.")); }
    else if (!current && statusTitle->text() == tr("Ожидаем получателя")) { statusTitle->setText(tr("Раздача остановлена")); statusDetail->setText(tr("Файл больше не доступен другим устройствам.")); }
    updateControls();
}
void MainWindow::updatePeers(const QList<Peer> &list) {
    const auto selected = devices->currentItem();
    const auto identity = selected ? selected->data(Qt::UserRole).value<Peer>().id : QString();
    devices->clear();
    auto peers = list;
    std::sort(peers.begin(), peers.end(), [](const Peer &a, const Peer &b) { return a.device.toCaseFolded() < b.device.toCaseFolded(); });
    for (const auto &peer : peers) {
        auto *item = new QListWidgetItem(QString("%1   ·   %2\n%3   ·   %4").arg(peer.device, peer.address, peer.name, humanSize(peer.size)));
        item->setData(Qt::UserRole, QVariant::fromValue(peer)); item->setToolTip(peer.name + "\n" + peer.address);
        devices->addItem(item);
        if (peer.id == identity) devices->setCurrentItem(item);
    }
    empty->setVisible(peers.isEmpty()); devices->setVisible(!peers.isEmpty());
    receiveButton->setEnabled(devices->currentItem() && !service->busy());
}
void MainWindow::receiveFile() {
    if (!devices->currentItem() || service->busy()) return;
    const auto peer = devices->currentItem()->data(Qt::UserRole).value<Peer>();
    QMessageBox dialog(this);
    dialog.setWindowTitle(tr("Подтвердите приём")); dialog.setTextFormat(Qt::PlainText);
    dialog.setText(tr("Получить «%1» (%2) от %3?").arg(peer.name, humanSize(peer.size), peer.device));
    dialog.setInformativeText(tr("Папка: %1\nСуществующие файлы останутся на месте.").arg(directory));
    auto *accept = dialog.addButton(tr("Получить"), QMessageBox::AcceptRole);
    dialog.addButton(tr("Отмена"), QMessageBox::RejectRole);
    dialog.exec();
    if (dialog.clickedButton() == accept) {
        try { service->receive(peer, directory); }
        catch (const std::exception &error) { showError(QString::fromUtf8(error.what())); }
    }
}
void MainWindow::updateControls() {
    const bool busy = service->busy();
    const QList<QWidget *> controls{sendTab, receiveTab, chooseButton, folderButton, refreshButton, drop, devices, stopButton};
    for (auto *widget : controls) widget->setEnabled(!busy);
    shareButton->setEnabled(!path.isEmpty() && !service->offering() && !busy && service->networkReady());
    receiveButton->setEnabled(devices->currentItem() && !busy);
}
void MainWindow::startSession(Transfer *transfer, bool isSending) {
    current = transfer; sending = isSending; count = total = 0; elapsed.start();
    connect(transfer, &Transfer::progress, this, [this](qint64 bytes, qint64 length) { count = bytes; total = length; });
    connect(transfer, &Transfer::finished, this, &MainWindow::transferFinished);
    connect(transfer, &Transfer::failed, this, &MainWindow::transferFailed);
    statusTitle->setText(sending ? tr("Отправляем файл…") : tr("Получаем файл…"));
    statusDetail->setText(tr("Подключение к устройству…"));
    progress->setValue(0); progress->show(); cancelButton->show(); openButton->hide();
    progressTimer.start(); updateControls();
}
void MainWindow::renderProgress() {
    const int percent = total ? static_cast<int>(static_cast<double>(count) * 100 / total) : 0;
    progress->setValue(percent);
    const auto speed = count / qMax(elapsed.elapsed() / 1000.0, .01);
    const auto eta = speed > 0 ? static_cast<qint64>((total - count) / speed) : 0;
    if (count == total && total) statusDetail->setText(tr("Данные переданы. Проверяем целостность и сохранение…"));
    else statusDetail->setText(tr("%1 из %2  ·  %3/с  ·  осталось ≈ %4 с").arg(humanSize(count), humanSize(total), humanSize(speed), QString::number(eta)));
}
void MainWindow::endSession() {
    current = nullptr; progressTimer.stop(); cancelButton->hide(); service->clearOffer(); updateControls();
}
void MainWindow::transferFinished(const QString &result) {
    endSession(); progress->setValue(100);
    statusTitle->setText(sending ? tr("Файл отправлен") : tr("Файл сохранён"));
    statusDetail->setText(sending ? tr("Получатель подтвердил целостность файла.") : result);
    if (!sending) { lastSaved = result; openButton->show(); }
}
void MainWindow::transferFailed(const QString &reason) { endSession(); progress->hide(); showError(reason); }
void MainWindow::showError(const QString &reason) {
    statusTitle->setText(reason == tr("Передача отменена") ? tr("Передача отменена") : tr("Не удалось завершить действие"));
    statusDetail->setText(reason + tr("\nПроверьте подключение и повторите попытку.")); openButton->hide();
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (service->busy() && QMessageBox::question(this, tr("Передача ещё идёт"), tr("Отменить передачу и закрыть приложение?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        event->ignore(); return;
    }
    service->stop(); event->accept();
}
