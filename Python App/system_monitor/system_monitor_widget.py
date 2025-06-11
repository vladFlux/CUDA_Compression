from PySide6.QtWidgets import QWidget, QHBoxLayout
from .terminal_output_widget import TerminalOutputWidget
from .resources import ResourceMonitor
from .emitting_stream import EmittingStream

import sys


class SystemMonitorWidget(QWidget):
    def __init__(self):
        super().__init__()

        self.terminal_output = TerminalOutputWidget()
        self.resource_monitor = ResourceMonitor()

        self.init_output()

        layout = QHBoxLayout()

        layout.addWidget(self.terminal_output)
        layout.addWidget(self.resource_monitor)

        self.setLayout(layout)

    def init_output(self):
        sys.stdout = EmittingStream(self.terminal_output.append_text)
        sys.stderr = EmittingStream(self.terminal_output.append_text)