
import ctypes
import numpy as np

class Window():

    def __init__(self, width, height):
        self.width = width
        self.height = height

    def __del__(self):
        print('WARN: window was destroyed')

    def draw(self, image):
        print('OK: image drawn')
        print(image.copy().reshape((self.width, self.height)))



@ctypes.CFUNCTYPE(ctypes.py_object, ctypes.c_uint, ctypes.c_uint)
def window_constructor(width, height):
    window = Window(width, height)
    print(f'py: called window constructor: new window {id(window):x}')
    return window

@ctypes.CFUNCTYPE(None, ctypes.py_object, ctypes.POINTER(ctypes.c_uint))
def window_draw(window, uptr):
    print('py: called window draw method')
    raw_pixels = np.ctypeslib.as_array(uptr, shape=[window.width * window.height])
    window.draw(raw_pixels)


@ctypes.CFUNCTYPE(None, ctypes.py_object)
def window_destroy(window):
    print(f'py: called window destructor: delete {id(window):x}')
    del window


def main():
    dll = ctypes.cdll.LoadLibrary('./libbackend.so')

    print('py: calling set_window_constructor')
    dll._set_PY_window_constructor(window_constructor)

    print('py: calling set_window_draw')
    dll._set_PY_window_draw(window_draw)

    print('py: calling set_window_destructor')
    dll._set_PY_window_destructor(window_destroy)

    print('py: calling main')
    dll.main()
    print('py: main done')

if __name__ == '__main__':
    main()

