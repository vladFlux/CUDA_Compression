from PySide6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QSizePolicy, QPushButton,
    QFileDialog, QMessageBox, QWidget, QSpacerItem
)
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class ExecutionTimeChartPopup(QDialog):
    def __init__(self, execution_times: dict, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Execution Time Comparison")
        self.setMinimumSize(1200, 800)

        self.execution_times = execution_times
        self.figure = Figure()  # Keep a reference for saving
        self._init_ui()


    def _init_ui(self):
        layout = QVBoxLayout(self)

        # Canvas for plotting
        self.canvas = FigureCanvas(self.figure)
        self.canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        layout.addWidget(self.canvas)

        # Create a horizontal layout for the button with spacers
        button_layout = QHBoxLayout()
        button_layout.addSpacerItem(QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Minimum))

        save_button = QPushButton("Save Chart")
        save_button.setFixedWidth(150)  # Optional: make the button a reasonable width
        save_button.clicked.connect(self.save_chart)
        button_layout.addWidget(save_button)

        button_layout.addSpacerItem(QSpacerItem(0, 0, QSizePolicy.Expanding, QSizePolicy.Minimum))

        layout.addLayout(button_layout)

        # Plotting
        ax = self.figure.add_subplot(111)
        ax.set_title("Execution Time per Run")
        ax.set_xlabel("Benchmark Run")
        ax.set_ylabel("Time (s)")

        # Determine number of runs (x-axis ticks) and algorithms
        all_algos = list(self.execution_times.keys())
        num_algos = len(all_algos)
        max_runs = max(len(times) for times in self.execution_times.values())
        x = list(range(max_runs))

        bar_width = 0.8 / num_algos  # To fit all bars within 1 unit per run

        for i, (algo_name, times) in enumerate(self.execution_times.items()):
            offset = [xi + i * bar_width for xi in x]
            ax.bar(offset, times, width=bar_width, label=algo_name)

        # Set x-ticks to the center of each group
        group_centers = [xi + bar_width * (num_algos - 1) / 2 for xi in x]
        ax.set_xticks(group_centers)
        ax.set_xticklabels([f"{i}" for i in x])

        ax.legend()
        ax.grid(True)

        self.setLayout(layout)
        self.show()

    def save_chart(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Chart As Image", "execution_times.png",
            "PNG Image (*.png);;JPEG Image (*.jpg);;PDF Document (*.pdf);;All Files (*)"
        )

        if file_path:
            try:
                self.figure.savefig(file_path)
                QMessageBox.information(self, "Saved", f"Chart saved to:\n{file_path}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save chart:\n{e}")
