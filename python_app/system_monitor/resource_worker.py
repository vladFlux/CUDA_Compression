from PySide6.QtCore import QObject, Signal, QMutex, QWaitCondition
import psutil
import cpuinfo

from pynvml import (
    nvmlInit,
    nvmlShutdown,
    nvmlDeviceGetHandleByIndex,
    nvmlDeviceGetName,
    nvmlDeviceGetUtilizationRates,
    nvmlDeviceGetMemoryInfo,
    NVMLError
)

class StatsWorker(QObject):
    stats_ready = Signal(dict)

    def __init__(self, interval=1):
        super().__init__()
        self.interval = interval
        self._running = True
        self._mutex = QMutex()
        self._wait_condition = QWaitCondition()
        self.nvml_available = False

        try:
            nvmlInit()
            self.nvml_available = True
        except NVMLError as e:
            print(f"[StatsWorker] NVIDIA NVML initialization failed: {e}")
            print(f"GPU resources will not be displayed")
            self.nvml_available = False

    def stop(self):
        self._mutex.lock()
        self._running = False
        self._wait_condition.wakeAll()
        self._mutex.unlock()
        if self.nvml_available:
            nvmlShutdown()

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

            if self.nvml_available:
                try:
                    handle = nvmlDeviceGetHandleByIndex(0)
                    name = nvmlDeviceGetName(handle).decode("utf-8")
                    util = nvmlDeviceGetUtilizationRates(handle)
                    mem_info = nvmlDeviceGetMemoryInfo(handle)

                    stats.update({
                        "gpu_name": name,
                        "gpu_usage": util.gpu,
                        "vram_used": mem_info.used / (1024 ** 2),
                        "vram_total": mem_info.total / (1024 ** 2),
                        "vram_percent": (mem_info.used / mem_info.total) * 100 if mem_info.total else 0
                    })
                except NVMLError as e:
                    stats["gpu_error"] = f"NVMLError: {e}"
            else:
                stats["gpu_info"] = "No NVIDIA GPU or driver detected."

            self.stats_ready.emit(stats)

            self._mutex.lock()
            if self._running:
                self._wait_condition.wait(self._mutex, int(self.interval * 1000))
            self._mutex.unlock()
