#!/usr/bin/env python3
"""Run isolated offline engine benchmarks; 100% CPU means one core at real time."""
import argparse
import json
import platform
import random
import statistics
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', default='build')
    parser.add_argument('--seconds', type=float, default=10)
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--output', default='local/performance/engine.json')
    args = parser.parse_args()
    if args.repeats < 1 or not 0 < args.seconds <= 25:
        parser.error('repeats must be positive and seconds must be between 0 and 25')
    scenarios = ['idle_no_rack', 'idle_rack', 'idle_monitor', 'demo_song',
                 'sampler_32', 'sampler_128', 'song_100', 'sampler_32_stretch',
                 'sampler_32_chorus', 'sampler_32_eq', 'fm_8', 'fm_32',
                 'fm_128', 'fm_legacy_32', 'fm_32_motion']
    cases = [(scenario, 512) for scenario in scenarios]
    cases += [(scenario, 64) for scenario in ['idle_monitor', 'sampler_32', 'fm_8',
                                            'fm_32', 'fm_128', 'sampler_32_stretch', 'sampler_32_eq']]
    results = {f'{scenario}/{block}': [] for scenario, block in cases}
    executable = str(Path(args.build).resolve() / 'benchmark_engine')
    for repeat in range(args.repeats):
        order = cases[:]
        random.Random(60 + repeat).shuffle(order)
        for scenario, block in order:
            row = json.loads(subprocess.check_output([executable, scenario, str(args.seconds), str(block)], text=True))
            results[f'{scenario}/{block}'].append(row)
        print(f'Completed repeat {repeat + 1}/{args.repeats}', flush=True)
    medians = []
    for rows in results.values():
        medians.append({key: statistics.median(row[key] for row in rows)
                        if isinstance(rows[0][key], (int, float)) else rows[0][key] for key in rows[0]})
    report = {'machine': platform.uname()._asdict(), 'repeats': args.repeats,
              'method': 'Offline 48 kHz stereo, 128 warmup blocks; process CPU time; wall-clock block latency; separate process per scenario. Not hardware dropout measurements.',
              'results': results, 'medians': medians}
    destination = Path(args.output)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2))
    for row in medians:
        print(f"{row['scenario']:22} {int(row['block_frames']):4} frames "
              f"{row['cpu_percent']:7.2f}% CPU  p99 {row['block_p99_ms']:6.3f} ms "
              f"/ {row['deadline_ms']:6.3f} ms deadline")


if __name__ == '__main__':
    main()
