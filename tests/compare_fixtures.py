#!/usr/bin/env python3
"""Compare rewrite output against immutable Phase 0 original outputs.
Uses Python standard library only as test orchestration, never app runtime.
The tolerance argument defaults to zero. Timing/state/export checks always exact.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile


def pcm_error(expected, actual):
    if len(expected) != len(actual) or len(expected) % 4:
        raise AssertionError(f"PCM length differs: {len(expected)} vs {len(actual)}")
    maximum = squared = 0.0
    changed = 0
    for (x,), (y,) in zip(struct.iter_unpack('<f', expected), struct.iter_unpack('<f', actual)):
        if not math.isfinite(x) or not math.isfinite(y):
            raise AssertionError('Non-finite PCM')
        error = abs(x - y)
        maximum = max(maximum, error)
        squared += error * error
        changed += x != y
    count = len(expected) // 4
    return maximum, math.sqrt(squared / count) if count else 0.0, changed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--fixtures', type=Path, required=True)
    parser.add_argument('--max-error', type=float, default=0.0)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if not math.isfinite(args.max_error) or args.max_error < 0:
        parser.error('max-error must be finite and nonnegative')
    build, fixtures = args.build.resolve(), args.fixtures.resolve()
    results = []
    scenarios = []
    benchmark = json.loads((fixtures / 'manifest.json').read_text())
    for item in benchmark['fixtures']:
        scenarios.append(('benchmark', [item['name'], '1', str(item['block']), 'verify'],
                          { 'capture.f32': (fixtures / f"{item['name']}-{item['block']}.f32", item['sha256']) }))
    for group in ['features', 'callback']:
        for item in json.loads((fixtures / group / 'manifest.json').read_text()):
            directory = fixtures / group / f"{item['mode']}-{item['block']}"
            scenarios.append(('feature' if group == 'features' else 'callback',
                              [str(item['mode']), str(item['block'])],
                              {name: (directory / name, digest) for name, digest in item['files'].items()}))
    for group, command, files in scenarios:
        with tempfile.TemporaryDirectory(prefix='libreloop-parity-') as directory:
            run = subprocess.run([str(build / f'{group}_capture'), *command], cwd=directory,
                                 capture_output=True, text=True, timeout=60)
            if run.returncode:
                raise AssertionError(f'{group} {command}: {run.stdout}\n{run.stderr}')
            summary = {'fixture': [group, *command], 'maximum_error': 0.0, 'rms_error': 0.0,
                       'different_samples': 0, 'checks': []}
            for name, (reference, frozen_hash) in files.items():
                expected = reference.read_bytes()
                if hashlib.sha256(expected).hexdigest() != frozen_hash:
                    raise AssertionError(f'Frozen original fixture changed: {reference}')
                actual = (Path(directory) / name).read_bytes()
                if name.endswith('.f32'):
                    maximum, rms, count = pcm_error(expected, actual)
                    summary['maximum_error'] = max(summary['maximum_error'], maximum)
                    summary['rms_error'] = max(summary['rms_error'], rms)
                    summary['different_samples'] += count
                    if maximum > args.max_error or (args.max_error == 0 and expected != actual):
                        raise AssertionError(f'{group} {command} {name}: max={maximum}, rms={rms}, differing={count}')
                elif expected != actual:
                    raise AssertionError(f'{group} {command} {name}: state/project/export differs')
                summary['checks'].append(name)
            results.append(summary)
    if args.report:
        args.report.write_text(json.dumps(results, indent=2) + '\n')
    print(f'{len(results)} original fixture scenarios passed; max error={max(r["maximum_error"] for r in results)}; timing/export exact')


if __name__ == '__main__':
    main()
