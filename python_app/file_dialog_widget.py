from PySide6 import QtWidgets
from PySide6.QtGui import Qt, QPixmap
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QLabel

import os


class FileDialogWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        # Outer layout: Horizontal (to allow image or side widget)
        outer_layout = QtWidgets.QHBoxLayout()

        # Inner layout: your existing vertical layout
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

        # Optional: spacer below input/output
        self.layout.addStretch()

        # Wrap vertical layout in a QWidget so it can be added to outer HBox
        left_widget = QtWidgets.QWidget()
        left_widget.setLayout(self.layout)

        # Add left side (file dialogs) to outer layout
        outer_layout.addWidget(left_widget)


        self.image_label = QLabel()
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setMinimumSize(167, 165)

        self.original_pixmap = QPixmap("./assets/icons/cuda_icon.png")
        self.image_label.setPixmap(self.original_pixmap)

        outer_layout.addWidget(self.image_label)
        self.setLayout(outer_layout)

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

    def is_valid_output_path(self, path: str) -> bool:
        if not path:
            return False
        folder = os.path.dirname(path) or '.'  # if no dirname, assume current dir
        return os.path.isdir(folder) and os.access(folder, os.W_OK)

    def update_input_file_from_line_edit(self):
        text = self.input_line_edit.text()
        if os.path.exists(text):
            self.input_file = text
            self.input_line_edit.setStyleSheet("")
        else:
            self.input_file = None
            self.input_line_edit.setStyleSheet("border: 1px solid red;")

    def update_output_file_from_line_edit(self):
        text = self.output_line_edit.text()
        if self.is_valid_output_path(text):
            self.output_file = text
            self.output_line_edit.setStyleSheet("")
        else:
            self.output_file = None
            self.output_line_edit.setStyleSheet("border: 1px solid red;")


    def get_input_file_path(self):
        return self.input_line_edit.text()

    def get_output_file_path(self):
        return self.output_line_edit.text()


