import sys

from PySide6.QtGui import QIcon
from PySide6.QtCore import QSize

import app_main

if __name__ == "__main__":
    application = app_main.QtWidgets.QApplication([])

    with open("./assets/style.qss", "r") as style_file:
        application.setStyleSheet(style_file.read())

    icon = QIcon("./assets/icons/cuda_icon.png")
    icon.addFile("./assets/icons/cuda_icon.png", QSize(64, 64))
    application.setWindowIcon(icon)

    widget = app_main.CompressionApp()
    widget.setWindowIcon(icon)
    widget.resize(800, 600)
    widget.show()

    sys.exit(application.exec())