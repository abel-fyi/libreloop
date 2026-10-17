# Original audio and shared-state audit

Static audit of frozen source plus core tests; no original audio callback/device execution was possible because miniaudio is supplied by the unavailable raylib archive. Original tests/audio_test.c directly includes audio.c and invokes callback without a device; this should be run and captured before Phase 1. Core tests do not establish callback correctness or physical latency.

## Render path

`audio.c/callback` -> `consume_commands` -> `render_mixer_io` -> `engine.c/render_audio` -> concrete instrument processing (`instrument.h`, sampler_voice.c, fm_synth.c, dx7.c, analog.c) -> effects_process_eq -> chorus/equalizer. Capture callback only pushes PCM into rings; mixer input pops device rings and output pushes post-fader takes. Preview/metronome/listening gain/clamp/analyzer/view publication surround shared render path. Export uses render -> same render_audio, excluding device-only contributors.

No malloc/calloc/realloc/free, stdio/file access or printf was found on this application's steady render/capture callback path. Sample processing, effects allocation, decoding, history, file mapping and writer work occur outside it. This is a source finding, not instrumented proof over all libraries/OS backends. DX7 tables and parameter-name snprintf run in project/default FM preparation before startup; dx7_init calls pthread_once from fm_legacy, not from dx7_sample. Explicit initialization before callback remains a port requirement.

## Violations and responsiveness risks

| Finding | Original source | Phase 1 requirement |
| --- | --- | --- |
| Callback calls pthread_mutex_trylock then unlock twice | audio.c:150..153,221..224 | Fails user's no-lock rule even though unsuccessful trylock defers updates rather than silencing playback. Replace mailbox/view ownership with bounded atomic publication; no mutex operation on callback. |
| Meter peak accumulation unbounded weak CAS retry | audio.c:215..218 | UI atomic_exchange can compete indefinitely in theory; use single-producer bounded publication/per-block peaks without unbounded retries. Preserve visible peak meaning. |
| Command drain can consume a full256-entry queue, large project+plan copies, STOP voice clears and effect resets | audio.c/consume_commands, playback_plan.c | Queue drain is bounded while mutex held, but worst burst/reset cost needs measurement. Keep ordered events, final seek and key-ups; coalesce only state updates whose semantics permit it. |
| Large render stack, fallback plan, automation arrays and per-voice chorus state | engine.c/render_audio, engine.h/Player, fm_synth.h/FMVoice | Measure callback stack/high-water and reset/burst cost. Do not change DSP arithmetic. Preallocate runtime/scratch outside callback as needed. |
| Mapped samples can fault during callback reads | sample_storage.c, sample_at | No explicit file call in callback, but memory mapping does not prove resident PCM; page faults may block. Establish resident/pre-touched realtime buffers/lifetime policy; Windows mapping behavior needs separate evidence. |
| Analytic pitch timeline iterates source segments, many clips/points, nextafter fallback | audio_timing.c, playback_plan preparation/fallback | Finite bounded data capacities do not establish suitable realtime deadline; dense pitch automation duration/position reconstruction needs worst-case benchmark. Preserve exact integral/inverse. |
| Slow whole-project/geometry work under UI mailbox mutex | audio_update/project_snapshot_update | Audio continues when lock busy; control/view changes postpone. New publication must preserve coherent project+plan and acknowledgement, not create torn state or stale freed PCM. |
| UI waits for queue space/ack, no timeout | submit/wait_acknowledged;100µs nanosleep | UI can freeze on non-stopped failed/stalled device. No callback blocking here, but responsiveness issue; failure/device-lifecycle handling belongs at boundary. Avoid silently dropping events or freeing before acknowledgement. |
| clock_gettime in callback | audio.c/monotonic_time | OS clock implementation/cost not proved on each target; small platform monotonic clock, verify no allocation/locking on supported realtime targets. |
| Float and64-bit atomics could be library-lock backed on another target | audio.c/recording rings | Lock-free probe passed here for uint_fast64,uint,int,uchar,float; require per-target compile/runtime guarantee. Do not infer Windows/32-bit support. |

The no-lock redesign is explicitly authorized by Phase 1 requirements; it must preserve audio output and event ordering. Quirk fixes affecting user behavior still require approval.

## Ownership and data flow

UI single thread owns Project, history, sample originals/processed data and all interaction state. Callback owns project/player/live/plan/voices/effects and borrowed PCM while device runs. Pending ProjectSnapshot+commands and view are guarded by same mutex. ProjectSnapshot plan is prepared coherently on UI; callback copies both flags together. Sample replacement clears affected voices/preview and sets audio_resync. A serial acknowledgement after consume and view publication lets UI free retired samples. UI may consume when ready=0 or device stopped while holding startStopLock. Preserve this dual lifecycle, but prevent concurrent offline/audio render owners in new design.

True SPSC rings: one playback callback -> one UI consumer for each analyzer stream; one capture device callback -> playback callback per input ring; playback callback -> one writer thread per take ring. Rings use relaxed index owner reads plus acquire peer reads/release writes. Overflow drops/refuses new samples, flags recording failure; analyzer drops full frames. No overwriting live unread samples. Recording allocation/device start/stop/uninit outside callback, device stopped before switching recording fields; safe lifecycle assumption depends on miniaudio stop completing callbacks (not verified here).

MIDI ring is backend input -> UI, not audio callback. Linux one worker producer; macOS input receive callback assumes one serialized producer for selected source. CoreMIDI notification only writes changed flag and wake. Producer serialization and lifecycle/flush ordering need verification; do not blindly label macOS queue SPSC without OS contract evidence. midi_input_flush sets read to write; release/close should prevent in-flight publication. GUI wake from worker is OS event operation, never introduced into audio thread.

Sampler job fields are set before pthread_create; done atomic publishes result; UI checks then joins before reading/freeing; generation/epoch rejects stale output. Waveform caches, FFT, history/sample allocations and decoder operate UI/worker only. Recording writer atomically publishes written frame count; UI maps growing canonical files and peak caches. File header and mapping consistency needs extended recording validation on every target.

## Idle loop audit

main.c enables event waiting after>.15s inactivity when animate is false. Movement, navigation input, resize, key press/release/repeat, queued mouse presses/release, completed sampler job reset activity. animate includes audio_active, mixer meter decay, spectrum above-78dB, visible Rack activity decay, Browser preview, playback, captured gestures, window grab, popup drag, sample drag, visible instrument audition. Mixer_decay_active appears independently of Mixer visibility, so hidden decay may still wake; preserve original effective condition rather than documentation's simplified claim. Track flash itself is not an independent wake reason. Status hover has no timer.

Audio update and cursor flush happen before EndDrawing, then raylib can block. Sampler completion and MIDI/hotplug post GLFW empty events. There is no polling wake timer for midi_refresh while fully idle; the2s refresh check runs only on a frame. Playback is60FPS target, partial busy-wait disabled in CMake. The audio device continues callbacks even when UI blocks; near-zero UI work does not mean zero process/audio CPU. Analyzer rings are always fed, but do not themselves post a wake. Meter/spectrum decay may need multiple frames before sleep. Quantitative idle/active GUI CPU/GPU unavailable, not inferred from this audit.

## Required follow-up verification

Capture actual callback PCM including command order, preview/key/metronome/output clipping, recording taps, busy UI mailbox and sample retirement from original before port. Existing audio_test already covers these assertions, but was not runnable. Instrument allocator/lock/file calls and race lifetime under realistic producers, stress final command/key-up preservation, check lock-free primitives on each supported target, render automation/routing/reset worst cases, test prolonged recording and map faults, measure hardware deadlines/latency. Core benchmark results measure render work alone.
