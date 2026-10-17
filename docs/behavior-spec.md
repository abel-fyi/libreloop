# Behavior specification

Reference: `5e860696fe6cb6cd4c170f6ebf5378039b3e4af6`. All paths below are relative to original `src/`. This inventories executable source behavior, including quirks, rather than designing a new DAW. Static inspection is distinct from hands-on verification. See [baseline](baseline.md) and [report](phase-0-report.md).

Exact numeric branches, hit rectangles, control declarations, keyboard gates and timing expressions are indexed with original line numbers in [interaction constants](interaction-constants.md). That companion is part of this specification, not permission to re-derive formulas. [Source inventory](source-inventory.md) indexes every source/header and defined function; [parameter inventory](parameter-inventory.md) preserves control ranges/defaults and the project schema. Port functions incrementally, preserving branch order and floating-point operations.

## Capacity, initial state and document lifecycle

| Feature | Effective behavior and source |
| --- | --- |
| Capacity | `engine.h`: 32 channels, 8 patterns, 128 notes per pattern/channel, 100 Playlist lanes, 64 clip slots/lane, 100 inserts plus Master, 10 effect slots/bus, 32 automation sources × 64 points, 32 MIDI mappings; 48-byte names, 1024-byte sample paths. `sample.h`: RATE=48000; SAMPLE_MAX_FRAMES=INT_MAX/8. |
| Fresh document | `engine.c/project_new`: one unloaded Sampler (`@empty`), Pattern 1, 16-step pattern, empty notes/Playlist, BPM 120, all inserts available. `ui_state.c`: output volume 1, Browser focused, width 174, piano top 72, zoom 1, note length 2, span 16 for every pattern; full initial values in constants appendix. |
| Demo | `project_demo`, `samples_default`: deterministic generated Kick/Snare/Hat/Tone; BPM 110, Intro/Groove, 16/32 steps, eight-bar arrangement; Song mode. Generated noise uses seed 1 and `seed=seed*1664525u+1013904223u`. |
| New/Open/Demo/Quit | `app_project.c/replace_project`, `project_document.c/project_document_request`: finish recording; dirty state asks Save/Discard/Cancel. Failed load/save leaves current project; cancelling Save keeps replacement pending handling consistent. Quit completes only after save/discard. New filename `project.llp`, demo `demo.llp`. |
| Dirty indication | `project_document_dirty`: compare Project bytes excluding `audio_seconds`; navigation/window layout are not project edits. Filename star; Save establishes saved snapshot. |
| Save/Open/Export/Collect | `ui_menus.c/project_file_action`, `app_project.c/project_file_commit`: own chooser, later Save overwrites chosen path, Save As selects another. Flush processing first. Remember export destination. Collect creates companion sample assets. |
| Load install | `install_project`: stop preview/playback, panic MIDI, invalidate sample epochs, reset gestures/selections, zoom and loops, clear history, apply samples then document. Preserve existing project format; see schema appendix. |
| Missing audio | `load_project`, `channel_missing`: load project with missing sample indicators, preserved missing duration, right-click relink or drop replacement; failed processing is transactional. |

## Coordinate model, event cadence and focus

`ui_shell.c/ui_scale`: `max(1,min(screen_width/1200.f,screen_height/675.f))`. Window starts 1200×675; minimum 900×506 (`main.c`). Mouse position divides by this layout scale once. Logical dimensions are screen dimensions/scale. Raster density is `scale*framebuffer_width/window_width`; height ratio is queried but unused. Mac enables high DPI; Linux intentionally does not enable raylib's high-DPI flag. Font density follows actual framebuffer ratio. Preserve this effective coordinate system in Phase 2.

`windows.c` manages six movable, resizable, hideable editors: Rack, Arrangement, Piano Roll, Mixer, Instrument (Sampler/FM), EQ. Initial rects/minima/order are in the appendix. TITLE=18. Reverse draw order determines top hit; focus reorders, pinned windows stay above others. Title double-click: same ID, time>0, interval<.3 s, both coordinate distances<4. Chrome buttons occupy final 54 units in 18-unit slots; bottom-right resize requires x/y strictly within last 12. Maximize preserves restore rect; constrain to desktop. Dialog title drags are captured, clamped to window, and cancel on release/new press. Instrument minimum height is 420 for synth, 300 otherwise.

`main.c` deliberately processes previous event-frame state: native wheel polling, queued button presses, navigation, active gestures, shortcuts, window updates, OS drops, rendering/editor-local actions, history checkpoint, processing queue, cursor flush, audio publication, idle decision, EndDrawing. Replay must preserve this order; rendering currently also handles input. One pending press per button is consumed each frame (`mouse_callback`, `mouse_pressed`); held/released use raylib state. Multiple quick presses survive as queued presses. Cursor requests are resolved once per frame, last request wins; default requested every frame.

`hover` requires input_enabled plus point/rect collision: x>=left and x<right, y>=top and y<bottom ([pinned raylib shapes](https://raw.githubusercontent.com/raysan5/raylib/5.5/src/rshapes.c)). Modal popups, context menus, sample dragging, capture and window chrome gate inputs separately. `captured` enumerates graph, EQ, automation, picker, navigation, resize, scrollbar, keys, knob/fader, routing, note, velocity, arrangement, Browser, Rack, cable, ruler gestures. `ui.windows.focused` and top visible order both matter: clipboard uses focused editor; Delete uses top visible editor. MIDI take disables editing Rack/Playlist/Piano. Focus loss cancels navigation, audition keys and FM graph drags through their existing branches, not a universal cancel-all rule.

Status descriptions update on hover immediately; no delayed tooltip timer was found. Double-click timing is app-specific, not OS defaults. Preserve each call site's rectangle inequalities and time tolerance from appendix.

## Navigation, trackpad, pinch and auto grid

Sources: `macos_input.m/macos_navigation_init/poll/close`, `navigation.c/navigation_motion`, `ui_shell.c/navigate_editors`, `engine.c/timeline_zoom/timeline_grid_layout`, `ui_playlist.c/row_zoom_update/follow_view`, `main.c`.

Mac Cocoa local monitor watches only target NSWindow's scroll/magnify events and returns the events unchanged. Precise scroll sums `scrollingDeltaX/Y` into float pending.x/y and sets precise=1. It does not distinguish phase/momentumPhase and does not reverse natural scroll: effective Cocoa values are used as received, including momentum events. Nonprecise scroll falls through to GLFW/raylib. Magnify computes factor=1+magnification and, only if finite and positive, accumulates `logf(factor)/logf(1.08f)` in pending.zoom. Poll copies and zeros all pending fields. Multiple magnifications therefore compose logarithmically.

Each frame initially uses `GetMouseWheelMoveV`; if native.precise, native struct replaces it entirely, retaining native zoom too; otherwise only native.zoom replaces zoom. This precedence avoids counting precise scroll twice. The other panels still read raylib's scalar wheel separately; precise editor navigation must not silently change their scalar behavior.

`navigation_motion(input,shift,zoom_modifier,scale)` starts zoom=input.zoom. Command (macOS)/Control (Linux) adds `input.y*(precise?.05f:1)` to zoom and returns immediately, discarding x pan. Otherwise distance=`precise?1/scale:48`; x=`-input.x*distance`; Shift subtracts input.y*distance from x; without Shift y=`-input.y*distance`. Native pinch works without modifiers and combines with scroll-modifier zoom. There is no application inertia filter or acceleration, no gesture-centroid anchor and no input quantization for precise x/y.

Navigation is allowed only over top hit editor 1/2, no popup/context/sample drag/focus loss/window grab, no other capture except active middle drag. Hit region is local x>=120 Playlist or >=4 Piano, x<width-24, y>=grid top and <grid top+area. Middle press focuses and captures; delta is last logical pointer-current pointer on both axes, with resize-all cursor. Release/focus loss/modal cancels it; captured panning can continue outside grid.

Playlist: grid x=212, y=96, width=editor.w-236, area=editor.h-96-18; span=16/arrangement.zoom bars. Horizontal pan adds motion.x*span/gridw and clamps >=0. Pointer zoom anchor=`clamp((local.x-212)/gridw,0,1)` even over track headers. Vertical pan clamps 0..max(0,track_position(100)-area).

Piano: grid x=60, y=86, width=editor.w-84, area=editor.h-180. Horizontal pan adds motion.x*span/gridw and clamps >=0. Zoom occurs only at local.x>=60, anchor=`clamp((local.x-60)/gridw,0,1)`. Y deltas accumulate in `piano_scroll_remainder += motion.y/(area/visible_rows)`; `(int)` truncates rows toward zero, top subtracts rows, clamps min_top..127, subtracts consumed rows, zeros remainder at either limit. Visible rows=`max(8,min(128,25/piano_zoom))`; min_top=`ceilf(visible_rows)-1`.

Horizontal zoom uses `next=span*expf(-wheel*logf(1.08f))`; reject nonfinite/nonpositive next or nonfinite reciprocal. `start=max(0,start+anchor*(span-next))`; span=next. `unit` is ignored. There is no fixed numeric min/max zoom limit. Empty pre-origin space is possible only via Follow: `start=position-span*.5`; no clamp when following. Follow pauses for manual navigation/capture/window grab, not a timed cooldown; resumes at next eligible frame. Disabling Follow clamps starts back >=0.

Auto grid, **exact** (`engine.c/timeline_grid_layout`): start `{lines=0,labels=16,band_alpha=0}`; invalid/nonpositive pixels_per_step returns it. Double labels while `labels*pixels+.001f<30 && labels<1e30f`. If `16*pixels+.001f>=12`, start lines=.25f, halve while `lines*pixels+.001f>=32`, then double while `lines*pixels+.001f<16 && lines<16`. Edit quantum is lines if positive, else labels. Band alpha=`.09f*min(1,max(0,(16*pixels-2)/2))`. Bands use 64 steps, alternate odd blocks anchored to step 0. Bar/beat lines use fmod(step,16/4); beat alpha 1, other minor .65. `snap_floor=floorf(value/q)*q`, `snap_round=roundf(value/q)*q`, or unchanged value for q<=0. No snap selector and no modifier bypass. Ruler label spacing additionally doubles until text width+8 fits; this does not redefine edit snap.

Vertical row zoom is separate toolbar capture: `zoom=start*expf((initial_y-y)/160)`. Track zoom clamps .4..4 and preserves track_at(scroll) anchor; track height=`max(28,min(640,(stored>0?stored:52)*zoom))`. Track divider hit <3 units at either boundary; direct resize stores `roundf(clamp(32,320,initial_height+dy))/track_zoom`. Piano zoom clamps 25/128..25/8, holds top where possible and resets scroll remainder.

Horizontal scrollbar thumbs use `min(width,max(24,width*span/range))`. Clicking outside thumb centers it at pointer; drag scale maximum/max(1,travel); view ranges grow to max(previous,view+2*span,content+span), never shrink except resets. Playlist right scrollbar follows same minimum; Piano right thumb is `varea*visible_rows/128` (no 24-unit minimum), pitch drag uses varea and 128. Rack scrollbars use their independent geometry; Mixer thumb minimum is 12. Do not unify these into a generic scrollbar with changed behavior.

## Piano Roll gestures

Sources: `ui_piano.c/piano/piano_gesture_update/piano_note_drag_update/piano_key_at`, `engine.c/note_* / notes_*`, `main.c` gesture updates.

Tool buttons: Pencil/Brush/Select at x=8+i*24,y=28,20×20; independent from Playlist. Current channel/pattern shared with Rack. Grid hides notes beyond source pattern end; grey partial-bar tail `step>=end && step<ceil(end/16)*16` is inactive. Pattern extension rounds upward to whole bars, existing Playlist copy caps stay independent.

Rendered note hit iterates increasing note index; last drawn rectangle wins. Edge is final 7 units and suppressed by shortcut modifier; effective body test is x<visible_end-7. Hover resize rectangle uses original x+w, while press uses cropped note end; preserve this difference. Notes have 1-unit horizontal visual inset but entire w×row hit. Stacked engine hit scans NOTES-1..0 and uses start<=step<end. Zero-length notes hit as length 1.

Press precedence: right starts erase; shortcut starts box (Shift additive); Shift+hit toggles selection; Shift or Select on empty starts box; Brush paints except hit selected group>1; otherwise hit starts move or resize, selecting it exclusively if unselected. Empty Pencil calls note_add at snapped floor and ceilf(pointer_pitch), with remembered length (initial 2); selects it and sets moving_note=1. Clicking existing note remembers its length, or 2 if zero. All successful hit/create audition. **Current source creates and then moves a new note; it does not lock start/pitch while dragging to set length, despite the usage guide.**

Move: dx=`snap_round(step-grab.x,q)`, dy=`roundf(grab.row-current_row)`; clamp dx>=-minimum_selected_start, dy within all selected pitches 0..127; extend to selected end+dx; restore original notes and apply deltas. Only the grabbed note's changed pitch triggers audition. `notes_move` validates finite bounds/length limit; it does **not** check duplicate onset/pitch collisions. `note_add` also does not reject an existing onset/pitch; Brush explicitly checks note_at first. Preserve overlaps and topmost behavior unless user requests a fix.

Resize: delta=`snap_round(step-grab.x,q)`, minimum q>0?q:.01f; `notes_resize` clamps common delta to max(min(minimum,original_length)-original_length). Zero-length drum lengths are treated as 1; unchanged delta preserves original length 0. Extend pattern and remember grabbed note's resulting length.

Box: preserves initial selection when additive; note active, start<edit_steps, start<=right, end>left, pitch>=low and pitch-1<=high. This asymmetric interval is intentional for parity. Brush/erase interpolate segment count=`min(NOTES*2,ceil(max(abs(dx)/spacing,abs(dy))*2))`, minimum 1, inclusive samples 0..count. Brush spacing=max(remembered_length,q), erase spacing=q>0?q:1. Brush start=`origin+floor((at-origin)/spacing)*spacing`; refuses note_at same pitch with start tolerance .00001f. Erase hits original stack snapshot, so a held stroke cannot peel underlying notes. Key range ceilf(pitch), 0..127.

Velocity lane y=height-84, height=62. Value=`clamp(1,127,(vy+62-pointer_y)/.48f)`, tolerance radius=3/cw. Left paints consecutive segments; right recomputes straight ramp from initial snapshot on every move. `notes_velocity` includes all active notes with onsets in `[min(from,to)-radius,max(from,to)+radius]`, regardless of selection/pitch; t denominator threshold .000001f; round and clamp 1..127. Pattern/channel switch, modal/new press cancels, release finishes.

Piano keys hit x in [4,58), y in grid extent. Black pitches are 1,3,6,8,10 modulo 12, width 34; right white region x>=38 on black row resolves adjacent white key using row fraction<.5. White keys width 54 and extend into adjacent black rows. Hold/drag changes audition pitch; old key off then new on; slot=96+pitch%25; release, focus loss, hide/modal cancels. Channel captured at gesture start.

Shift+arrow moves selected notes exactly one step/semitone (initial press or OS repeat), even when auto grid is finer/coarser. Command/Ctrl+Up/Down or K/J moves an octave only in Pencil, focused visible piano and Browser unfocused; branch order wins over other clipboard shortcuts. Bounds reject whole move; extension uses ceil(end/16)*16. No slide-note mode, legato/quantize command, copy-drag or modifier snap override appears in this source.

## Playlist / arrangement gestures

Sources: `arrangement.c/arrangement_press/drag/release/place/hit`, `ui_playlist.c/playlist/clip_edge`, `main.c`.

Any lane can contain pattern, audio or automation; rightmost allocated slot wins hits. Source ID stored as index+1; source order patterns 0..7, audio 8..39, automation 40..71. Starts are bars; pattern/automation lengths/offsets are steps, audio caps/offsets seconds. Source picker tabs Patterns/Audio/Automation; source cards 112×50, spacing 52; select changes drawing source; double-click opens Rack/Sampler. Pattern + creates unique Pattern N name and unused palette color (24 colors, then cycle). Source actions rename/color/delete; deleting sources shifts references and associated UI state. Pattern last-delete retains one blank pattern. Track names separately rename; lane mute/solo supported.

Tools Pencil/Brush/Select/Cut/Stretch at same toolbar pitch 24. Shift-click toggles selection with any tool; Shift empty starts additive box; shortcut temporary box wins before arrangement_press. Track header x<212 uses TRACK_SELECT, including whole rows; Shift adds. Right body starts erasing with any tool, but **right-click name strip (<14 units from row top) opens source context**, not erase. Automation body owns curves first. Active recording source forbids relevant edits.

Hit intervals start<=bar<start+length/16. Edge is min(left_distance,right_distance)<7 units, nearest determines left/right (tie right); shortcut/Cut/live source suppress edge. Press order: headers, right erase, Shift selection, Cut, Stretch gating, source sync, edge resize, empty Select box, Brush, Pencil move. Brush uses hit source's length/offset; selected group>1 moves instead. Cut snaps floor with `.00001f` step bias, rejects either half<=.0001f; keeps offsets, selects both copies. Stretch only audio edge; opposite edge fixed, changes shared sampler time `.25..4`, scales every copy's offsets/caps by ratio, previews processing until release. It does not use arrangement_place overlap checks during resize/stretch.

`arrangement_place`: reject negative/nonfinite starts, invalid lane/source, nonpositive length; overlap if each boundary exceeds `.0001f` step tolerance. Slot initially floor(start) if start<64, else 0; seek first empty if occupied. Pattern source length includes full pattern; full-length audio stores cap=0 when within .0001f; explicit audio caps convert to seconds.

Move: dx=`snapped(current)-snapped(original)` where snapped=`snap_floor(bar*16+.00001f,q)/16`; unsnapped fallback raw delta. Clamp >=-first_start, dy=floor(current_lane)-initial_lane clamped by entire selected group. Rebuild temporary Project from snapshot, remove selected, place each; abort all on any collision/capacity failure. Selection follows moved slot IDs on release. No copy-drag path.

Resize snapshots sources/starts/offsets/caps. Left delta rounds current-original; right rounds pointer relative to initial start minus initial length (different anchors). Unchanged if abs(x-initial_x)<.00001f. Common minimum=min(q>0?q:.01f,old_length). Left cannot cross timeline origin or source origin for patterns/audio; automation may prepend points and shift offsets of every copy. Right pattern cannot exceed preserved full source end. Direct resize does not validate overlap. Audio conversions include channel_speed in offset adjustment; keep exact expressions in appendix.

Box clip intervals use start<right and end>left, row<bottom and row+1>top; TRACK_SELECT changes row comparison to <=bottom and ignores time. Erase interpolates continuous x interval in same lane; interrupted/outside resets last_lane. Brush stride=source_steps/16 or 1 when <=0, seeded to hit/press origin; first=ceil((left-origin)/stride-.00001f), at most 64 placements, inclusive right+.00001f.

Playlist clip double-click: same lane/slot, <.35s, x/y distance<5; plain left excludes Shift/Cut/Stretch and pattern edges, but permits audio edge. Picker double-click also requires sample epoch, same source, <.35 and <5 on each axis. Automation picker does not call its double-click helper. Ruler double-click uses <.3s and <6 each axis, same editor/button and pattern where applicable; it clears loop instead of setting marker. Left ruler sets start; right adjusts nearest loop endpoint, midpoint ties anchor first endpoint. Ruler resets only when stopped; explicit loop begins at lower endpoint and suppresses automatic song end return behavior.

Picker ghost begins when either axis displacement is >=3; this threshold only controls drawing, not release placement. Browser sample ghost begins when Manhattan displacement>5; release drop requires moved. Browser Escape/new press/modal cancellation differs from picker cancellation (picker released in Playlist; no generic Escape cancel path). Picker drop requires grid and top hit editor, snaps floor; ghost can display an invalid overlap and final placement fail. OS multiple file drops process in order at current pointer; `.llp` requests replacement, others attempt audio drop.

`app_project.c/drop_sample`: Playlist Audio list imports without placement regardless current picker tab; existing audio hit replaces its shared sample; empty grid imports reusable Audio channel only after placement precheck and decode, then full overlap check. Visible Rack row replaces sample; empty/add region creates Unsorted sample channel. Instrument-body drop replaces sample. Full target rects are in appendix. Unsupported target/status/errors preserve project where coded. Replacement retains sampler settings/routing/notes/crops/color but renames channel to file stem; relink preserves name explicitly. Dropping onto FM changes it to Sampler and removes unsupported FM automation.

## Rack, controls, Mixer and effects

Rack: `ui_rack.c/paint_rack/rack`: fixed step width=16; grid x=200, width=editor.w-226; rows start TITLE+14=32 at 28-unit pitch. Left paints C4/pitch60 velocity100 zero-length one-shots; right erases; stroke interpolation fills crossed steps, max128 per update. Melodic channel determined by piano-open marker or any pitch!=60/length>0; mini notes replace steps and click opens Piano. Painting unavailable steps extends whole pattern; no toggle-off with left click. Filter All/Audio/Unsorted; scroll compound integer conversion behavior must be retained. Add Sampler/FM via own menu, max32; channel delete removes notes in every pattern, associated audio clips/automation/MIDI bindings and shifts indices.

`ui_widgets.c/capture_control/knob_style`, `main.c`: knob hit square radius+1 (10 normally, 8 swing). Vertical drag=`value-delta_y/scale*(reverse?-1:1)*(BPM?.25f:(high-low)/160)`. Logarithmic drag=`value*powf(high/low,-delta_y/scale/160)`; integer uses continuous control_raw, clamp then round. Width reverses direction. Wheel adds `(high-low)/20` per scalar notch, integer adds1 then round; logarithmic style multiplies pow(high/low,wheel/20). Clamp to declared limits. Knob drag remains captured outside its bounds. Tempo wheel adds raw scalar1 per notch, 30..300. Pitch range wheel round(delta), 1..48; route drag `(int)((initial_y-y)/3)` (truncation toward zero) adds route_start, clamps0..insert_count and reveals insert. Exact right-click Reset/Enter value, automation and MIDI Learn when supported. Numeric parser accepts decimal comma/dot, finite bounded values, integer enforcement only when declared. Fixed DX7 coarse canonicalizes aliases `round(value)&3` only on interaction; default records unchanged until touched.

Mixer: `ui_mixer.c/mixer`, `engine.c/insert_connect/playback_plan_prepare`: Master fixed; insert strip pitch51, left62, effects panel width160. Volume/pan/width, mute/solo, rename/reset, post-fader stereo meters and arm lights. Single insert output default Master, 255 disconnected; input/output jack hit12×12; cable preview follows mouse, on release connects only to input hit; cycle detection bounded100 hops. Solo includes routed upstream sources. Right output unplugs. Device choices persist; external output routing is **reserved/unimplemented**, actual playback default device. Record inputs may be None/default/named.

Fader mapping: gain<=0 ->position0; <=1 uses `.75*(20*log10(gain)+60)/60`; >1 uses `.75+.25*db/gain_db(2)`; clamp0..1. Inverse position<=0 gain0, lower .75 maps -60..0dB, upper quarter0..+6.02dB via powf(10,db/20), max2. Drag anchored to existing handle (no jump at press); hit x+23,y top-4,width22,height range+8. Output listening knob0..1.25 independent from Master/export; master pitch -12..12 affects sample speed and previews/export independent of tempo.

Effects: ten slots every bus, Empty/Chorus/Equalizer only, select/add/delete/bypass/wet-dry/load/save. Removal deletes matching effect automation and fades existing processing state through rack path. Chorus defaults rate .513Hz/depth1.85ms/mix .5, rate .05..5/depth0..8. EQ seven bands, Off/Bell/LowShelf/HighShelf/LowCut/HighCut (cuts12dB/oct), frequency20..20000, gain±18dB, Q .2..10, wet/dry. EQ graph band hit20×20, frequency=20*powf(1000,clamped_x), gain=36*(.5-y), cuts omit gain, wheel Q+.1*scalar delta; right resets whole band. `ui_mixer.c/eq_editor` response cache 257 points; presets bypass reset. Stable parameter IDs and exact defaults in appendix.

Meters: level decay exp(-elapsed/.18); hold .5s when peak>=previous and>.00001; subsequent peak decay exp(-elapsed/.45); animate while hold/level>.001. Channel/lane flash exp(-frame_time*20), active brightness .32; channel decay threshold .005. Spectrum FFT is UI-side: stereo average power, 4096 FFT, updates every1024 frames, 96 log bands, attack .75/release .18, floor-90; visible threshold-78. Audio analyzer rings8192 frames/stream. These values affect idle and redraw scheduling.

## Instruments, processing and preset behavior

`device.c` exposes concrete Sampler/FM Synth/Chorus/EQ descriptors; there are no external plugins, scanning or plugin-host protocols. Some existing strings say “plugin”; these refer to built-in instrument menu only. Do not add plugin formats.

Sampler (`ui_instruments.c/sampler`, `sampler.c/sample_process`, `app_project.c/sampler_queue/update/flush`): normalize/reverse/polarity flags1/2/4, process pitch±12, time .25..4, mode Resample/Stretch, Fit tempo captures current BPM or resets0; start/length/trim0..1. Shared non-destructive originals and processed PCM. Region crop offset=llround(frames*start), length=llround((frames-offset)*length); trim removes peaks<=powf(10,-4.5+3*trim) at both edges across either channel. Reverse/polarity precede WSOLA/pitch process, normalize after. WSOLA window min(frames,1024), hop window/2; search ±min(frames/2,384), coarse4/refine±3, match penalty1e-6; Hann crossfade. Exact processing formulas retained in source and fixtures.

Live tempo fit uses sampler_voice_sample: 20ms rate slew `1/(RATE*.02)`, two1024-phase grains staggered512, alignment coarse±256 step8/refine±7, correlation scans256 step8 both channels; linear blend. Processing jobs never run while control drag, Stretch gesture or number popup active; only one worker; generation/epoch/settings decide whether result applies. Worker posts wake event. Flush joins/reprocesses synchronously before save/export/record. Waveform has live envelope preview for settings excluding pitch/mode differences; processed detail arrives after release.

FM Synth has three existing engines: Custom three-operator FM, Six-operator DX7, Analog. Custom Sound/Body/Attack/Motion tabs expose carrier ratio/fine, body pitch/fine/integer harmonic/FM amount, strike ratio/fine/amount, separate sound/body/strike ADSR, parallel/stacked routing, velocity brightness, LFO sine/triangle/saw/square, rate/vibrato/tremolo/fade, graph drag. ADSR handles hit14×14; graph time uses expm1(position*log1p(high*100))/100; sustain vertical mapping. Motion graph x `.1*pow(120,position)`, y amount0..1 with pitch amount×100.

DX7 Op1..6/Global; Envelope/Keyboard or Motion/Pitch-envelope pages; native four rates/four levels, output/coarse/fine/detune/velocity/tracking, ratio/fixed frequency mode, breakpoint/left-right depth and curves/rate/amp sensitivity, global algorithm stored0..31 displayed+1, feedback0..7, oscillator/key sync, transpose0..48 displayed-24, native LFO6 shapes and pitch sensitivity. Stage drags map horizontal inverted rate0..99 and vertical level0..99 with rounding; frequency fixed-mode coarse alias preservation as above. Key tracking extension0..100%. Adapted core notices remain attached.

Analog: Sound/Amplitude/Filter/Motion tabs. Two oscillators controlled as shared waveform/detune/mix, sub/noise, pulse width/PWM, chorus, amplitude and filter envelopes, cutoff/resonance/filter-env depth, per-note LFO. Saw/Pulse/Triangle/Sine. It is not the art's freely routable modulation matrix or three independent oscillators. Factory presets (9): DX7 EP/Bass/Marimba/Tub Bells, MK80-inspired EP, Juno-inspired Pad/Bass, Chime EP, Better Chime. Default FM is factory0. Factory selection replaces settings/engine; “Edited sound” uses exact struct comparison. Load/save device-specific presets, audition C4 toggles slot127, source presets can retain/replace sample path. `preset.c` reads LIBRELOOP_PRESET versions 1..6 with validation; does not imply old project support.

## Automation, MIDI, recording and transport

Automation (`automation.c`, `ui_playlist.c`, `ui_widgets.c`, `ui_menus.c/create_automation`, `engine.c/render_audio`, `audio_timing.c`): supported parameters from descriptor table, owner and slot stable. Control right-click creates source and Playlist copy on first lane with room; range is selected Song loop or current pattern span>=16. Source starts at loop start/16 or existing code's floor expression (see appendix); do not correct units casually. Initial endpoints hold normalized current value. Click curve creates point on snapped time/clamped value; squared nearest distance<49; edit only Pencil, no shortcut/Shift, body>=row_top+15, title remains clip gesture. Existing point overrides edge hit. Right deletes interior points only; endpoints retained. Drag retains grab x, snap_round, clamp value; neighbor time cannot be crossed, equality allowed; negative first-point drag prepends source and adjusts all offsets; dragging beyond copy end extends copy/source. Exact title/curve/edge precedence matters.

Playback interpolation is piecewise linear, first value held before first, last after last. Active overlapping curves later lane/slot win; after copies end hold latest ended value per target; active overrides held, muted/solo lanes excluded. Automation uses buffer-local controls, leaving manual Project values untouched. Integer controls rounded per sample. UI displays same values during Song playback. Audio pitch automation integrates exponential speed analytically (`audio_timeline_source/duration`) for source position/duration, integer pitch-range rounding splits at half-step boundaries with1e-9 direction bias; slope threshold1e-12, boundary delta1e-10, nextafter fallback. Do not replace with approximate integration.

MIDI (`midi.c/midi_parse`, `midi_input.c`, `app_midi.c`): CoreMIDI source unique ID on Mac; ALSA client:port on Linux with name fallback for unique match. 64-device display capacity; single selected input. Byte parser supports running status, ignores realtime>=F8, ignores SysEx/system data; noteon/off, CC, pitch bend used. Arrival monotonic timestamp rather than MIDI packet timestamp. GUI wake on event/hotplug. Ring2048, held voices32 (slot64+i); repeated note releases old; full held pool steals slot0. Note-on velocity0=off; sustain CC64>=64, note release deferred; CC>=120 releases channel and sustain. Learn binds channel+CC0..119, replaces existing same CC, max32; clear target/all; mapping saved in Project. Pitch bend normalization splits at8192 with divisors8191/8192; volume mappings use fader law, log parameter mappings log law; fixed DX coarse round(normalized*3)/31. On loss/overflow panic/flush; refresh after2s when selected input, only when UI runs (no independent hotplug timer while blocked).

Typing piano physical US positions Z/S/X/D/C/V/G/B/H/N/J/M/,/L/./;/slash maps48..64; Q/2/W/3/E/R/5/T/6/Y/7/U/I/9/O/0/P/[/=/] maps60..78. Slots0..35; releases use remembered channel counters. Enabled only toggle, focused window, no Browser/modal/drag, no Ctrl/Super/Alt; no text entry. Full key list and gates in appendix.

Recording (`app_recording.c`, `recording.c`, `recording_writer.c`, `midi.c`): independent Audio/Notes/Automation mask1/2/4 persists. Audio uses armed Mixer buses, post-fader taps, shared capture input stream per unique device, starts Song at marker/current Song cursor. Free channel/lane required per take. Record files canonical float32 stereo48k WAV, writer thread chunks4096, header checkpoints>=RATE frames, idle writer nanosleep5ms; capture/take rings4*RATE frames each with overflow flag. First capture frames align to first playback callback after commit. Device start/stop occurs outside audio callback. Error finishes and retains written audio, mapping failure preserves filepath/duration; empty take deletes its empty file. No count-in state or delay exists in this reference.

MIDI take creates new pattern/free lane and mutes it during recording, starts from current marker/Song cursor; musical position=max(0,(time-origin_time)*BPM/15), no quantize. Notes min length .01, extension whole bars; held durations grow. Automated CC/mouse control creates muted lane/source per target, preserves initial value and point at first_time-.01 when appropriate. Identical value run threshold .000001; max64 compacts interior point with minimal abs(interpolated-value)*time span, preserves endpoints. Finish restores lane mutes; empty last-created note pattern removed/restored; failed take retained. Recording freezes BPM/mode and clears Song loop; disables editing sources. One history action after take completes.

Transport (`ui_shell.c/transport_toggle/stop`, `audio.c/consume_commands/callback`, `engine.c`): Space when no modal toggles play/stop all previews and voices, resets to ruler start; first Stop returns to marker, second stopped Stop rewinds Song marker and current PAT marker where applicable. PAT loops marker..pattern end; Song after final edge returns0, explicit loop returns loop_start. Source edits/tempo changes reconcile active audio at current cursor; preserve musical position on BPM change by ratio old/new. Musical tick96 per step, onset llround(start*96), frames=RATE*60/BPM/4; swing warps two-step pair using delay=swing*.5 and separate first/second linear slopes, preserving order. Sampler duration fade5ms; key release5ms. FM release keeps envelopes. Float engine retains headroom; actual output listening gain then finite clamp[-1,1]. Metronome clicks1200 samples, first beat accent1800Hz vs1200Hz, .25 amplitude exp(-i/180); excluded from export. Export whole Song48k stereo PCM16 in512-frame blocks, truncates `clamp(sample)*32767`, no preview/metronome/listening gain or extra effect tail.

## History, clipboard, Browser and file chooser

`history.c`: max64 states/256MiB including shared source storage accounting, always keep latest. Snapshot includes Project and immutable originals with epoch/generation stamps; mapped sample clones refcount, heap sources clone. Ignore regenerated durations in comparison. New edit truncates redo; capture after release (not during captured/recording), control/note/clip/brush/automation gestures one action. Manual Shift repeat checkpoints before each move. Undo/redo restore sampler processed data and routing/names/colors; preserve disk files and view layout, reset selections/pointers as coded. Fail allocations/preparation before committing. Undo has no delete-file side effect.

Clipboard (`edit_clipboard.c`, `ui_piano.c/edit_selection`): note copy makes earliest onset relative0; paste at Piano ruler marker, same pitches, length/velocity intact, extends whole bars, capacity atomic, duplicate onset allowed. Clip copy preserves relative lanes/start/length/source offsets and source-count identity. Paste at selected lane else original first lane, Song marker; rejects changed sources, no room, occupied time (.00001f bar tolerance), out-of-range coordinates>1e12. Clipboard invalidated for clips when source counts change; notes may remain across documents. Cmd/Ctrl C/X/V/A gated no modal/capture/sample drag/Browser. CmdZ/ShiftZ, Linux CtrlY; Save S/ShiftS, Open O. Delete macOS Delete or Backspace, Linux Delete, gates top visible editor and no active MIDI take. Selection changes are navigation, not undo state.

Browser (`browser.c`, `main.c`): bundled sample tree root first; added roots max8, depth64, total10000 nodes. Directory-first then case-sensitive basename sort; supported .wav/.flac/.mp3/.llp; no preset tree/instrument categories synthesized from art. Expand/collapse manages subtree selection. Click sample previews/replays5s max at .7 gain, project loads with dirty guard. Up/Down/K/J selection+preview and H/L or arrows folding only initial press (not repeat); active on hover/focus, no command modifiers. Wheel casts scalar to int then ×3 rows; fractional scalar is discarded. Auto reveal selected row. Browser resize x<120 collapses to24; expands clamp<=min(600,logical_width-460), divider6. Status/footer22; preview68 at height-98. XDG/HOME config, legacy Homebeat folder fallback.

Own chooser (`file_chooser.c`, `ui_menus.c/draw_file_picker/text_input`): canonical full folder path, Up/Home, hidden toggle, directory-first case-insensitive sort, extension case-insensitive semicolon list; 10000 entries; each purpose remembers last folder in chooser-folders.txt. File list24-unit rows, wheel×3; Up/Down initial press only, Enter activates folder/file, .35s same-row double-click without pointer tolerance. Scroll thumb>=24; click/drag centers thumb immediately. Filename/path fields; UTF-8 append/backspace removes whole last codepoint, select-all flag; no insertion cursor or arbitrary text selection. Clipboard paste stops on first control/DEL/capacity; Ctrl/Super prevents text chars; arrows deselect-all only. Rename trims leading/trailing ASCII spaces, rejects empty. Save auto-adds extension, own overwrite confirmation, cancel preserves document; cannot replace with native/SDL dialogs.

Text editing branch order is select-all, paste, Backspace, arrow deselection,
then consume the entire character queue. Paste copies original UTF-8 bytes and
stops at decoded codepoint `<32`, `==127`, or `strlen(text)+bytes>=capacity`.
Typed input skips those control characters and command-modified characters;
accepted input appends only with `strlen(text)+bytes<capacity`. Backspace walks
back through continuation bytes while `((unsigned char)text[n]&0xc0)==0x80`.
There is no key-repeat Backspace branch. Pasting a null clipboard retains the
selection; a non-null empty clipboard clears selected text. Invalid clipboard
bytes follow pinned raylib5.5 `GetCodepointNext` behavior and are copied unchanged
when admitted. The rewrite extraction is `interaction/text_edit.c:text_edit`
and `text_backspace`; the desktop adapter remains `ui_menus.c:text_input`.
Twenty-four original-derived contexts check both model and actual adapter,
including multi-byte capacity boundaries and simultaneous paste/delete/typing.

Numeric popup Apply (`ui_menus.c:draw_popup_content`) copies into a 64-byte
buffer, replaces every comma with a dot, uses `strtof`, then accepts only a
consumed number with no trailing non-whitespace, finite value, inclusive
`number_low..number_high`, and (when integer-only) `parsed==roundf(parsed)`.
Rejected input leaves the target and popup unchanged. No errno/underflow check
exists; signed zero, hexadecimal floating-point input and finite underflow
follow the C library's original parsing. Rename strips only ASCII spaces at
both ends; tabs/newlines/non-breaking spaces remain accepted as name bytes.
Trailing spaces are removed in-place before empty-name rejection; leading
spaces are skipped without shifting the edit buffer. The pure rewrite helpers
are `text_edit.c:text_number_parse` and `text_rename_name`; 200 numeric and 17
name cases compare against original branches, including the 63-byte copy limit.

Popup/context keyboard navigation (`ui_widgets.c:menu_key/menu_keys_begin/end`)
tracks the previous frame's item bounds, pointer and count. A changed menu ID or
explicit open resets count to zero, selects the requested default and records the
current pointer. Pointer motion or a deferred left press chooses the first old
bound containing the pointer using inclusive left/top and exclusive right/bottom.
Next is Down/J/Right/L; previous is Up/K/Left/H; each accepts initial press or OS
repeat. Simultaneous next+previous does nothing. An unopened existing menu with
items scrolls one row at its edge while hidden rows remain, otherwise wraps the
selected item modulo count and wraps scroll to zero or `fmaxf(0,total-count)`.
Enter/keypad Enter accepts initial press only and is suppressed on the open
frame. Button drawing records at most128 bounds, excludes the close symbol
`"x"` from navigation, and consumes Enter on the selected item. End caps count
at128, clears active/enter, and clamps an oversized selection only when count>0.
The pure rewrite is `menu_navigation.c:menu_navigation_begin/end`; both it and
the actual desktop wrappers match32 original begin/end captures. Bounds and
pointer coordinates are logical UI units; these functions contain no timed or
DPI-dependent branch. Menu drawing/activation and native event delivery remain
separate acceptance work.

Chooser handler replay (`tests/harness/file_picker_sequence.h`) runs actual
`draw_file_picker`, sync/submit/commit and text-input handlers with real chooser
directory enumeration, filtering, extension/canonicalization/overwrite checks and
remembered-folder persistence. Twenty-four three-frame contexts at logical
scales1/2 freeze selection, strict `.35` double-click boundary (and negative-time
quirk), directory Enter, filename/path Enter, invalid paths, confirmation/back,
cancellation, hidden files, fractional wheel and centered-thumb drag/release,
Up/Home, empty folders, open/save/collect/relink/preset/export actions and I/O
failure recovery. Project I/O completion is controlled; drawing and button hover
descriptions are disabled. The replay compares chooser/UI state, status/errors,
I/O calls and remembered paths; it does not verify actual project I/O, native
clipboard/IME, SDL delivery or rendered dialog appearance. Only the temporary
filesystem root is normalized in captures; control values and timestamps remain
original. The existing file/project/asset tests cover their separate services.

Real project-handler integration (`tests/harness/project_commit_sequence.h`)
compares the carried original app_project.c operations with the rewrite using
actual project/preset/asset I/O, mapped stereo float WAV data, sample processing,
history retention and WAV export. Twelve contexts cover save-as/collect/export,
valid load/missing sample/corrupt load, failed save, relink preserving channel
name, sampler preset success/missing-source failure, and confirmation save before
quit with success/failure. Snapshots include full current/saved projects,
original/processed PCM and generation counters, status/document/history state
and generated files. Only temporary-root spelling in captured project paths is
normalized; asset paths retain all directory components. mkdtemp's six-character
input is deterministic, with real directory creation and the original asset
naming formula retained. Hardware publication is counted; decoder fallback,
worker scheduling, dialog rendering and native delivery are not tested by this
integration capture. Existing decoder/callback tests supply separate evidence.

## Discrepancies requiring user decisions before behavior changes

The prompt names count-in, copy-drag, slide/quantize and snap modifiers, but no implemented counterpart is present at the frozen commit. Adding them would be new functionality. Source differs from usage prose for new-note drag and duplicate-onset blocking. Picker cancellation, unlimited horizontal zoom and certain resize overlaps are also quirks to preserve. No change is proposed here. Confirm the frozen commit is the desired behavioral target before deciding any correction; keep parity with this source unless explicitly authorized otherwise.

## Verification scope

34 core tests passed, 50 repeated original audio fixtures captured. UI inventory is source-reviewed; GUI execution, native trackpad/mouse feel, pixels/font rasterization, recording hardware and realtime callback tests remain unverified because desktop dependencies could not be fetched. Phase 0 is not a hands-on parity acceptance.
