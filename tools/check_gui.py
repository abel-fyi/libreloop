#!/usr/bin/env python3
"""Isolated Linux/X11 regression checks; requires Xvfb, libX11 and libXtst."""
import argparse
import ctypes as C
import ctypes.util
import os
from pathlib import Path
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', default='build')
    parser.add_argument('--dpi', type=int, default=96, help='X11 monitor DPI (96, 120, 144 or 192)')
    parser.add_argument('--app', help='Check an installed or relocated executable')
    args = parser.parse_args()
    build = Path(args.build).resolve()
    executable = Path(args.app).resolve() if args.app else build / 'libreloop'
    font = Path(__file__).resolve().parents[1] / 'assets/fonts/LiberationSans-Regular.ttf'
    read_fd, write_fd = os.pipe()
    server = subprocess.Popen(['Xvfb', '-displayfd', str(write_fd), '-noreset', '-screen', '0', '1200x675x24'],
                              pass_fds=(write_fd,), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    os.close(write_fd)
    app = None
    try:
        with os.fdopen(read_fd) as stream:
            display_name = ':' + stream.readline().strip()
        # GLFW reads monitor scale from Xft.dpi, independently of framebuffer size.
        resources = C.CDLL(ctypes.util.find_library('X11'))
        resources.XOpenDisplay.argtypes = [C.c_char_p]; resources.XOpenDisplay.restype = C.c_void_p
        resources.XDefaultRootWindow.argtypes = [C.c_void_p]; resources.XDefaultRootWindow.restype = C.c_ulong
        resources.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]; resources.XInternAtom.restype = C.c_ulong
        resources.XChangeProperty.argtypes = [C.c_void_p, C.c_ulong, C.c_ulong, C.c_ulong, C.c_int, C.c_int, C.c_char_p, C.c_int]
        resources.XCloseDisplay.argtypes = [C.c_void_p]
        connection = resources.XOpenDisplay(display_name.encode()); assert connection
        setting = f'Xft.dpi: {args.dpi}\n'.encode()
        resources.XChangeProperty(connection, resources.XDefaultRootWindow(connection),
                                  resources.XInternAtom(connection, b'RESOURCE_MANAGER', 0),
                                  31, 8, 0, setting, len(setting))
        resources.XCloseDisplay(connection)
        with tempfile.TemporaryDirectory(prefix='libreloop-gui-') as directory:
            env = dict(os.environ, DISPLAY=display_name, XDG_CONFIG_HOME=directory + '/config')
            subprocess.run([str(build / 'text_fonts_test'), str(font)], env=env, cwd=directory, check=True, timeout=30)
            with open(directory + '/smoke.log', 'w') as log:
                subprocess.run([str(executable), '--smoke'], env=env, cwd=directory,
                               stdout=log, stderr=log, check=True, timeout=30)
            assert len(list(Path(directory).glob('libreloop-*.png'))) == 4
            x = C.CDLL(ctypes.util.find_library('X11'))
            xt = C.CDLL(ctypes.util.find_library('Xtst'))
            x.XOpenDisplay.argtypes = [C.c_char_p]; x.XOpenDisplay.restype = C.c_void_p
            x.XDefaultRootWindow.argtypes = [C.c_void_p]; x.XDefaultRootWindow.restype = C.c_ulong
            x.XQueryTree.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong), C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_uint)]
            x.XFetchName.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_void_p)]
            x.XFree.argtypes = [C.c_void_p]
            x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]; x.XInternAtom.restype = C.c_ulong
            x.XFlush.argtypes = [C.c_void_p]
            x.XCloseDisplay.argtypes = [C.c_void_p]
            class Attributes(C.Structure):
                _fields_ = [('x', C.c_int), ('y', C.c_int), ('width', C.c_int), ('height', C.c_int),
                            ('border_width', C.c_int), ('depth', C.c_int), ('visual', C.c_void_p),
                            ('root', C.c_ulong), ('class_', C.c_int), ('bit_gravity', C.c_int),
                            ('win_gravity', C.c_int), ('backing_store', C.c_int), ('backing_planes', C.c_ulong),
                            ('backing_pixel', C.c_ulong), ('save_under', C.c_int), ('colormap', C.c_ulong),
                            ('map_installed', C.c_int), ('map_state', C.c_int), ('all_event_masks', C.c_long),
                            ('your_event_mask', C.c_long), ('do_not_propagate_mask', C.c_long),
                            ('override_redirect', C.c_int), ('screen', C.c_void_p)]
            x.XGetWindowAttributes.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(Attributes)]
            x.XSetInputFocus.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_ulong]
            xt.XTestFakeMotionEvent.argtypes = [C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_ulong]
            xt.XTestFakeButtonEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
            class Message(C.Structure):
                _fields_ = [('type', C.c_int), ('serial', C.c_ulong), ('send_event', C.c_int),
                            ('display', C.c_void_p), ('window', C.c_ulong), ('message_type', C.c_ulong),
                            ('format', C.c_int), ('data', C.c_long * 5)]
            class Event(C.Union):
                _fields_ = [('message', Message), ('padding', C.c_long * 24)]
            x.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.POINTER(Event)]
            display = x.XOpenDisplay(display_name.encode())
            assert display
            def find_window():
                root, parent, count = C.c_ulong(), C.c_ulong(), C.c_uint()
                children = C.POINTER(C.c_ulong)()
                x.XQueryTree(display, x.XDefaultRootWindow(display), C.byref(root), C.byref(parent), C.byref(children), C.byref(count))
                found = None
                for index in range(count.value):
                    name = C.c_void_p()
                    if x.XFetchName(display, children[index], C.byref(name)) and name.value:
                        title = C.string_at(name.value); x.XFree(name)
                        if title.startswith(b'LibreLoop -'): found = children[index]
                if children: x.XFree(children)
                return found
            def click(px, py, button=1):
                xt.XTestFakeMotionEvent(display, 0, px, py, 0); x.XFlush(display); time.sleep(.08)
                xt.XTestFakeButtonEvent(display, button, 1, 0); x.XFlush(display); time.sleep(.08)
                xt.XTestFakeButtonEvent(display, button, 0, 0); x.XFlush(display); time.sleep(.2)
            def close_request(window):
                event = Event(); event.message = Message(33, 0, 1, display, window,
                    x.XInternAtom(display, b'WM_PROTOCOLS', 0), 32,
                    (C.c_long * 5)(x.XInternAtom(display, b'WM_DELETE_WINDOW', 0), 0, 0, 0, 0))
                x.XSendEvent(display, window, 0, 0, C.byref(event)); x.XFlush(display); time.sleep(.35)
            for decision in ('discard', 'save'):
                with open(directory + '/' + decision + '.log', 'w') as log:
                    app = subprocess.Popen([str(executable)], env=env, cwd=directory, stdout=log, stderr=log)
                    window = None
                    for _ in range(100):
                        window = find_window()
                        if window:
                            attributes = Attributes()
                            if x.XGetWindowAttributes(display, window, C.byref(attributes)) and attributes.map_state == 2: break
                            window = None
                        assert app.poll() is None
                        time.sleep(.05)
                    assert window, 'App did not create a window'
                    x.XSetInputFocus(display, window, 1, 0); x.XFlush(display); time.sleep(.3)
                    click(430, 18, 4)  # Change tempo, creating an unsaved edit.
                    close_request(window); assert app.poll() is None, 'Dirty close bypassed the prompt'
                    click(702, 375); assert app.poll() is None, 'Cancel closed the app'
                    close_request(window); assert app.poll() is None
                    if decision == 'discard':
                        click(466, 375); assert app.poll() is None
                        click(802, 562); assert app.poll() is None, 'Canceling the save chooser closed the app'
                        close_request(window); assert app.poll() is None
                        click(584, 375)
                    else:
                        click(466, 375); assert app.poll() is None, 'Save closed before choosing a file'
                        click(894, 562)
                    assert app.wait(timeout=10) == 0
                    app = None
                    if decision == 'save':
                        project_file = Path(directory) / 'project.hbt'
                        assert project_file.is_file()
                        assert float(project_file.read_text().splitlines()[1].split()[0]) == 121, 'Pointer changed the wrong control'
            x.XCloseDisplay(display)
        print('GUI checks passed: font scales, Unicode, populated smoke views, close/cancel/discard/save.')
    finally:
        if app and app.poll() is None: app.terminate(); app.wait(timeout=10)
        server.terminate(); server.wait(timeout=10)


if __name__ == '__main__':
    main()
