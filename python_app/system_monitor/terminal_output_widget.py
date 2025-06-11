from PySide6 import QtWidgets, QtGui

class TerminalOutputWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.layout = QtWidgets.QVBoxLayout()

        self.output_box = QtWidgets.QTextEdit()
        self.output_box.setReadOnly(True)
        self.output_box.setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace;")
        self.output_box.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)

        self.layout.addWidget(self.output_box)
        self.setLayout(self.layout)

    def append_text(self, text):
        self.output_box.append(text)

    def clear(self):
        self.output_box.clear()
