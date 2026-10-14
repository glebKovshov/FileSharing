import sys

from PySide6.QtCore import QCoreApplication
from PySide6.QtWidgets import QApplication, QLabel


def main():
    QCoreApplication.setOrganizationName("FileSharing")
    QCoreApplication.setApplicationName("FileSharing")
    app = QApplication(sys.argv)
    window = QLabel("FileSharing · Qt 6")
    window.resize(640, 400)
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
