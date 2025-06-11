from PySide6 import QtWidgets
from PySide6.QtCore import QThread
from .resource_worker import StatsWorker


class ResourceMonitor(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        # Labels
        self.cpu_name_label = QtWidgets.QLabel()
        self.cpu_label = QtWidgets.QLabel("CPU: ")
        self.ram_label = QtWidgets.QLabel("RAM: ")
        self.gpu_name_label = QtWidgets.QLabel()
        self.gpu_usage_label = QtWidgets.QLabel("GPU: ")
        self.vram_label = QtWidgets.QLabel("VRAM: ")

        # Inner layout for resource stats
        stats_layout = QtWidgets.QVBoxLayout()
        stats_layout.addWidget(self.cpu_name_label)
        stats_layout.addWidget(self.cpu_label)
        stats_layout.addWidget(self.ram_label)
        stats_layout.addWidget(self.gpu_name_label)
        stats_layout.addWidget(self.gpu_usage_label)
        stats_layout.addWidget(self.vram_label)

        # Group box
        group_box = QtWidgets.QGroupBox("System Resource Usage")
        group_box.setLayout(stats_layout)

        # Outer layout
        main_layout = QtWidgets.QVBoxLayout()
        main_layout.addWidget(group_box)
        self.setLayout(main_layout)

        # Worker thread setup
        self.thread = QThread()
        self.worker = StatsWorker()
        self.worker.moveToThread(self.thread)
        self.worker.stats_ready.connect(self.update_stats)
        self.thread.started.connect(self.worker.run)
        self.thread.start()

    def update_stats(self, stats):
        self.cpu_name_label.setText(stats.get("cpu_name", "Unknown CPU"))
        self.cpu_label.setText(f"CPU usage: {stats['cpu_usage']:.1f}%")
        self.ram_label.setText(
            f"RAM: {stats['ram_used']:.1f}/{stats['ram_total']:.1f} GB ({stats['ram_percent']:.1f}%)"
        )

        if "gpu_name" in stats:
            self.gpu_name_label.setText(stats["gpu_name"])
        elif "gpu_info" in stats:
            self.gpu_name_label.setText(stats["gpu_info"])

        if "gpu_usage" in stats:
            self.gpu_usage_label.setText(f"GPU usage: {stats['gpu_usage']:.1f}%")
        else:
            self.gpu_usage_label.setText("GPU: -")

        if "vram_used" in stats and "vram_total" in stats:
            self.vram_label.setText(
                f"VRAM: {stats['vram_percent']:.1f}% ({stats['vram_used']:.0f}/{stats['vram_total']:.0f} MB)"
            )
        elif "gpu_error" in stats:
            self.vram_label.setText(f"VRAM: Error - {stats['gpu_error']}")
        else:
            self.vram_label.setText("VRAM: -")


    def closeEvent(self, event):
        self.worker.stop()
        self.thread.quit()
        self.thread.wait()

        super().closeEvent(event)


