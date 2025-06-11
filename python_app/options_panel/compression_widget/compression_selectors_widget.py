from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QGroupBox

from .mode_select import ModeSelectorWidget
from .hardware_select import HardwareSelectorWidget


class CompressionSelectorsWidget(QWidget):
    def __init__(self):
        super().__init__()

        # Create selectors
        self.mode_select = ModeSelectorWidget()
        self.hardware_select = HardwareSelectorWidget()

        # Outer layout for the widget
        outer_layout = QVBoxLayout()
        self.setLayout(outer_layout)

        # Group box for compression settings
        compression_groupbox = QGroupBox("Compression settings:")
        groupbox_layout = QVBoxLayout()
        compression_groupbox.setLayout(groupbox_layout)

        # Horizontal layout for mode and hardware
        top_row = QHBoxLayout()
        top_row.addWidget(self.mode_select)
        top_row.addWidget(self.hardware_select)

        # Assemble layouts
        groupbox_layout.addLayout(top_row)

        # Add group box to main layout
        outer_layout.addWidget(compression_groupbox)

    def get_compress_mode(self):
        return self.mode_select.get_compress_mode()
