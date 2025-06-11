from PySide6.QtCore import QObject, Signal, QMutex, QWaitCondition
import psutil
import GPUtil
import cpuinfo


class StatsWorker(QObject):
    stats_ready = Signal(dict)

    def __init__(self, interval=1):
        super().__init__()
        self.interval = interval
        self._running = True
        self._mutex = QMutex()
        self._wait_condition = QWaitCondition()

    def stop(self):
        self._mutex.lock()
        self._running = False
        self._wait_condition.wakeAll()  # Wake the thread if it’s waiting
        self._mutex.unlock()

    def run(self):
        while True:
            self._mutex.lock()
            if not self._running:
                self._mutex.unlock()
                break
            self._mutex.unlock()

            stats = {
                "cpu_name": cpuinfo.get_cpu_info().get("brand_raw", "Unknown CPU"),
                "cpu_usage": psutil.cpu_percent(),
            }

            mem = psutil.virtual_memory()
            stats.update({
                "ram_used": mem.used / (1024 ** 3),
                "ram_total": mem.total / (1024 ** 3),
                "ram_percent": mem.percent
            })

            try:
                gpus = GPUtil.getGPUs()
                if gpus:
                    gpu = gpus[0]
                    stats.update({
                        "gpu_name": gpu.name,
                        "gpu_usage": gpu.load * 100,
                        "vram_used": gpu.memoryUsed,
                        "vram_total": gpu.memoryTotal,
                        "vram_percent": (gpu.memoryUsed / gpu.memoryTotal) * 100 if gpu.memoryTotal else 0
                    })
            except Exception as e:
                stats["gpu_error"] = str(e)

            self.stats_ready.emit(stats)

            self._mutex.lock()
            if self._running:
                self._wait_condition.wait(self._mutex, int(self.interval * 1000))  # Wait for interval or stop
            self._mutex.unlock()
