#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Collect offscreen preview render measurements without changing build policy."""
import argparse
import json
from pathlib import Path
import statistics
import subprocess
parser=argparse.ArgumentParser()
parser.add_argument('--matrix',type=Path,required=True)
parser.add_argument('--baseline',type=Path,required=True)
parser.add_argument('--rounds',type=int,default=3)
parser.add_argument('--resume',action='store_true')
args=parser.parse_args();source=Path(__file__).resolve().parent.parent;root=args.matrix.resolve()
variants=[('O3-O3-lto0-baseline',args.baseline.resolve())]
for item in json.loads((root/'results.json').read_text(encoding='utf-8')):
    if item['status']=='tested':variants.append((item['name'],root/item['name']))
results=json.loads((root/'ui-results.json').read_text(encoding='utf-8')) if args.resume and (root/'ui-results.json').exists() else []
for name,build in variants:
    if any(e['name']==name and 'summary' in e for e in results):
        print(name,'already collected',flush=True)
        continue
    results=[e for e in results if e['name']!=name]
    with (root/(name+'-ui-build.log')).open('w') as log:
        subprocess.run(['cmake','-S',str(source),'-B',str(build)],stdout=log,stderr=subprocess.STDOUT,check=True)
        subprocess.run(['cmake','--build',str(build),'--target','benchmark_ui','benchmark_engine','--parallel','4'],stdout=log,stderr=subprocess.STDOUT,check=True)
    entry={'name':name,'runs':[],'engine_runs':[]};results.append(entry)
    for repeat in range(args.rounds):
        for density in ([1,2] if repeat%2==0 else [2,1]):
            output=subprocess.check_output([str(build/'benchmark_ui'),str(source/'assets/fonts/LiberationSans-Regular.ttf'),str(density),'300'],text=True)
            data=json.loads(output);data['repeat']=repeat;entry['runs'].append(data)
            (root/'ui-results.json').write_text(json.dumps(results,indent=2)+'\n')
        if name.endswith('baseline'):
            for scenario in ['idle_no_rack','demo_song','sampler_32','sampler_128','sampler_32_stretch','sampler_32_chorus','sampler_32_eq','fm_8','fm_32','fm_128','fm_analog_32']:
                data=json.loads(subprocess.check_output([str(build/'benchmark_engine'),scenario,'3','512'],cwd=source,text=True));data['repeat']=repeat;entry['engine_runs'].append(data)
    entry['summary']={str(d):{'runs':sum(r['density']==d for r in entry['runs']),
        'median_cpu_seconds':statistics.median(r['cpu_seconds'] for r in entry['runs'] if r['density']==d),
        'median_frame_p50_ms':statistics.median(r['frame_p50_ms'] for r in entry['runs'] if r['density']==d),
        'median_frame_p99_ms':statistics.median(r['frame_p99_ms'] for r in entry['runs'] if r['density']==d)} for d in [1,2]}
    entry['executable_bytes']=(build/'LibreLoop.app/Contents/MacOS/LibreLoop').stat().st_size
    (root/'ui-results.json').write_text(json.dumps(results,indent=2)+'\n');print(name,'UI measurements collected',flush=True)
