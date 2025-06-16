from PySide6.QtWidgets import QWidget, QHBoxLayout
from .compression_widget import CompressionSelectorsWidget
from .comparison_widget import ComparisonRunSettingsWidget

class OptionsPanelWidget(QWidget):
    def __init__(self):
        super().__init__()

        options_layout = QHBoxLayout()

        self.selectors = CompressionSelectorsWidget()
        self.comparison = ComparisonRunSettingsWidget()

        options_layout.addWidget(self.selectors)
        options_layout.addWidget(self.comparison)

        self.setLayout(options_layout)
