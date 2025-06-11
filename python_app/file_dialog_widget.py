from PySide6 import QtWidgets
import os


class FileDialogWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.layout = QtWidgets.QVBoxLayout()

        # File path placeholders
        self.input_file = None
        self.output_file = None

        self.input_line_edit = QtWidgets.QLineEdit(self)
        self.input_button = QtWidgets.QPushButton("Browse")

        self.output_line_edit = QtWidgets.QLineEdit(self)
        self.output_button = QtWidgets.QPushButton("Browse")

        # Initialize and add input/output groups
        self.input_group = self.init_input_group()
        self.output_group = self.init_output_group()
        self.layout.addWidget(self.input_group)
        self.layout.addWidget(self.output_group)

        self.setLayout(self.layout)

    def init_input_group(self):
        group = QtWidgets.QGroupBox("Input File")
        group.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Maximum)

        layout = QtWidgets.QHBoxLayout()
        layout.setContentsMargins(8, 4, 8, 4)
        layout.setSpacing(4)

        self.input_line_edit.setMinimumWidth(500)

        layout.addWidget(self.input_line_edit)
        layout.addWidget(self.input_button)

        self.input_button.clicked.connect(self.select_input_file)
        self.input_line_edit.textChanged.connect(self.update_input_file_from_line_edit)

        group.setLayout(layout)
        return group

    def init_output_group(self):
        group = QtWidgets.QGroupBox("Output File")
        group.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Maximum)

        layout = QtWidgets.QHBoxLayout()
        layout.setContentsMargins(8, 4, 8, 4)
        layout.setSpacing(4)

        self.output_line_edit.setMinimumWidth(500)

        layout.addWidget(self.output_line_edit)
        layout.addWidget(self.output_button)

        self.output_button.clicked.connect(self.select_output_file)
        self.output_line_edit.textChanged.connect(self.update_output_file_from_line_edit)

        group.setLayout(layout)
        return group

    def select_input_file(self):
        file, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select Input File", "",
                                                        "All Files (*);;Text Files (*.txt)")
        if file:
            self.input_file = file
            self.input_line_edit.setText(self.input_file)

    def select_output_file(self):
        file, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Select Output File", "",
                                                        "All Files (*);;Text Files (*.txt)")
        if file:
            self.output_file = file
            self.output_line_edit.setText(self.output_file)

    def update_input_file_from_line_edit(self):
        text = self.input_line_edit.text()
        if os.path.exists(text):
            self.input_file = text

    def update_output_file_from_line_edit(self):
        text = self.output_line_edit.text()
        if text:
            self.output_file = text

    def get_input_file_path(self):
        return self.input_line_edit.text()

    def get_output_file_path(self):
        return self.output_line_edit.text()
