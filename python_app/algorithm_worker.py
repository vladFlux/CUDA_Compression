from PySide6.QtCore import QObject, Signal, Slot
import subprocess


class AlgorithmWorker(QObject):
    output_line = Signal(str)
    finished = Signal()
    error = Signal(str)

    def __init__(self, command):
        super().__init__()
        self.command = command

    @Slot()
    def run(self):
        try:
            process = subprocess.Popen(
                self.command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )

            for line in process.stdout:
                if line:
                    self.output_line.emit(line.rstrip())

            process.wait()
            self.finished.emit()

        except Exception as e:
            self.error.emit(str(e))
