from PySide6.QtWidgets import QMainWindow, QWidget
from PySide6 import QtWidgets
from PySide6.QtCore import QThread

from options_panel import OptionsPanelWidget

from file_dialog_widget import FileDialogWidget
from system_monitor.system_monitor_widget import SystemMonitorWidget
from algorithm_worker import AlgorithmWorker
from button_menu import ControlButtonsWidget


class CompressionApp(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("CUDA Compression")

        # Create central widget and attach layout
        central_widget = QWidget()
        self.main_layout = QtWidgets.QVBoxLayout(central_widget)

        # Options panel
        self.options_panel = OptionsPanelWidget()

        # File dialogs
        self.file_dialog_widget = FileDialogWidget()

        # Stats components
        self.system_monitor = SystemMonitorWidget()

        # Buttons component
        self.control_buttons = ControlButtonsWidget()

        # Add all components to main layout
        self.main_layout.addWidget(self.options_panel)
        self.main_layout.addWidget(self.file_dialog_widget)
        self.main_layout.addWidget(self.system_monitor)
        self.main_layout.addWidget(self.control_buttons)

        # Set central widget with layout
        self.setCentralWidget(central_widget)

        # Connect buttons from the button menus
        self.buttons_connect()

    def run_current_algorithm(self):
        cpu_select = self.options_panel.selectors.hardware_select.is_cpu_selected()
        cpu_flag = ""

        if cpu_select:
            cpu_flag = "cpu_"

        mode = self.options_panel.selectors.mode_select.get_compress_mode()
        command = [
            f"../build/{cpu_flag}huffman{mode}",
            self.file_dialog_widget.get_input_file_path(),
            self.file_dialog_widget.get_output_file_path()
        ]

        # Disable the button to prevent duplicate presses
        self.control_buttons.algorithm_button.setEnabled(False)
        self.control_buttons.algorithm_button.setText("Running")

        self.algo_thread = QThread()
        self.algo_worker = AlgorithmWorker(command)
        self.algo_worker.moveToThread(self.algo_thread)

        self.algo_worker.output_line.connect(self.system_monitor.terminal_output.append_text)

        # When the worker is done, re-enable the button
        self.algo_worker.finished.connect(self.control_buttons.enable_button)
        self.algo_worker.error.connect(self.control_buttons.enable_button)

        self.algo_worker.finished.connect(self.algo_thread.quit)
        self.algo_worker.finished.connect(self.algo_worker.deleteLater)
        self.algo_thread.finished.connect(self.algo_thread.deleteLater)

        self.algo_worker.error.connect(lambda msg: self.system_monitor.terminal_output.append_text(f"[ERROR] {msg}"))

        self.algo_thread.started.connect(self.algo_worker.run)
        self.algo_thread.start()

    def buttons_connect(self):
        self.control_buttons.clear_button.clicked.connect(self.system_monitor.terminal_output.clear)
        self.control_buttons.algorithm_button.clicked.connect(self.run_current_algorithm)
        #self.comparison_button

    def closeEvent(self, event):
        self.system_monitor.resource_monitor.close()
        super().closeEvent(event)
