#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Ensure portability allowances still reject structural/numeric regressions."""
import io
import struct
import unittest
import wave
from fixture_compare import compare


def floats(*values):
    return struct.pack('<' + 'f'*len(values), *values)


def wav(values, rate=48000):
    out = io.BytesIO()
    with wave.open(out, 'wb') as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(rate)
        f.writeframes(struct.pack('<'+'h'*len(values), *values))
    return out.getvalue()


class ComparisonTest(unittest.TestCase):
    def check(self, a, b, kind):
        compare(a, b, kind, portable=True)

    def reject(self, a, b, kind):
        with self.assertRaises(AssertionError):
            self.check(a, b, kind)

    def test_strict_remains_exact(self):
        with self.assertRaises(AssertionError):
            compare(floats(.1), floats(.10000001), 'pcm', portable=False)
        self.reject(b'project 1', b'project 2', 'exact')

    def test_text_rounding(self):
        self.check(b'point 2 5.70530701\n', b'point 2 5.70530653\n', 'text')
        self.reject(b'point 2 5.70530701\n', b'point 3 5.70530653\n', 'text')
        self.reject(b'point 2 5.7\n', b'point 2 5.8\n', 'text')
        self.reject(b'point 2 5.7\n', b'other 2 5.7\n', 'text')
        self.reject(b'point 2 5.7\n', b'point 2 5.7 1\n', 'text')

    def test_pcm_rounding(self):
        self.check(floats(.1, -.1), floats(.10000001, -.1), 'pcm')
        self.reject(floats(.1), floats(.101), 'pcm')
        self.reject(floats(.1), floats(.1, 0), 'pcm')
        self.reject(floats(0), floats(float('nan')), 'pcm')
        self.reject(floats(float('inf')), floats(float('inf')), 'pcm')
        # Low-level noise across the whole signal must fail the RMS limit.
        self.reject(floats(0, 0), floats(5e-7, 5e-7), 'pcm')

    def test_geometry_relative_rounding(self):
        self.check(floats(-97029.34375), floats(-97029.3359375), 'geometry')
        self.reject(floats(-97029), floats(-97028), 'geometry')

    def test_raster_is_sparse_and_one_level(self):
        self.check(bytes(20000), bytes(19999)+b'\1', 'raster')
        self.reject(bytes(20000), bytes(19999)+b'\2', 'raster')
        self.reject(bytes(20000), bytes(19997)+b'\1\1\1', 'raster')
        self.reject(bytes(20000), bytes(20001), 'raster')

    def test_wav_quantization_format_and_duration(self):
        self.check(wav([100, -100]), wav([101, -101]), 'wav')
        self.reject(wav([100]), wav([102]), 'wav')
        self.reject(wav([100]), wav([100, 0]), 'wav')
        self.reject(wav([100]), wav([100], rate=44100), 'wav')


if __name__ == '__main__':
    unittest.main()
