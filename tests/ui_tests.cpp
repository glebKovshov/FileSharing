#include "ui/mainwindow.h"
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

class UiTests : public QObject {
    Q_OBJECT
private:
    QPushButton *findButton(MainWindow &window, const QString &text) {
        for (auto *button : window.findChildren<QPushButton *>()) if (button->text() == text) return button;
        return nullptr;
    }
private slots:
    void firstLaunchAndShareControls() {
        QTemporaryDir folder;
        const auto path = folder.filePath(QString::fromUtf8("Пример.txt"));
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("hello"); file.close();
        SharingService service(nullptr, 0);
        MainWindow window(nullptr, &service);
        window.show();
        auto *share = findButton(window, QString::fromUtf8("Начать раздачу"));
        QVERIFY(share); QVERIFY(!share->isEnabled()); QVERIFY(!service.offering());
        window.selectFile(path);
        QVERIFY(share->isEnabled()); QVERIFY(!service.offering());
        QTest::mouseClick(share, Qt::LeftButton);
        QVERIFY(service.offering()); QVERIFY(!share->isEnabled());
        auto *stop = findButton(window, QString::fromUtf8("Остановить раздачу"));
        QVERIFY(stop->isVisible()); QTest::mouseClick(stop, Qt::LeftButton);
        QVERIFY(!service.offering()); QVERIFY(share->isEnabled());
        window.setMode(1);
        QVERIFY(!findButton(window, QString::fromUtf8("Получить файл"))->isEnabled());
        window.close();
    }
    void unsafeFileReportsReadableError() {
        SharingService service(nullptr, 0);
        MainWindow window(nullptr, &service);
        window.selectFile("/not/a/real/file");
        QVERIFY(!service.offering());
        bool readable = false;
        for (auto *label : window.findChildren<QLabel *>()) {
            QVERIFY(label->textFormat() == Qt::PlainText);
            if (label->text().contains(QString::fromUtf8("Выберите обычный файл"))) readable = true;
        }
        QVERIFY(readable);
    }
    void renderScreenshots() {
        const auto destination = qEnvironmentVariable("FS_SCREENSHOTS");
        if (destination.isEmpty()) QSKIP("Set FS_SCREENSHOTS to render UI previews");
        QDir().mkpath(destination);
        SharingService service(nullptr, 0);
        MainWindow window(nullptr, &service);
        window.show();
        QTest::qWait(50);
        QVERIFY(window.grab().save(QDir(destination).filePath("send.png")));
        window.setMode(1);
        QTest::qWait(50);
        QVERIFY(window.grab().save(QDir(destination).filePath("receive.png")));
        window.close();
    }
};
QTEST_MAIN(UiTests)
#include "ui_tests.moc"
