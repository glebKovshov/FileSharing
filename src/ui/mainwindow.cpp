#include "mainwindow.h"
#include <QLabel>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("FileSharing");
    setCentralWidget(new QLabel("FileSharing · C++ / Qt 6"));
    resize(800, 600);
}
