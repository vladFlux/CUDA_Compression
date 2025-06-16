from PySide6.QtWidgets import (
    QWidget, QPushButton, QLabel, QSpinBox,
    QVBoxLayout, QHBoxLayout, QGroupBox
)

class ComparisonRunSettingsWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        # Group box with title
        group_box = QGroupBox("Comparison Settings")

        # Layouts
        group_layout = QVBoxLayout()
        top_row_layout = QHBoxLayout()

        # Label and spin box
        self.label = QLabel("Number of Runs")
        self.run_spin_box = QSpinBox()
        self.run_spin_box.setMinimum(1)
        self.run_spin_box.setMaximum(1000)
        self.run_spin_box.setValue(1)

        top_row_layout.addWidget(self.label)
        top_row_layout.addWidget(self.run_spin_box)

        # Button
        self.clear_data_button = QPushButton("Clear data")

        # Assemble group box
        group_layout.addLayout(top_row_layout)
        group_layout.addWidget(self.clear_data_button)
        group_box.setLayout(group_layout)

        # Outer layout
        outer_layout = QVBoxLayout()
        outer_layout.addWidget(group_box)
        self.setLayout(outer_layout)

    def get_run_count(self) -> int:
        return self.run_spin_box.value()
