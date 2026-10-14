#include "ui/mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QTimer>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("FileSharing");
    app.setApplicationName("FileSharing");
    app.setApplicationVersion("2.0.0");
    app.setStyle("Fusion");
    app.setWindowIcon(QIcon(":/icon.svg"));
    MainWindow window;
    window.show();
    if (app.arguments().contains("--smoke-test")) QTimer::singleShot(250, &app, &QApplication::quit);
    return app.exec();
}
