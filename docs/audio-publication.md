# Audio ownership and publication

The public audio API has exactly one UI producer and one serialized playback-callback consumer. MIDI/backend workers send events to UI; they do not become audio-command producers. Recording capture devices each own their separate input ring. This topology is required, not inferred from SPSC naming.

## Commands and immutable project snapshots

The256-command ring is preallocated. UI writes only uncommitted slots and release-publishes a committed write index at the end of its control operation. Callback acquire-loads that watermark once, then processes at most256 commands. It release-publishes the consumed read index. No callback wait or mutex is present. UI alone may wait for capacity; an explicitly stopped device gives UI sole render ownership for draining commands.

Eight preallocated ProjectSnapshot slots carry coherent Project plus prepared PlaybackPlan. A changed snapshot is attached to a committed command and assigned its retire serial. UI never reuses a slot until acquire-observing callback acknowledgement at or past that serial. Callback scans its fixed command batch for the latest snapshot and applies it before processing the batch, preserving original latest-project-first behavior for note/key/transport events. It acknowledges only after it has copied state, cleared retired-sample references and published the updated voice view. Sample replacement callers keep existing acknowledgement-before-free semantics.

Continuous state does not silently overwrite ordered key-ups, STOP or final seeks. Queue capacity and stopped-device handling are covered by carried original assertions. UI control_depth is a single-producer publication scope, not an audio lock; callback neither reads it nor waits for it.

## Views

Callback, UI and atomic middle each own one of three fixed view buffers. Writer fills only its owned buffer and exchanges it into the middle. Reader exchanges its own old buffer with the dirty middle and reads only the newly acquired buffer. Exchanges carry acquire/release ownership. UI cached fields never alias the callback's writable view. Background callback tests never invoke the UI reader from the audio thread.

## Meter peak publication

Each atomic peak has one callback writer; UI can only exchange it with zero. A strong compare/exchange can fail only after such a clear, returning zero as the observed value. Therefore a second strong compare/exchange suffices; there is no weak/spurious or unbounded retry loop. Same maximum/clear semantics and original thresholds retained. Startup checks the lock freedom of representative queue, view, serial, position, meter, activity and recording primitives; an unsupported backend rejects startup instead of hiding a callback mutex.

## Test adaptation and preserved assertions

Golden PCM, state and exported WAV files are unchanged. The original test's blocked mailbox is now simulated by staging UI publication across the same callback boundaries. The mutex acquisition itself is no longer a valid input to the new implementation. All original CHECK expressions in audio_test remain unchanged; test-only callback wrappers refresh the UI-owned view after serial callback calls. Concurrent callback calls do not refresh UI view.

The14 callback fixture captures retain their same input values/timing. Their40960..45056-frame busy-UI interval postpones commit until45056, yielding exact original PCM and acknowledgement state. This changes the synchronization test mechanism, not the expected output or assertion. It is documented separately so no assertion is weakened to accept the port.

The macOS runtime guard is a test-only dylib interposing libc/pthread calls. Its positive control must detect forbidden trylock, and it also rejects the frozen original callback as expected. All14 new callback modes pass. ASan+UBSan and TSan pass the carried concurrent sample-retirement/queue-pressure test. LeakSanitizer is unsupported on this host and is not claimed. These checks cover exercised dynamic APIs, not arbitrary OS scheduling, page-fault latency or untested hardware backends. Mapped sample residence and callback-stack worst cases remain profiling risks; mapped original/history sharing is preserved, not silently changed into copying all long files.
