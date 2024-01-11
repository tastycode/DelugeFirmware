import ctypes


sysex = ctypes.CDLL('./sysex.so')
# Declare the function prototype
setConfig = sysex.setConfig
setConfig.argtypes = [ctypes.c_int32, ctypes.c_int32]
setConfig.restype = None

setConfig(122,333)
# Access the entire dictionary of symbols

