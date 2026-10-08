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

from audit_palette import palettes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', default='build')
    parser.add_argument('--dpi', type=int, default=96, help='X11 monitor DPI (96, 120, 144 or 192)')
    parser.add_argument('--app', help='Check an installed or relocated executable')
    args = parser.parse_args()
    dark = palettes()['dark']
    packed = lambda rgb: (rgb[0]<<16) | (rgb[1]<<8) | rgb[2]
    active_colors = {packed(dark[role]) for role in ('highlight','active_hover')}
    menu_surface = packed(dark['surface'])
    build = Path(args.build).resolve()
    executable = Path(args.app).resolve() if args.app else build / 'libreloop'
    font = Path(__file__).resolve().parents[1] / 'assets/fonts/LiberationSans-Regular.ttf'
    read_fd, write_fd = os.pipe()
    server = subprocess.Popen(['Xvfb', '-displayfd', str(write_fd), '-noreset', '-screen', '0', '2400x1600x24'],
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
            legacy_theme=Path(directory)/'config/libreloop/theme.txt'
            legacy_theme.parent.mkdir(parents=True)
            legacy_theme.write_text('1 ff0000\n')  # Removed Light/accent preferences cannot affect startup.
            subprocess.run([str(build / 'text_fonts_test'), str(font)], env=env, cwd=directory, check=True, timeout=30)
            subprocess.run([str(build / 'theme_test'), '--render'], env=env, cwd=directory, check=True, timeout=30)
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
            x.XMoveWindow.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_int]
            x.XStringToKeysym.argtypes = [C.c_char_p]; x.XStringToKeysym.restype = C.c_ulong
            x.XKeysymToKeycode.argtypes = [C.c_void_p,C.c_ulong]; x.XKeysymToKeycode.restype = C.c_uint
            xt.XTestFakeKeyEvent.argtypes = [C.c_void_p,C.c_uint,C.c_int,C.c_ulong]
            x.XResizeWindow.argtypes = [C.c_void_p, C.c_ulong, C.c_uint, C.c_uint]
            x.XGetImage.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_int, C.c_uint, C.c_uint, C.c_ulong, C.c_int]
            x.XGetImage.restype = C.c_void_p
            x.XGetPixel.argtypes = [C.c_void_p, C.c_int, C.c_int]; x.XGetPixel.restype = C.c_ulong
            x.XDestroyImage.argtypes = [C.c_void_p]
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
                        if title.startswith(b'LibreLoop '): found = children[index]
                if children: x.XFree(children)
                return found
            def click(px, py, button=1):
                xt.XTestFakeMotionEvent(display, 0, px, py, 0); x.XFlush(display); time.sleep(.08)
                xt.XTestFakeButtonEvent(display, button, 1, 0); x.XFlush(display); time.sleep(.08)
                xt.XTestFakeButtonEvent(display, button, 0, 0); x.XFlush(display); time.sleep(.2)
            def key(name):
                code=x.XKeysymToKeycode(display,x.XStringToKeysym(name.encode()))
                assert code
                xt.XTestFakeKeyEvent(display,code,1,0); x.XFlush(display); time.sleep(.3)
                xt.XTestFakeKeyEvent(display,code,0,0); x.XFlush(display); time.sleep(.25)
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
                    x.XMoveWindow(display,window,0,0); x.XFlush(display); time.sleep(.2)
                    x.XSetInputFocus(display, window, 1, 0); x.XFlush(display); time.sleep(.3)
                    # Mouse and keyboard transfer one highlight between Edit menu choices.
                    def menu_pixel(px,py):
                        sample=x.XGetImage(display,window,px,py,1,1,C.c_ulong(-1),2)
                        assert sample
                        pixel=x.XGetPixel(sample,0,0)&0xffffff; x.XDestroyImage(sample)
                        return pixel
                    def wait_pixels(expected):
                        deadline=time.monotonic()+3
                        while time.monotonic()<deadline:
                            if all(menu_pixel(px,py)==color for px,py,color in expected): return
                            time.sleep(.05)
                        raise AssertionError([(px,py,hex(menu_pixel(px,py)),hex(color)) for px,py,color in expected])
                    menu_highlight=packed(dark['hover']); menu_base=packed(dark['control'])
                    click(74,19); key('Down')
                    wait_pixels([(59,44,menu_base),(59,70,menu_highlight)])
                    xt.XTestFakeMotionEvent(display,0,74,47,0); x.XFlush(display); time.sleep(.3)
                    wait_pixels([(59,44,menu_highlight),(59,70,menu_base)])
                    key('Down')  # Stationary mouse must not steal keyboard focus back.
                    wait_pixels([(59,44,menu_base),(59,70,menu_highlight)])
                    key('Escape')
                    # Exercise real resize events, including scales where EDIT previously lost T.
                    if decision == 'discard':
                        dimensions = [(900,506),(1200,675),(1280,720),(1848,1040),
                                      (1860,1047),(1872,1053),(1884,1060),(1920,1080),
                                      (1370,770),(1200,675)]
                        for width,height in dimensions:
                            x.XResizeWindow(display,window,width,height); x.XFlush(display); time.sleep(.7)
                            attributes = Attributes(); assert x.XGetWindowAttributes(display,window,C.byref(attributes))
                            assert (attributes.width,attributes.height)==(width,height)
                            scale=max(1,min(width/1200,height/675))
                            click(round(attributes.x+74*scale),round(attributes.y+19*scale))
                            wait_pixels([(round(54*scale),round(59*scale),menu_surface)])
                            escape=x.XKeysymToKeycode(display,x.XStringToKeysym(b'Escape'))
                            xt.XTestFakeKeyEvent(display,escape,1,0); x.XFlush(display); time.sleep(.08)
                            xt.XTestFakeKeyEvent(display,escape,0,0); x.XFlush(display); time.sleep(.15)
                            # The last toolbar toggle must remain visible and clickable at narrow widths.
                            logical_width=width/scale
                            monitor_width=min(108,max(0,int(logical_width-636-208)))
                            keys_x=636+monitor_width+4+40+12+4*24+24
                            def toggle_pixel():
                                # Compare resting fills; mouse hover is the same lavender as selection.
                                xt.XTestFakeMotionEvent(display,0,round(attributes.x+40*scale),round(attributes.y+35*scale),0)
                                x.XFlush(display); time.sleep(.15)
                                sample=x.XGetImage(display,window,round((keys_x+3)*scale),round(9*scale),1,1,C.c_ulong(-1),2)
                                assert sample
                                pixel=x.XGetPixel(sample,0,0)&0xffffff; x.XDestroyImage(sample)
                                # Sample the fill above the icon; selected and hover fills use the curated palette.
                                return pixel in active_colors
                            xt.XTestFakeMotionEvent(display,0,round(attributes.x+(keys_x+11)*scale),round(attributes.y+19*scale),0)
                            x.XFlush(display); time.sleep(.15)
                            before=toggle_pixel()
                            click(round(attributes.x+(keys_x+11)*scale),round(attributes.y+19*scale))
                            deadline=time.monotonic()+2
                            while toggle_pixel()==before and time.monotonic()<deadline: time.sleep(.05)
                            assert toggle_pixel()!=before, f'Typing piano toggle inaccessible at {width}x{height}'
                            click(round(attributes.x+(keys_x+11)*scale),round(attributes.y+19*scale))
                            deadline=time.monotonic()+2
                            while toggle_pixel()!=before and time.monotonic()<deadline: time.sleep(.05)
                            after=toggle_pixel()
                            assert after==before, f'Typing piano toggle did not restore at {width}x{height}'
                        print('Actual resized EDIT menu/input checks passed',flush=True)
                    click(496, 18, 4)  # Saved BPM below verifies the regrouped tempo control hit.
                    close_request(window); assert app.poll() is None, 'Dirty close bypassed the prompt'
                    key('Down'); key('Up'); key('Return')
                    assert app.poll() is None, 'Keyboard Cancel closed the app'
                    close_request(window); assert app.poll() is None
                    if decision == 'discard':
                        key('Up'); key('Up'); key('Return'); assert app.poll() is None
                        click(802, 562); assert app.poll() is None, 'Canceling the save chooser closed the app'
                        close_request(window); assert app.poll() is None
                        key('k'); key('Return')
                    else:
                        key('j'); key('Return'); assert app.poll() is None, 'Save closed before choosing a file'
                        click(894, 562)
                    try:
                        assert app.wait(timeout=10) == 0
                    except subprocess.TimeoutExpired as error:
                        raise AssertionError(f'Keyboard {decision} flow did not finish') from error
                    app = None
                    if decision == 'save':
                        project_file = Path(directory) / 'project.hbt'
                        assert project_file.is_file()
                        assert float(project_file.read_text().splitlines()[1].split()[0]) == 121, 'Pointer changed the wrong control'
            x.XCloseDisplay(display)
        print('GUI checks passed: font scales, Unicode, populated smoke views, keyboard Cancel/Save/Discard, close/save.')
    finally:
        if app and app.poll() is None: app.terminate(); app.wait(timeout=10)
        server.terminate(); server.wait(timeout=10)


if __name__ == '__main__':
    main()
