from PySide6 import QtWidgets


class ComparisonCheckboxWidget(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        self.layout = QtWidgets.QVBoxLayout(self)
        self.setLayout(self.layout)

        # Create a group box with a title
        comparison_group_box = QtWidgets.QGroupBox("Comparison settings:")
        self.checkbox_layout = QtWidgets.QHBoxLayout()
        comparison_group_box.setLayout(self.checkbox_layout)

        self.init_checkboxes()

        self.layout.addWidget(comparison_group_box)

    def init_checkboxes(self):
        # Create checkboxes
        self.rle_checkbox = QtWidgets.QCheckBox("RLE")
        self.bwt_checkbox = QtWidgets.QCheckBox("BWT")
        self.huffman_checkbox = QtWidgets.QCheckBox("Huffman")
        self.lzw_checkbox = QtWidgets.QCheckBox("LZW")

        # Connect signals to print selection when changed
        self.rle_checkbox.toggled.connect(lambda checked: self.on_checkbox_toggled("rle", checked))
        self.bwt_checkbox.toggled.connect(lambda checked: self.on_checkbox_toggled("bwt", checked))
        self.huffman_checkbox.toggled.connect(lambda checked: self.on_checkbox_toggled("huffman", checked))
        self.lzw_checkbox.toggled.connect(lambda checked: self.on_checkbox_toggled("lzw", checked))

        # Add checkboxes to the layout with spacing
        checkboxes_layout = QtWidgets.QHBoxLayout()
        checkboxes_layout.setSpacing(70)

        checkboxes_layout.addWidget(self.rle_checkbox)
        checkboxes_layout.addWidget(self.bwt_checkbox)
        checkboxes_layout.addWidget(self.huffman_checkbox)
        checkboxes_layout.addWidget(self.lzw_checkbox)

        self.checkbox_layout.addStretch()
        self.checkbox_layout.addLayout(checkboxes_layout)
        self.checkbox_layout.addStretch()

    def on_checkbox_toggled(self, name: str, checked: bool):
        state = "selected" if checked else "deselected"
        print(f"{name.upper()} was {state} for comparison")

    def get_selected_algorithms(self) -> list:
        selected = []
        if self.rle_checkbox.isChecked():
            selected.append("rle")
        if self.bwt_checkbox.isChecked():
            selected.append("bwt")
        if self.huffman_checkbox.isChecked():
            selected.append("huffman")
        if self.lzw_checkbox.isChecked():
            selected.append("lzw")
        return selected
