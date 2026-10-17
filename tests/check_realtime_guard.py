#!/usr/bin/env python3
"""A positive control prevents a silently disabled callback audit from passing."""
import resource
import subprocess
import sys
result = subprocess.run([sys.argv[1]], capture_output=True, text=True,
                        preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_CORE, (0, 0)))
if result.returncode == 0 or 'Real-time forbidden call: pthread_mutex_trylock' not in result.stderr:
    raise SystemExit(f'Realtime guard failed positive control: {result.returncode}: {result.stderr}')
print('Realtime guard detects forbidden pthread_mutex_trylock')
