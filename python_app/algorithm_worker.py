from PySide6.QtCore import QObject, Signal, Slot
import subprocess
import re


class AlgorithmWorker(QObject):
    output_line = Signal(str)
    execution_time = Signal(int)
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
                line = line.rstrip()
                if line:
                    self.output_line.emit(line)

                    # Check if this line contains execution time
                    match = re.search(r"Execution time:\s+(\d+)s\s+(\d+)ms", line)
                    if match:
                        seconds = int(match.group(1))
                        milliseconds = int(match.group(2))
                        total_ms = seconds * 1000 + milliseconds
                        self.execution_time.emit(total_ms)

            process.wait()
            self.finished.emit()

        except Exception as e:
            self.error.emit(str(e))
