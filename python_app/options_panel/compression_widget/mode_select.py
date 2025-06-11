from PySide6 import QtWidgets


class ModeSelectorWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.mode = True

        self.main_layout = QtWidgets.QVBoxLayout(self)
        self.setLayout(self.main_layout)

        # Initialize and add the group box containing radio buttons
        mode_group_box = QtWidgets.QGroupBox("Algorithm Behavior")
        self.radio_layout = QtWidgets.QHBoxLayout()
        mode_group_box.setLayout(self.radio_layout)

        self.init_radio_buttons()

        self.main_layout.addWidget(mode_group_box)

    def init_radio_buttons(self):
        self.compress_radio = QtWidgets.QRadioButton("Compress")
        self.decompress_radio = QtWidgets.QRadioButton("Decompress")
        self.compress_radio.setChecked(True)

        self.button_group = QtWidgets.QButtonGroup(self)
        self.button_group.addButton(self.compress_radio)
        self.button_group.addButton(self.decompress_radio)

        self.compress_radio.toggled.connect(self.on_compress_changed)
        self.decompress_radio.toggled.connect(self.on_decompress_changed)

        self.radio_layout.setSpacing(100)  # ← space between buttons
        self.radio_layout.addStretch()
        self.radio_layout.addWidget(self.compress_radio)
        self.radio_layout.addWidget(self.decompress_radio)
        self.radio_layout.addStretch()

    def on_compress_changed(self, checked):
        if checked:
            self.mode = True
            print("Algorithm set for compression")

    def on_decompress_changed(self, checked):
        if checked:
            self.mode = False
            print("Algorithm set for decompression")

    def get_compress_mode(self) -> str:
        if self.mode:
            return "-c"
        else:
            return "-d"
