import os.path

from PySide6.QtWidgets import QMainWindow, QWidget
from PySide6 import QtWidgets
from PySide6.QtCore import Qt, QThread, QTimer

from options_panel import OptionsPanelWidget

from file_dialog_widget import FileDialogWidget
from system_monitor.system_monitor_widget import SystemMonitorWidget
from algorithm_worker import AlgorithmWorker
from button_menu import ControlButtonsWidget
from graph_viewer import ExecutionTimeChartPopup


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

        # Dict for comparison data
        self.execution_data = {}
        self.running_jobs = []
        self.comparison_runs = 1

    def run_current_algorithm(self):
        # Build command with flags
        input_path = self.file_dialog_widget.get_input_file_path()

        if not os.path.isfile(input_path) or input_path == "":
            print("Invalid input path!")
            return

        output_path = self.file_dialog_widget.get_output_file_path()

        if output_path == "":
            print("Output requires a path or file name!")
            return

        mode = self.options_panel.selectors.mode_select.get_compress_mode()
        cpu_select = self.options_panel.selectors.hardware_select.is_cpu_selected()
        cpu_flag = ""

        if cpu_select:
            cpu_flag = "cpu_"

        command = [f"../build/{cpu_flag}huffman{mode}", input_path, output_path]

        # Disable the buttons to prevent duplicate presses
        self.control_buttons.algorithm_button.setEnabled(False)
        self.control_buttons.algorithm_button.setText("Running...")
        self.control_buttons.comparison_button.setEnabled(False)
        self.control_buttons.comparison_button.setText("Running...")

        # Create thread and worker
        self.algo_thread = QThread()
        self.algo_worker = AlgorithmWorker(command)

        self.algo_worker.moveToThread(self.algo_thread)

        # Connect the worker output to the terminal
        self.algo_worker.output_line.connect(self.system_monitor.terminal_output.append_text)

        # When the worker is done, re-enable the button
        self.algo_worker.finished.connect(self.control_buttons.enable_button)
        self.algo_worker.error.connect(self.control_buttons.enable_button)

        # Cleanup
        self.algo_worker.finished.connect(self.algo_thread.quit)
        self.algo_worker.finished.connect(self.algo_worker.deleteLater)
        self.algo_thread.finished.connect(self.algo_thread.deleteLater)

        self.algo_worker.error.connect(lambda msg: self.system_monitor.terminal_output.append_text(f"[ERROR] {msg}"))

        # Start
        self.algo_thread.started.connect(self.algo_worker.run)
        self.algo_thread.start()

    def buttons_connect(self):
        self.control_buttons.clear_button.clicked.connect(self.system_monitor.terminal_output.clear)
        self.control_buttons.algorithm_button.clicked.connect(self.run_current_algorithm)
        self.control_buttons.comparison_button.clicked.connect(self.run_comparisons)

    def run_comparisons(self):
        input_path = self.file_dialog_widget.get_input_file_path()

        if not os.path.isfile(input_path) or input_path == "":
            print("Invalid input path!")
            return

        output_path = self.file_dialog_widget.get_output_file_path()

        if output_path == "":
            print("Output requires a path or file name!")
            return

        self.algorithms_to_run = [
            ("CPU Huffman", ["../build/cpu_huffman_compression", input_path, output_path], self.comparison_runs),
            ("GPU Huffman", ["../build/huffman_compression", input_path, output_path], self.comparison_runs),
        ]

        self.control_buttons.comparison_button.setEnabled(False)
        self.control_buttons.comparison_button.setText("Running...")
        self.control_buttons.algorithm_button.setEnabled(False)
        self.control_buttons.algorithm_button.setText("Running...")

        # Start comparison
        self.run_next_algorithm()

    def run_next_algorithm(self):
        if not self.algorithms_to_run:
            self.popup = ExecutionTimeChartPopup(self.execution_data)
            self.popup.show()

            self.control_buttons.comparison_button.setEnabled(True)
            self.control_buttons.comparison_button.setText("Start comparison")
            self.control_buttons.algorithm_button.setEnabled(True)
            self.control_buttons.algorithm_button.setText("Run algorithm")
            return

        name, command, remaining_runs = self.algorithms_to_run.pop(0)
        self.current_algorithm_name = name

        thread = QThread()
        worker = AlgorithmWorker(command)

        self.running_jobs.append((thread, worker))

        def cleanup():
            self.running_jobs.remove((thread, worker))
            worker.deleteLater()
            thread.deleteLater()

            if remaining_runs > 1:
                self.algorithms_to_run.insert(0, (name, command, remaining_runs - 1))

            QTimer.singleShot(1000, lambda: self.run_next_algorithm())

        worker.execution_time.connect(self.handle_execution_time)
        worker.output_line.connect(self.system_monitor.terminal_output.append_text, Qt.QueuedConnection)
        worker.error.connect(lambda msg: self.system_monitor.terminal_output.append_text(f"[ERROR] {msg}"),
                             Qt.QueuedConnection)

        worker.finished.connect(thread.quit)
        thread.finished.connect(cleanup)

        worker.moveToThread(thread)
        thread.started.connect(worker.run)
        thread.start()

    def cleanup_thread(self, thread, worker):
        if (thread, worker) in self.running_jobs:
            self.running_jobs.remove((thread, worker))
        thread.wait()
        worker.deleteLater()
        thread.deleteLater()

    def closeEvent(self, event):
        self.system_monitor.resource_monitor.close()
        super().closeEvent(event)

    def handle_execution_time(self, time_ms):
        time_sec = round(time_ms / 1000, 3)
        print(f"[INFO] Algorithm execution time: {time_sec} ms")
        name = self.current_algorithm_name
        if name not in self.execution_data:
            self.execution_data[name] = []
        self.execution_data[name].append(time_sec)
