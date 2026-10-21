#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit build experiment; writes builds/logs/results outside original repo.
No candidate is adopted by this script. Uses the existing original fixture tests.
"""
import argparse
import json
import os
from pathlib import Path
import statistics
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--sysroot', type=Path, required=True)
parser.add_argument('--sdl-prefix', type=Path, required=True)
parser.add_argument('--rounds', type=int, default=3)
parser.add_argument('--resume', action='store_true')
args = parser.parse_args()
source = Path(__file__).resolve().parent.parent
root = args.output.resolve()
root.mkdir(parents=True, exist_ok=True)
scenarios = ['idle_no_rack', 'demo_song', 'sampler_32', 'sampler_128', 'sampler_32_stretch',
             'sampler_32_chorus', 'sampler_32_eq', 'fm_8', 'fm_32', 'fm_128', 'fm_analog_32']
results = json.loads((root/'results.json').read_text(encoding='utf-8')) if args.resume and (root/'results.json').exists() else []
for engine, ui, lto in [('O2','Os',False), ('O3','Os',False), ('O2','Os',True), ('O3','Os',True)]:
    name = f'{engine}-{ui}-lto{int(lto)}'
    build = root / name
    existing = next((r for r in results if r['name']==name), None)
    if existing and existing['status']=='tested':
        print(name,'already tested',flush=True)
        continue
    if existing: results.remove(existing)
    entry = {'name': name, 'engine': engine, 'ui': ui, 'lto': lto, 'status': 'building'}
    results.append(entry)
    def run(command, filename):
        with (root / filename).open('w') as log:
            return subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, cwd=source).returncode
    command = ['cmake','-S',str(source),'-B',str(build),'-DCMAKE_BUILD_TYPE=Release',
        '-DLIBRELOOP_VALIDATE_SYSTEM_SDL=ON',f'-DCMAKE_PREFIX_PATH={args.sdl_prefix}',
        f'-DCMAKE_OSX_SYSROOT={args.sysroot}','-DCMAKE_OSX_DEPLOYMENT_TARGET=15.4',
        f'-DLIBRELOOP_EXPERIMENT_ENGINE_OPT={engine}',f'-DLIBRELOOP_EXPERIMENT_UI_OPT={ui}',
        f'-DLIBRELOOP_EXPERIMENT_LTO={"ON" if lto else "OFF"}']
    start = time.monotonic()
    if run(command, name+'-configure.log') or run(['cmake','--build',str(build),'--parallel','4'], name+'-build.log'):
        entry['status'] = 'build_failed'
    elif run(['cmake','--build',str(build),'--target','benchmark_engine','--parallel','4'],name+'-benchmark-build.log'):
        entry['status'] = 'benchmark_build_failed'
    else:
        entry['build_seconds'] = time.monotonic()-start
        if run(['ctest','--test-dir',str(build),'--output-on-failure'], name+'-tests.log'):
            entry['status'] = 'regression_failed'
        else:
            entry['status'] = 'tested'
            exe = build/'LibreLoop.app/Contents/MacOS/LibreLoop'
            entry['executable_bytes'] = exe.stat().st_size
            bundle = exe.parent.parent.parent
            entry['bundle_file_bytes'] = sum(p.stat().st_size for p in bundle.rglob('*') if p.is_file())
            entry['engine_runs'] = []
            for repeat in range(args.rounds):
                # Alternate scenario order to reduce systematic warm/late bias.
                for scenario in scenarios[::1 if repeat%2==0 else -1]:
                    out = subprocess.check_output([str(build/'benchmark_engine'),scenario,'3','512'],cwd=source,text=True)
                    measurement = json.loads(out)
                    measurement['repeat'] = repeat
                    entry['engine_runs'].append(measurement)
                    (root/(name+'-engine-runs.json')).write_text(json.dumps(entry['engine_runs'],indent=2)+'\n')
            entry['engine_summary'] = {}
            for scenario in scenarios:
                runs = [r for r in entry['engine_runs'] if r['scenario']==scenario]
                entry['engine_summary'][scenario] = {'runs':len(runs), 'median_cpu_seconds':statistics.median(r['cpu_seconds'] for r in runs)}
    (root/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print(name,entry['status'],flush=True)
