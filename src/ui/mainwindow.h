#pragma once
#include "core/sharingservice.h"
#include <QElapsedTimer>
#include <QMainWindow>
#include <QSettings>
#include <QFrame>
class QLabel;
class QPushButton;
class QListWidget;
class QStackedWidget;
class QProgressBar;

class DropZone : public QFrame {
    Q_OBJECT
public:
    explicit DropZone(QWidget *parent = nullptr);
    void setFile(const QString &name, qint64 size);
signals:
    void fileDropped(const QString &path);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    QLabel *title, *detail;
    void highlight(bool enabled);
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr, SharingService *service = nullptr);
    ~MainWindow() override;
public slots:
    void selectFile(const QString &path);
    void setMode(int index);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    SharingService *service;
    QSettings settings;
    QString directory, path, lastSaved;
    QPointer<Transfer> current;
    bool sending = false;
    QElapsedTimer elapsed;
    QTimer progressTimer;
    qint64 count = 0, total = 0;
    QStackedWidget *pages;
    DropZone *drop;
    QListWidget *devices;
    QProgressBar *progress;
    QLabel *offerStatus, *folderLabel, *empty, *statusTitle, *statusDetail;
    QPushButton *sendTab, *receiveTab, *chooseButton, *shareButton, *stopButton,
        *folderButton, *refreshButton, *receiveButton, *cancelButton, *openButton;
    void buildSend();
    void buildReceive();
    void updatePeers(const QList<Peer> &peers);
    void updateControls();
    void shareFile();
    void receiveFile();
    void offerChanged(bool enabled);
    void startSession(Transfer *transfer, bool isSending);
    void endSession();
    void renderProgress();
    void transferFinished(const QString &result);
    void transferFailed(const QString &reason);
    void showError(const QString &reason);
};
