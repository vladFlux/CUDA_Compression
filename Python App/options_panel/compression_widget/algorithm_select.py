from PySide6 import QtWidgets


class AlgorithmSelectorWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.layout = QtWidgets.QVBoxLayout(self)
        self.setLayout(self.layout)

        # Create a group box titled "Algorithm"
        algo_group_box = QtWidgets.QGroupBox("Algorithm")
        self.algo_layout = QtWidgets.QHBoxLayout()
        algo_group_box.setLayout(self.algo_layout)

        self.init_radio_buttons()

        self.layout.addWidget(algo_group_box)

    def init_radio_buttons(self):
        # Create radio buttons
        self.rle_radio = QtWidgets.QRadioButton("RLE")
        self.bwt_radio = QtWidgets.QRadioButton("BWT")
        self.huffman_radio = QtWidgets.QRadioButton("Huffman")
        self.lzw_radio = QtWidgets.QRadioButton("LZW")

        self.rle_radio.setChecked(True)

        # Group buttons
        self.button_group = QtWidgets.QButtonGroup(self)
        self.button_group.addButton(self.rle_radio)
        self.button_group.addButton(self.bwt_radio)
        self.button_group.addButton(self.huffman_radio)
        self.button_group.addButton(self.lzw_radio)

        # Connect toggled signals
        self.rle_radio.toggled.connect(lambda checked: self.on_algorithm_selected("RLE", checked))
        self.bwt_radio.toggled.connect(lambda checked: self.on_algorithm_selected("BWT", checked))
        self.huffman_radio.toggled.connect(lambda checked: self.on_algorithm_selected("Huffman", checked))
        self.lzw_radio.toggled.connect(lambda checked: self.on_algorithm_selected("LZW", checked))

        # Layout for buttons
        buttons_layout = QtWidgets.QHBoxLayout()
        buttons_layout.setSpacing(70)
        buttons_layout.addWidget(self.rle_radio)
        buttons_layout.addWidget(self.bwt_radio)
        buttons_layout.addWidget(self.huffman_radio)
        buttons_layout.addWidget(self.lzw_radio)

        self.algo_layout.addStretch()
        self.algo_layout.addLayout(buttons_layout)
        self.algo_layout.addStretch()

    def on_algorithm_selected(self, algo_name: str, checked: bool):
        if checked:
            print(f"Selected Algorithm: {algo_name}")

    def get_selected_algorithm(self) -> str:
        if self.rle_radio.isChecked():
            return "rle"
        elif self.bwt_radio.isChecked():
            return "bwt"
        elif self.huffman_radio.isChecked():
            return "huffman"
        elif self.lzw_radio.isChecked():
            return "lzw"
        return ""
