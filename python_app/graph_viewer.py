from PySide6.QtWidgets import QDialog, QVBoxLayout, QSizePolicy
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class ExecutionTimeChartPopup(QDialog):
    def __init__(self, execution_times: dict, parent=None):
        """
        Creates and shows a chart comparing execution times.

        Args:
            execution_times (dict): e.g. {
                'Algorithm A': [0.1, 0.15, 0.2],
                'Algorithm B': [0.05, 0.1, 0.13]
            }
        """
        super().__init__(parent)
        self.setWindowTitle("Execution Time Comparison")
        self.setMinimumSize(600, 400)

        self.execution_times = execution_times

        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)

        figure = Figure()
        canvas = FigureCanvas(figure)
        canvas.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        ax = figure.add_subplot(111)
        ax.set_title("Execution Time per Run")
        ax.set_xlabel("Run")
        ax.set_ylabel("Time (s)")

        max_runs = 0
        for name, times in self.execution_times.items():
            runs = list(range(len(times)))
            ax.plot(runs, times, marker='o', label=name)
            max_runs = max(max_runs, len(times))

        # Set custom X-axis labels: "Run 0", "Run 1", etc.
        xticks = list(range(max_runs))
        xlabels = [f"{i}" for i in xticks]
        ax.set_xticks(xticks)
        ax.set_xticklabels(xlabels, rotation=45)

        ax.legend()
        ax.grid(True)

        layout.addWidget(canvas)
        self.setLayout(layout)
        self.show()
