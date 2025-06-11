class EmittingStream:
    def __init__(self, append_callback):
        self.append_callback = append_callback

    def write(self, text):
        if text.strip():  # Skip empty lines
            self.append_callback(text)

    def flush(self):
        pass  # Required for compatibility with Python's IO system
