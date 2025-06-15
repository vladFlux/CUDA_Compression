from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QWidget, QPushButton, QHBoxLayout, QSizePolicy, QSpacerItem
from PySide6.QtCore import QSize


class ControlButtonsWidget(QWidget):
    def __init__(self):
        super().__init__()

        self.algorithm_button = QPushButton("Run algorithm")
        self.comparison_button = QPushButton("Start comparison")
        self.clear_button = QPushButton("Clear terminal")

        self.init_button_icons()

        layout = QHBoxLayout()

        for button in [self.algorithm_button, self.comparison_button, self.clear_button]:
            button.setMinimumHeight(45)
            button.setMinimumWidth(150)
            button.setIconSize(QSize(24, 24))
            button.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)

        layout.addWidget(self.algorithm_button)
        layout.addWidget(self.comparison_button)
        layout.addItem(QSpacerItem(5, 20, QSizePolicy.Expanding))
        layout.addWidget(self.clear_button)

        self.setLayout(layout)

    def enable_button(self):
        self.algorithm_button.setEnabled(True)
        self.algorithm_button.setText("Run Algorithm")
        self.comparison_button.setEnabled(True)
        self.comparison_button.setText("Start comparison")

    def init_button_icons(self):
        icon = QIcon("./assets/icons/run_algo.svg")
        self.algorithm_button.setIcon(icon)

        icon = QIcon("./assets/icons/comparison.svg")
        self.comparison_button.setIcon(icon)

        icon = QIcon("./assets/icons/clear.svg")
        self.clear_button.setIcon(icon)
