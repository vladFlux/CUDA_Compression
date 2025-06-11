from PySide6 import QtWidgets


class HardwareSelectorWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.layout = QtWidgets.QVBoxLayout(self)
        self.setLayout(self.layout)

        # Create a group box with title "Hardware"
        hardware_group_box = QtWidgets.QGroupBox("Hardware")
        self.hardware_layout = QtWidgets.QHBoxLayout()
        hardware_group_box.setLayout(self.hardware_layout)

        self.init_radio_buttons()

        self.layout.addWidget(hardware_group_box)

    def init_radio_buttons(self):
        # Create radio buttons
        self.cpu_radio = QtWidgets.QRadioButton("CPU")
        self.gpu_radio = QtWidgets.QRadioButton("GPU")

        # Set CPU as default selected
        self.gpu_radio.setChecked(True)

        # Group the radio buttons
        self.button_group = QtWidgets.QButtonGroup(self)
        self.button_group.addButton(self.cpu_radio)
        self.button_group.addButton(self.gpu_radio)

        # Connect signals if you want to handle changes immediately
        self.cpu_radio.toggled.connect(self.on_hardware_changed)
        self.gpu_radio.toggled.connect(self.on_hardware_changed)

        # Layout with fixed spacing between buttons
        buttons_layout = QtWidgets.QHBoxLayout()
        buttons_layout.setSpacing(100)  # Adjust spacing here

        buttons_layout.addWidget(self.gpu_radio)
        buttons_layout.addWidget(self.cpu_radio)

        # Add stretch on both sides to center the buttons group
        self.hardware_layout.addStretch()
        self.hardware_layout.addLayout(buttons_layout)
        self.hardware_layout.addStretch()

    def on_hardware_changed(self, checked):
        if checked:
            selected = "CPU" if self.cpu_radio.isChecked() else "GPU"
            print(f"Algorithm set to run on {selected}")

    def is_cpu_selected(self) -> bool:
        return self.cpu_radio.isChecked()
