# SPDX-License-Identifier: GPL-3.0-only
"""Narrow numeric allowances for references captured on another toolchain.

Callers select a representation explicitly; unknown/state/project files stay
byte exact. Standalone scripts remain strict unless CTest opts into portability.
"""
import io
import math
import os
import re
import struct
import wave

PORTABLE = os.environ.get('LIBRELOOP_PORTABLE_FIXTURES') == '1'
NUMBER = re.compile(rb'(?<![\w.])[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?(?![\w.])')


def _compare(expected, actual, kind, portable):
    if portable is None:
        portable = PORTABLE
    if not portable or kind == 'exact':
        assert expected == actual, 'Bytes differ'
        return
    if kind == 'text':
        a, b = list(NUMBER.finditer(expected)), list(NUMBER.finditer(actual))
        assert NUMBER.sub(b'#', expected) == NUMBER.sub(b'#', actual), 'Text structure differs'
        assert len(a) == len(b), 'Numeric field count differs'
        for x, y in zip(a, b):
            xx, yy = x.group(), y.group()
            if not any(c in xx + yy for c in b'.eE'):
                assert int(xx) == int(yy), f'Integer field differs: {xx!r} / {yy!r}'
            else:
                xf, yf = float(xx), float(yy)
                assert math.isfinite(xf) and math.isfinite(yf), 'Non-finite coordinate'
                assert math.isclose(xf, yf, abs_tol=1e-4, rel_tol=2e-7), f'Coordinate differs: {xf} / {yf}'
    elif kind in ('pcm', 'geometry'):
        assert len(expected) == len(actual) and len(expected) % 4 == 0, 'Float buffer length differs'
        squared = 0.0
        for (x,), (y,) in zip(struct.iter_unpack('<f', expected), struct.iter_unpack('<f', actual)):
            assert math.isfinite(x) and math.isfinite(y), 'Non-finite float'
            error = abs(x-y)
            if kind == 'pcm':
                assert error <= 1e-6, f'PCM peak error {error} exceeds 1e-6'
            else:
                assert math.isclose(x, y, abs_tol=1e-5, rel_tol=2e-7), f'Geometry differs: {x} / {y}'
            squared += error * error
        if kind == 'pcm':
            rms = math.sqrt(squared / max(1, len(expected)//4))
            assert rms <= 1e-7, f'PCM RMS error {rms} exceeds 1e-7'
    elif kind == 'raster':
        assert len(expected) == len(actual), 'Raster length differs'
        changed = 0
        for x, y in zip(expected, actual):
            assert abs(x-y) <= 1, 'Raster differs by more than one gray level'
            changed += x != y
        assert changed <= max(1, len(expected)//10000), 'Too many raster pixels differ'
    elif kind == 'wav':
        with wave.open(io.BytesIO(expected), 'rb') as a, wave.open(io.BytesIO(actual), 'rb') as b:
            assert a.getparams() == b.getparams(), 'WAV format/duration differs'
            assert a.getsampwidth() == 2, 'Expected signed 16-bit PCM'
            aa, bb = a.readframes(a.getnframes()), b.readframes(b.getnframes())
            assert len(aa) == len(bb) == a.getnframes()*a.getnchannels()*2, 'Truncated WAV'
            for (x,), (y,) in zip(struct.iter_unpack('<h', aa), struct.iter_unpack('<h', bb)):
                assert abs(x-y) <= 1, 'WAV differs by more than one quantization step'
    else:
        raise ValueError(f'Unknown fixture representation: {kind}')


def compare(expected, actual, kind='exact', *, portable=None, label='fixture'):
    try:
        _compare(expected, actual, kind, portable)
    except AssertionError as error:
        raise AssertionError(f'{label}: {error}') from error
