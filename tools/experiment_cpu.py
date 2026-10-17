#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare experimental CPU rasterization with SDL software; no adoption."""
import argparse,json,statistics,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--cpu',type=Path,required=True);p.add_argument('--sdl',type=Path,required=True);args=p.parse_args()
source=Path(__file__).resolve().parent.parent;root=args.output.resolve();root.mkdir(parents=True,exist_ok=True)
results=[]
for name,build in [('sdl',args.sdl.resolve()),('cpu',args.cpu.resolve())]:
    entry={'name':name,'runs':[]};results.append(entry)
    for repeat in range(3):
        for density in ([1,2] if repeat%2==0 else [2,1]):
            for blit in ([False,True] if repeat%2==0 else [True,False]):
                command=[str(build/'benchmark_ui'),str(source/'assets/fonts/LiberationSans-Regular.ttf'),str(density),'200']
                if blit:command.append('--blit')
                data=json.loads(subprocess.check_output(command,text=True));data['repeat']=repeat;entry['runs'].append(data)
                (root/'timings.json').write_text(json.dumps(results,indent=2)+'\n')
    entry['summary']={f'{density}x-present{int(blit)}':{'runs':3,'median_cpu_seconds':statistics.median(r['cpu_seconds'] for r in entry['runs'] if r['density']==density and r['includes_sdl_present']==blit),'median_frame_p50_ms':statistics.median(r['frame_p50_ms'] for r in entry['runs'] if r['density']==density and r['includes_sdl_present']==blit),'median_frame_p99_ms':statistics.median(r['frame_p99_ms'] for r in entry['runs'] if r['density']==density and r['includes_sdl_present']==blit)} for density in [1,2] for blit in [False,True]}
    exe=build/'LibreLoop.app/Contents/MacOS/LibreLoop';entry['executable_bytes']=exe.stat().st_size
    (root/'timings.json').write_text(json.dumps(results,indent=2)+'\n');print(name,'render/present measurements complete',flush=True)
