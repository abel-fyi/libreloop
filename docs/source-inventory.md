# Frozen source inventory

All 89 source/header files are indexed below, excluding dependencies fetched by CMake. Function identification is lexical; macro-generated dispatch is additionally represented by its containing file. Read behavior-spec.md for semantics. Hash manifest covers complete file bytes.

## analog.c

58 lines; SHA-256 `f1ea0667064b26d40cf9a50f41e9cc7736b31711cef1946e44d4c62e701c9707`.

- `analog_default` — original line 6.
- `analog_valid` — original line 7.
- `analog_note_on` — original line 13.
- `blep` — original line 14.
- `oscillator` — original line 19.
- `morph` — original line 25.
- `saturate` — original line 30.
- `analog_sample` — original line 31.

## analog.h

11 lines; SHA-256 `c4588d4e31bd60882f25f826ad355632e6e32460eb92ef5255f3b08307f13f48`.

Public types, constants, interfaces or immutable data; see original file.

## app_midi.c

133 lines; SHA-256 `7c0969ab82fbd65427c6e811a56a9a3d10621981438168000c9b045f132abbf5`.

- `midi_target` — original line 4.
- `midi_panic` — original line 6.
- `midi_save_settings` — original line 11.
- `midi_refresh` — original line 16.
- `midi_select` — original line 41.
- `midi_initialize` — original line 51.
- `midi_release_key` — original line 58.
- `midi_control` — original line 66.
- `midi_poll` — original line 77.
- `typing_piano` — original line 117.

## app_project.c

429 lines; SHA-256 `b7a5d3f0c134ef1dfb6c71bb04c78f34a5b097a5f66d9a43d43f2ef0fd5ceacf`.

- `channel_missing` — original line 8.
- `channel_caption` — original line 13.
- `sample_duration` — original line 17.
- `sample_copy` — original line 21.
- `sampler_worker` — original line 25.
- `sampler_update` — original line 31.
- `sampler_flush` — original line 45.
- `sampler_queue` — original line 58.
- `project_dirty` — original line 72.
- `import_sample` — original line 74.
- `audition_entry` — original line 92.
- `rack_reveal_last` — original line 105.
- `add_instrument` — original line 112.
- `replace_instrument` — original line 128.
- `pattern_instruments` — original line 141.
- `drop_sample` — original line 149.
- `delete_channel` — original line 228.
- `install_project` — original line 253.
- `new_project` — original line 284.
- `load_project` — original line 298.
- `replace_project` — original line 309.
- `request_load_project` — original line 315.
- `replacement_saved` — original line 320.
- `history_stamps` — original line 326.
- `history_checkpoint` — original line 330.
- `undo_redo` — original line 338.
- `preset_commit` — original line 374.
- `project_file_commit` — original line 410.

## app_recording.c

125 lines; SHA-256 `9b425b84e25855f805d3f71602747feb1f7f98c073679370f0d33143678086c4`.

- `recording_finish` — original line 4.
- `recording_poll` — original line 43.
- `recording_start_audio` — original line 47.
- `recording_start` — original line 92.
- `recording_active` — original line 120.
- `recording_source` — original line 122.

## arrangement.c

223 lines; SHA-256 `2973eeb8ec834e24c85a5e1960eec9083db4fd71a5cea1573d0ec0974bd2be0d`.

- `clamp` — original line 5.
- `quantum` — original line 6.
- `snapped` — original line 7.
- `arrangement_edit_steps` — original line 8.
- `arrangement_hit` — original line 11.
- `arrangement_place` — original line 16.
- `source_steps` — original line 23.
- `place_copy` — original line 24.
- `paint` — original line 32.
- `arrangement_select_press` — original line 38.
- `arrangement_press` — original line 44.
- `arrangement_drag` — original line 117.
- `arrangement_release` — original line 215.
- `arrangement_zoom` — original line 219.

## arrangement.h

25 lines; SHA-256 `74c809492c105c90ba9e8580b6acb6934f23117fd3582fea6424a7bfb5ec79ae`.

Public types, constants, interfaces or immutable data; see original file.

## atomic_file.c

32 lines; SHA-256 `b4849f287496d9d7ae4dc47d6238112f34963d6a54a56acfcd04049a4818a693`.

- `atomic_file_open` — original line 7.
- `atomic_file_abort` — original line 19.
- `atomic_file_commit` — original line 24.

## atomic_file.h

11 lines; SHA-256 `744ff728523f536b0bd7e5c965970dd5210677deb6e2a5a6ff18c3db0806bf4e`.

Public types, constants, interfaces or immutable data; see original file.

## audio.c

604 lines; SHA-256 `70e92d34a1406a686dc30808a97d374023b899b0694db16b8ad52ab47d1446cd`.

- `audio_record_mode` — original line 64.
- `analyzer_push` — original line 83.
- `audio_spectrum_bus` — original line 89.
- `audio_spectrum_read` — original line 90.
- `ring_push` — original line 109.
- `ring_pop` — original line 118.
- `capture_callback` — original line 125.
- `record_input` — original line 129.
- `record_output` — original line 137.
- `monotonic_time` — original line 142.
- `callback` — original line 146.
- `publish_view` — original line 226.
- `consume_commands` — original line 249.
- `wait_control` — original line 354.
- `consume_stopped` — original line 358.
- `submit` — original line 368.
- `wait_acknowledged` — original line 378.
- `audio_start` — original line 385.
- `audio_devices` — original line 404.
- `audio_metronome` — original line 413.
- `audio_record_end` — original line 414.
- `record_release` — original line 420.
- `audio_record_start` — original line 424.
- `audio_record_read` — original line 466.
- `audio_record_failed` — original line 471.
- `audio_update` — original line 480.
- `audio_sample` — original line 487.
- `audio_channels` — original line 493.
- `audio_preview` — original line 499.
- `audio_note` — original line 504.
- `audio_key` — original line 507.
- `audio_key_velocity` — original line 508.
- `audio_stop` — original line 514.
- `audio_preview_position` — original line 518.
- `audio_key_position` — original line 527.
- `audio_position` — original line 549.
- `audio_meters` — original line 550.
- `audio_channel_activity` — original line 554.
- `audio_pattern_activity` — original line 560.
- `audio_track_activity` — original line 566.
- `audio_active` — original line 572.
- `audio_visual_position` — original line 573.
- `audio_close` — original line 584.
- `sample_load` — original line 591.

## audio.h

38 lines; SHA-256 `00b3e0841fc983195daa8e8a1cdb4dd73fc0b9e18720a6aae4511a62b48577f6`.

Public types, constants, interfaces or immutable data; see original file.

## audio_timing.c

98 lines; SHA-256 `92087af2722573f3af28836c05087b060c59ed82d7c31f0cdde57cf4d80a762a`.

- `audio_source_bpm` — original line 5.
- `audio_timeline_init` — original line 8.
- `segment` — original line 22.
- `integral` — original line 62.
- `audio_timeline_source` — original line 65.
- `audio_timeline_duration` — original line 74.
- `audio_clip_steps` — original line 86.
- `audio_clip_position` — original line 93.

## automation.c

84 lines; SHA-256 `c68474716693102285cd370e33273bab7157328070e975d9f17d131ec8bc5f94`.

- `automation_value` — original line 6.
- `automation_evaluate` — original line 15.
- `automation_point` — original line 29.
- `automation_move_point` — original line 37.
- `automation_create` — original line 54.
- `automation_delete` — original line 66.
- `automation_valid` — original line 76.

## browser.c

107 lines; SHA-256 `b3772d518f99e2176d8ac4851f1dba11a01b6d44112b930e2679af7263d53d35`.

- `order_nodes` — original line 7.
- `browser_select` — original line 11.
- `browser_toggle` — original line 17.
- `browser_left` — original line 49.
- `browser_right` — original line 55.
- `root_add` — original line 60.
- `browser_init` — original line 66.
- `browser_add` — original line 89.
- `browser_close` — original line 104.

## browser.h

20 lines; SHA-256 `4b7f54d5a51ef7ce5079595e17ac8668b67b96f0ae1f305b53469d59a0ec68e6`.

Public types, constants, interfaces or immutable data; see original file.

## chorus.c

35 lines; SHA-256 `1026fddaf24f96861bf80901a84753b54554b6dce3c4c7de0ff17459085924d1`.

- `chorus_default` — original line 6.
- `chorus_valid` — original line 7.
- `chorus_reset` — original line 8.
- `chorus_process` — original line 9.

## chorus.h

21 lines; SHA-256 `fd1429900a5e03150b8cf56844a0957146f69419f7451eb3616a652145d561e7`.

Public types, constants, interfaces or immutable data; see original file.

## device.c

48 lines; SHA-256 `7cac4873a6a706d25b8ee0be5733b2a93e74c32201abddbb0642977f844f228c`.

- `sampler_defaults` — original line 7.
- `fm_defaults` — original line 8.
- `chorus_defaults` — original line 9.
- `eq_defaults` — original line 10.
- `sampler_check` — original line 11.
- `fm_check` — original line 12.
- `chorus_check` — original line 13.
- `eq_check` — original line 14.
- `fm_parameter` — original line 15.
- `chorus_parameter` — original line 16.
- `eq_parameter` — original line 20.
- `device_descriptor` — original line 31.
- `instrument_descriptor` — original line 34.
- `effect_descriptor` — original line 37.
- `device_has_parameter` — original line 40.
- `device_parameter` — original line 46.

## device.h

24 lines; SHA-256 `2a9f76ab57b47a4a1c43dcd1cdbc4f753b3c4773143d62dfaf65d19660635539`.

Public types, constants, interfaces or immutable data; see original file.

## dx7.c

261 lines; SHA-256 `932962ab6d28a69d1769f04ce8b7d2292412a521645f9d755418680ded363be1`.

- `min_int` — original line 23.
- `max_int` — original line 24.
- `parameter_max` — original line 25.
- `prepare` — original line 31.
- `dx7_init` — original line 59.
- `dx7_parameter_descriptor` — original line 60.
- `dx7_factory_name` — original line 61.
- `dx7_factory` — original line 62.
- `dx7_legacy_tracking` — original line 69.
- `dx7_valid` — original line 72.
- `sine` — original line 76.
- `exponential` — original line 80.
- `frequency_lookup` — original line 85.
- `scale_level` — original line 90.
- `envelope_advance` — original line 91.
- `envelope_sample` — original line 105.
- `scale_curve` — original line 120.
- `oscillator_pitch` — original line 124.
- `pitch_advance` — original line 132.
- `pitch_sample` — original line 137.
- `setup_operator` — original line 144.
- `setup_lfo` — original line 164.
- `dx7_note_on` — original line 171.
- `dx7_note_off` — original line 180.
- `dx7_active` — original line 185.
- `lfo_sample` — original line 189.
- `lfo_delay` — original line 200.
- `render_block` — original line 205.
- `dx7_sample` — original line 258.

## dx7.h

33 lines; SHA-256 `e926021954e0d2308b0bc592c36df0a1a0f43487b27dc1dc70eb4ddb9a103323`.

Public types, constants, interfaces or immutable data; see original file.

## dx7_factory.h

40 lines; SHA-256 `4d6545d6ec0cc766fd48955f2d0a5f88e306e42c6e7ba53d1049abb0eaeae9b2`.

Public types, constants, interfaces or immutable data; see original file.

## dx7_tables.h

103 lines; SHA-256 `05d21799986a13f95783b1cb55f3eb252e06f8c783da719be8dd8c89f5eddeae`.

Public types, constants, interfaces or immutable data; see original file.

## edit_clipboard.c

66 lines; SHA-256 `d73af3817e81652c2d1bb9c4ad4c39b15281e15f07e64bc3af266b359999dc88`.

- `clipboard_copy_notes` — original line 5.
- `clipboard_copy_clips` — original line 14.
- `clipboard_paste_notes` — original line 23.
- `clipboard_paste_clips` — original line 42.

## edit_clipboard.h

18 lines; SHA-256 `640ef65141ec5b9d0778ecdb9b977728a928b6cee2149d34ef08ba4441f1f17e`.

Public types, constants, interfaces or immutable data; see original file.

## effects.c

35 lines; SHA-256 `b35d046385cd118fe1c8986b5c94f6dc6e5e0eb3fe729dda7c8d46d8619540d4`.

- `effects_create` — original line 6.
- `effects_free` — original line 11.
- `effects_reset` — original line 12.
- `effects_process_eq` — original line 13.
- `effects_process` — original line 33.

## effects.h

18 lines; SHA-256 `84ee150472366ec4649399e3dae5114120f12c8a7762c9c0eb2ca94f5bc10314`.

Public types, constants, interfaces or immutable data; see original file.

## engine.c

578 lines; SHA-256 `e6e2c82098c006e172e17fc51bd70ff7bbda8e0c3af0a06afc7e8cf9555e40a0`.

- `next_source_color` — original line 16.
- `grid_interval` — original line 24.
- `snap_floor` — original line 28.
- `snap_round` — original line 29.
- `timeline_zoom` — original line 30.
- `timeline_grid_layout` — original line 36.
- `timeline_thumb` — original line 50.
- `gain_db` — original line 51.
- `fader_position` — original line 53.
- `fader_gain` — original line 58.
- `project_default` — original line 64.
- `project_new` — original line 84.
- `project_demo` — original line 93.
- `channel_replace_instrument` — original line 108.
- `channel_delete` — original line 118.
- `pattern_delete` — original line 154.
- `insert_reset` — original line 170.
- `insert_connect` — original line 181.
- `note_at` — original line 190.
- `note_hit` — original line 195.
- `notes_velocity` — original line 202.
- `note_add` — original line 213.
- `note_move` — original line 220.
- `notes_move` — original line 226.
- `notes_resize` — original line 235.
- `samples_default` — original line 251.
- `player_reset` — original line 267.
- `player_seek` — original line 268.
- `channel_speed` — original line 273.
- `clip_source_steps` — original line 274.
- `clip_offset_steps` — original line 279.
- `clip_length` — original line 284.
- `song_steps` — original line 289.
- `solo_any` — original line 294.
- `solo_toggle` — original line 298.
- `trigger` — original line 303.
- `active_voices` — original line 334.
- `render_audio` — original line 340.
- `render` — original line 566.
- `render_live` — original line 569.
- `render_mixer` — original line 572.
- `render_mixer_io` — original line 575.

## engine.h

167 lines; SHA-256 `275eec04edce7175314a05194e71f7d6f82be90a3fd7b2e555cc051adcd76dc0`.

Public types, constants, interfaces or immutable data; see original file.

## equalizer.c

100 lines; SHA-256 `b252df711b3cccc32538bdd8254dd440aa76f99127ee6103c04ba3fe2d5d8ae3`.

- `equalizer_default` — original line 6.
- `equalizer_legacy` — original line 9.
- `equalizer_shape_name` — original line 14.
- `equalizer_valid` — original line 18.
- `equalizer_reset` — original line 25.
- `coefficients` — original line 26.
- `equalizer_process` — original line 48.
- `equalizer_response` — original line 90.

## equalizer.h

26 lines; SHA-256 `03f960b39357262b2416a7bc9c1a089c9cb5925af9b6b60900d966fb02ea0254`.

Public types, constants, interfaces or immutable data; see original file.

## file_chooser.c

145 lines; SHA-256 `44fd7f149bfec18c09e0f2c6796446c342c1436e080a94698a900ed234159d89`.

- `file_chooser_locations` — original line 13.
- `remember_folder` — original line 27.
- `file_chooser_remember` — original line 35.
- `free_entries` — original line 42.
- `file_chooser_close` — original line 46.
- `compare` — original line 47.
- `matches` — original line 51.
- `file_chooser_folder` — original line 62.
- `file_chooser_parent` — original line 90.
- `file_chooser_begin` — original line 95.
- `file_chooser_path` — original line 108.
- `file_chooser_begin_recent` — original line 137.

## file_chooser.h

24 lines; SHA-256 `9e04701a9feb419f6fc96577482000501d413e34cc6bfa388b0469e3957f353e`.

Public types, constants, interfaces or immutable data; see original file.

## fm_synth.c

171 lines; SHA-256 `b7766fe88b0ac3c4815a209dcc47bbff7aa4fc2adc7111bb10c04e8eae16a312`.

- `fm_legacy` — original line 7.
- `fm_epiano` — original line 14.
- `fm_factory` — original line 20.
- `fm_factory_name` — original line 45.
- `fm_default` — original line 46.
- `fm_parameter_pointer` — original line 47.
- `range` — original line 73.
- `fm_valid` — original line 74.
- `fm_lfo_value` — original line 84.
- `fm_note_on_velocity` — original line 91.
- `fm_note_on` — original line 100.
- `fm_note_off` — original line 101.
- `fm_active` — original line 109.
- `mod_envelope` — original line 110.
- `render_mono` — original line 118.
- `fm_sample_stereo` — original line 167.
- `fm_sample` — original line 171.

## fm_synth.h

41 lines; SHA-256 `068b204d122e0505f3e4da046e4b61effce00e39cd5f88b8a253225ea1b7431c`.

Public types, constants, interfaces or immutable data; see original file.

## history.c

75 lines; SHA-256 `e45e3243150542ad519ec80a0faecc6af9082bca22078f9b41d66a3820977065`.

- `sample_bytes` — original line 8.
- `discard` — original line 9.
- `history_clear` — original line 16.
- `canonical` — original line 17.
- `history_capture` — original line 23.
- `history_peek` — original line 53.
- `history_step` — original line 60.
- `history_rebind` — original line 62.
- `history_source_matches` — original line 70.

## history.h

16 lines; SHA-256 `d7c71a155b3b8982e7235406cd63f27a616db75f765a949bf3d61cb2d78ddd83`.

Public types, constants, interfaces or immutable data; see original file.

## instrument.c

9 lines; SHA-256 `35db348f2a48b67c3bc6dd0cbfe5adcf404cdd188dfedb215f0ea40afbfbeb98`.

- `instrument_start` — original line 3.
- `instrument_release` — original line 7.

## instrument.h

22 lines; SHA-256 `767058885cbff5223f95d6b87352d363fd8b9b244a96c84f3483eb524e9daad8`.

- `instrument_process` — original line 15.
- `instrument_active` — original line 19.

## macos_input.m

31 lines; SHA-256 `05823e6c86824816b39db3276eee359c06c49b8e419d844a8be4d55b4830dd19`.

- `macos_navigation_init` — original line 11.
- `macos_navigation_poll` — original line 25.
- `macos_navigation_close` — original line 28.

## main.c

483 lines; SHA-256 `53f14b003bbdd7cf4da18e23daa262c119c8c38b68fdcb551a08ea5d8c4a7949`.

- `main` — original line 4.

## midi.c

116 lines; SHA-256 `f4bd6d457dffe5b315560f8f83a0829fe5d1b56cbf3e9f877030346692ef98f8`.

- `midi_parse` — original line 6.
- `midi_bind` — original line 20.
- `midi_unbind` — original line 26.
- `position` — original line 30.
- `free_lane` — original line 31.
- `midi_take_begin` — original line 34.
- `midi_take_note` — original line 54.
- `midi_take_control` — original line 65.
- `midi_take_update` — original line 97.
- `midi_take_finish` — original line 106.

## midi.h

25 lines; SHA-256 `c12aaa611a788026dc4bbfad65dd244d71c73f484bed900c49cd499fd232282b`.

Public types, constants, interfaces or immutable data; see original file.

## midi_input.c

108 lines; SHA-256 `25f451fd97e28edabb6e066eb9c578c6b23b4252ea6d0a896637fb91e1f99386`.

- `midi_input_wake` — original line 14.
- `midi_input_time` — original line 15.
- `emit` — original line 16.
- `midi_input_poll` — original line 22.
- `midi_input_failed` — original line 26.
- `midi_input_changed` — original line 27.
- `notify_change` — original line 28.
- `midi_input_flush` — original line 29.
- `receive` — original line 33.
- `notify` — original line 37.
- `prepare` — original line 38.
- `source_id` — original line 39.
- `midi_input_devices` — original line 40.
- `midi_input_close` — original line 49.
- `midi_input_open` — original line 52.
- `prepare` — original line 66.
- `midi_input_devices` — original line 67.
- `receive` — original line 78.
- `midi_input_close` — original line 96.
- `midi_input_open` — original line 100.

## midi_input.h

15 lines; SHA-256 `e96089bd1df5bc7757ade822f3fb72aa305c55d98afaa975506859456e679a55`.

Public types, constants, interfaces or immutable data; see original file.

## navigation.c

11 lines; SHA-256 `70d7f0f99a5b5e19693325353822f6fe004c9ce1342062d856226d59c9f12efa`.

- `navigation_motion` — original line 3.

## navigation.h

12 lines; SHA-256 `8a781959506ab439cc803b7b3911fa69ffed352063515bfb77c06b90caa60ac2`.

Public types, constants, interfaces or immutable data; see original file.

## parameter.c

175 lines; SHA-256 `eb87c6f70d22f5216df44c5bd9eaca4ed60e312609706a01aa8159cd1f209ac6`.

- `parameter_descriptor` — original line 84.
- `parameter_pointer` — original line 89.
- `parameter_info` — original line 126.
- `parameter_write` — original line 136.
- `parameter_from_pointer` — original line 149.

## parameter.h

16 lines; SHA-256 `ef4cdd67da61df024efe12c343f5f8f1b15c219cc4a583cc050b8145459a9156`.

Public types, constants, interfaces or immutable data; see original file.

## playback_plan.c

60 lines; SHA-256 `f2094f810a61dc7e877baf06a23ae976536335364ca9a0112aca6f7e6a3872af`.

- `insert_solo` — original line 5.
- `playback_plan_compatible` — original line 9.
- `playback_plan_prepare` — original line 14.
- `project_snapshot_update` — original line 52.

## playback_plan.h

20 lines; SHA-256 `27de910d10114acb7583dd9d9e4331be2dd91214850d9f700cfee8c6d18a2ac9`.

Public types, constants, interfaces or immutable data; see original file.

## preset.c

81 lines; SHA-256 `be008c63b8faa3073cbde678f7809184029375d50e334c4c1ad1ad4d88571d4a`.

- `valid` — original line 8.
- `preset_save` — original line 16.
- `preset_load` — original line 44.

## preset.h

22 lines; SHA-256 `1a53047d562ba28ed89f104ba0829a9cd9edef6e47bac80afd06007cc306874a`.

Public types, constants, interfaces or immutable data; see original file.

## project_assets.c

113 lines; SHA-256 `244d4aa98f736ae02eef1f59ef0fc672b3ec6b9ad3f975e9e96494cf7dee6b33`.

- `project_directory` — original line 10.
- `project_sample_path` — original line 16.
- `relative_path` — original line 22.
- `project_relative_reference` — original line 35.
- `le` — original line 40.
- `save_sample` — original line 41.
- `project_save_assets` — original line 62.
- `project_load_assets` — original line 92.

## project_assets.h

14 lines; SHA-256 `e971724f3bce3d7ce4be2df2d802ba97bd79ca13e43e6e76e9e8ff5ec61523f4`.

Public types, constants, interfaces or immutable data; see original file.

## project_document.c

21 lines; SHA-256 `eac4a8912433e9e3df3d1592a5590f0a07cc1a46983bc5b0ee7b549c2d1b6bbf`.

- `project_document_dirty` — original line 6.
- `project_document_request` — original line 11.
- `project_document_saved` — original line 18.

## project_document.h

15 lines; SHA-256 `b2892c71f3f38dd47ef44b32f4afd05b39636de3a372a99bd0e932a48dcd4d46`.

Public types, constants, interfaces or immutable data; see original file.

## project_format.h

8 lines; SHA-256 `f7ca597f9eec276e4a60e1f60f9d7b023d63d2eec08d9ade3a893145e15347f1`.

Public types, constants, interfaces or immutable data; see original file.

## project_io.c

364 lines; SHA-256 `20c2c003da4fc48f43204a6eaba4203af3ff263aa8fa95f58f23e4c5302255a3`.

- `project_valid` — original line 15.
- `address` — original line 206.
- `structure_valid` — original line 209.
- `equal_value` — original line 219.
- `project_save` — original line 222.
- `line` — original line 246.
- `blank` — original line 256.
- `count_token` — original line 258.
- `number_prefix` — original line 264.
- `eol` — original line 273.
- `read_values` — original line 274.
- `project_load` — original line 308.
- `le` — original line 342.
- `export_wav` — original line 343.

## recording.c

81 lines; SHA-256 `28979b72e1bc0cb943aff472872a81a7082cab535e0e21c5c97be692574efb2b`.

- `recording_read` — original line 10.
- `recording_refresh` — original line 13.
- `recording_prepare` — original line 34.
- `recording_start_workers` — original line 65.
- `recording_finish_writers` — original line 69.
- `recording_cancel` — original line 75.

## recording.h

24 lines; SHA-256 `47f6068f041635ca38eb5fdac6c7c1ee2085e499860a4e902baa3afb45d9d2f1`.

Public types, constants, interfaces or immutable data; see original file.

## recording_writer.c

89 lines; SHA-256 `6b27743305f1db50e832b42e48c68df51b1bd10ab754099682eb18e6b86d6477`.

- `le` — original line 20.
- `header` — original line 21.
- `recording_writer_open` — original line 35.
- `worker` — original line 44.
- `recording_writer_path` — original line 72.
- `recording_writer_start` — original line 73.
- `recording_writer_frames` — original line 79.
- `recording_writer_failed` — original line 80.
- `recording_writer_finish` — original line 81.
- `recording_writer_free` — original line 86.

## recording_writer.h

16 lines; SHA-256 `fc5cd9187239b5672908658aa9fbcc4016ffecba9bd4bec791ac3e010d929656`.

Public types, constants, interfaces or immutable data; see original file.

## sample.h

24 lines; SHA-256 `7158cd9bc8a353a6e272f5d45ed8f17b43627d8b700bc9573a93c08e1e9427ef`.

- `sample_channels` — original line 19.
- `sample_at` — original line 20.

## sample_storage.c

61 lines; SHA-256 `a56b509147d0be0e00fee8fcf4676c8cc54fc0de368afab87744036c8fad6044`.

- `sample_free` — original line 12.
- `sample_clone` — original line 18.
- `sample_map_recording` — original line 29.
- `sample_map_wav` — original line 54.

## sampler.c

104 lines; SHA-256 `40e4f914c06fd75c7d19fab4b2851cc4ef243baf503a9619f2ac177591eb1c25`.

- `at` — original line 8.
- `similarity` — original line 9.
- `stretch` — original line 15.
- `sampler_processing_equal` — original line 43.
- `sampler_valid` — original line 47.
- `sampler_equal` — original line 48.
- `frame_peak` — original line 49.
- `sample_trim` — original line 54.
- `sample_process` — original line 66.

## sampler.h

25 lines; SHA-256 `a93c2fdd377b778f86072eafb96682d6c2fdfb39d8a9d545529b8de74c775ad4`.

Public types, constants, interfaces or immutable data; see original file.

## sampler_voice.c

64 lines; SHA-256 `b997e59e79ac7f7bd60df4aa667b64e8a172818ade2224ef33e7f52aad707a63`.

- `sampler_voice_reset` — original line 5.
- `read_sample` — original line 11.
- `match_score` — original line 17.
- `align_grain` — original line 25.
- `sampler_voice_sample` — original line 43.

## spectrum.c

51 lines; SHA-256 `ddc1fc302461f32037d3a8c584eda90e7ef6b0b0c5e3e5508ac9dca20bbe1af2`.

- `spectrum_reset` — original line 6.
- `spectrum_frequency` — original line 7.
- `transform` — original line 8.
- `spectrum_push` — original line 45.

## spectrum.h

14 lines; SHA-256 `7c0c5103a47fa4b8cdcf5dc4ba921195cefac56fbe0c8d20bc029db1397543d9`.

Public types, constants, interfaces or immutable data; see original file.

## text_fonts.c

102 lines; SHA-256 `04cfa577b856073593c440fafc411efae6e51b14a125b3d7e6fca001f37bdbb0`.

- `unload` — original line 13.
- `text_fonts_close` — original line 16.
- `text_fonts_update` — original line 21.
- `add` — original line 27.
- `text_font` — original line 37.
- `text_pixel_position` — original line 56.
- `advance` — original line 61.
- `glyph_width` — original line 64.
- `text_font_width` — original line 70.
- `text_fit` — original line 80.
- `text_button_size` — original line 97.

## text_fonts.h

15 lines; SHA-256 `8eef4c070fb97165dc8f02db17dabff2eb5b391009fa510ae1da34406814b88d`.

Public types, constants, interfaces or immutable data; see original file.

## theme.c

52 lines; SHA-256 `2f4d002694d87726974703bb0df8cbd9da7e78111fa77b3f366d167f81c06883`.

- `theme_init` — original line 32.
- `ui_surface` — original line 36.
- `ui_frame` — original line 37.
- `theme_foreground` — original line 42.
- `ui_fader_handle` — original line 48.

## theme.h

22 lines; SHA-256 `dea4351716035c882b78e32105a028a04ff7c69426156dfabce478f7616b6cee`.

Public types, constants, interfaces or immutable data; see original file.

## ui_instruments.c

378 lines; SHA-256 `bc85d70bcc16c20c5c360b193ca2c19142c9a171be9c4ab76628ec5c67599935`.

- `sampler_toggle` — original line 4.
- `sampler_switch` — original line 15.
- `fm_graph_update_at` — original line 19.
- `fm_graph_update` — original line 48.
- `fm_control` — original line 52.
- `fm_envelope_graph` — original line 100.
- `fm_legacy_editor` — original line 134.
- `fm_sound_name` — original line 194.
- `fm_footer` — original line 199.
- `fm_controls_row` — original line 207.
- `dx7_envelope_graph` — original line 212.
- `fm_dx_editor` — original line 226.
- `fm_analog_editor` — original line 275.
- `fm_editor` — original line 308.
- `sampler` — original line 316.

## ui_internal.h

451 lines; SHA-256 `403dc71dd79fdd72219f6c23e1b1f96da27640e7d1996c5a3363913147af0199`.

Public types, constants, interfaces or immutable data; see original file.

## ui_menus.c

626 lines; SHA-256 `2683ab5521e481b2b6ff49b5bdf4442b2c7d0e6e5a27514acccd3cabc9a89958`.

- `open_popup` — original line 4.
- `mixer_name` — original line 11.
- `begin_rename` — original line 13.
- `begin_number` — original line 19.
- `control_menu` — original line 24.
- `create_automation` — original line 33.
- `text_input` — original line 56.
- `device_caption` — original line 80.
- `device_button` — original line 82.
- `help_group` — original line 95.
- `help_binding` — original line 99.
- `draw_popup_content` — original line 103.
- `draw_popup` — original line 327.
- `file_picker_sync` — original line 336.
- `preset_action` — original line 341.
- `project_file_action` — original line 358.
- `file_picker_commit` — original line 375.
- `file_picker_submit` — original line 380.
- `draw_file_picker` — original line 389.
- `delete_pattern` — original line 476.
- `draw_context_content` — original line 496.
- `draw_context` — original line 618.

## ui_mixer.c

293 lines; SHA-256 `6ece461ce8d6ddcbb602cbee463c82892fc21ecb709b2a065b122832099a0231`.

- `mixer_input_at` — original line 4.
- `meters_update` — original line 11.
- `spectrum_update` — original line 23.
- `spectrum_draw` — original line 37.
- `eq_x` — original line 46.
- `eq_y` — original line 48.
- `eq_editor` — original line 50.
- `meter_height` — original line 133.
- `meter_draw` — original line 135.
- `selected_meter` — original line 149.
- `mixer` — original line 165.

## ui_piano.c

343 lines; SHA-256 `938ad90178857c17def4fa16a498c1fa8b4d171a1c698983965d8ca751f8424a`.

- `piano_visible_rows` — original line 4.
- `piano_min_top` — original line 6.
- `edit_selection` — original line 8.
- `piano_shift` — original line 33.
- `piano_octave` — original line 61.
- `piano_inactive` — original line 65.
- `piano_extend` — original line 67.
- `piano_gesture_update` — original line 69.
- `piano_note_drag_update` — original line 101.
- `piano_black_key` — original line 125.
- `piano_key_rect` — original line 129.
- `piano_key_at` — original line 138.
- `piano_key_update` — original line 145.
- `piano` — original line 157.

## ui_playlist.c

629 lines; SHA-256 `2ef0174e2ff4598a2caa446dd0de6455c3be306a890d5238297faaf32a884c29`.

- `track_height` — original line 4.
- `track_position` — original line 6.
- `track_at` — original line 12.
- `previews_update` — original line 17.
- `clip_preview` — original line 54.
- `picker_notes` — original line 77.
- `source_color` — original line 96.
- `source_name` — original line 100.
- `automation_curve` — original line 104.
- `draw_picker_drag` — original line 132.
- `draw_browser_drag` — original line 145.
- `tool_button` — original line 152.
- `row_zoom_update` — original line 163.
- `row_zoom_button` — original line 169.
- `ruler_press` — original line 182.
- `timeline_ruler` — original line 199.
- `timeline_grid` — original line 231.
- `follow_view` — original line 251.
- `track_activity_update` — original line 256.
- `clip_edge` — original line 273.
- `picker_double_click` — original line 279.
- `playlist` — original line 297.

## ui_rack.c

188 lines; SHA-256 `90a0f1a57ba2151b570246be6f6268d03b6a2371e74c8806d8f795eb3737dd2c`.

- `rack_channels` — original line 4.
- `rack_channel_at` — original line 10.
- `channel_route` — original line 12.
- `channel_controls` — original line 20.
- `pattern_playback_position` — original line 39.
- `edit_steps` — original line 51.
- `pattern_extend` — original line 53.
- `rack_melodic` — original line 59.
- `paint_rack` — original line 65.
- `rack` — original line 80.
- `pattern_rgb` — original line 176.
- `pattern_color` — original line 180.
- `pattern_pitch_range` — original line 182.

## ui_shell.c

158 lines; SHA-256 `06e57a1af7b62de770fd26c8aa7c7f91c473ac1e5db417d96260b75f6b4177e2`.

- `shortcut_down` — original line 4.
- `command_down` — original line 12.
- `delete_pressed` — original line 17.
- `resource_paths` — original line 25.
- `mouse_callback` — original line 42.
- `mouse_pressed` — original line 47.
- `request_cursor` — original line 49.
- `flush_cursor` — original line 51.
- `playback_loop` — original line 57.
- `captured` — original line 59.
- `ui_scale` — original line 61.
- `playback_start` — original line 63.
- `transport_toggle` — original line 69.
- `transport_stop` — original line 71.
- `draw_editor` — original line 78.
- `navigate_editors` — original line 124.

## ui_state.c

56 lines; SHA-256 `494408c099a2bafa10fbba5915c2d5e57d11cd596ac95d3508efca258543171c`.

Public types, constants, interfaces or immutable data; see original file.

## ui_waveform.c

145 lines; SHA-256 `9bd29dc8f006da30c9618cfc057aac9db3804ce02180f32bdbc75c3effe76fac`.

- `source_rgb` — original line 4.
- `clip_foreground` — original line 11.
- `audio_color` — original line 15.
- `sampler_view` — original line 23.
- `sampler_live_preview` — original line 40.
- `sampler_view_frames` — original line 45.
- `sampler_view_envelope` — original line 50.
- `audio_view_steps` — original line 60.
- `playlist_clip_length` — original line 65.
- `processed_waveform` — original line 75.
- `wave_source` — original line 88.
- `wave_quad` — original line 94.
- `draw_waveform` — original line 99.
- `audio_waveform_at` — original line 123.
- `audio_waveform` — original line 143.

## ui_widgets.c

537 lines; SHA-256 `8daad420731a1c5f93aa7f9947f497b1dae547e8e387122f739f5aa2336f02a4`.

- `fonts_update` — original line 4.
- `text_width` — original line 9.
- `raster_position` — original line 13.
- `label_moving` — original line 17.
- `label` — original line 22.
- `backspace` — original line 26.
- `fit_text` — original line 32.
- `circles_init` — original line 36.
- `circle` — original line 46.
- `icons_init` — original line 50.
- `icon` — original line 171.
- `arcs_init` — original line 178.
- `cables_init` — original line 195.
- `cable` — original line 202.
- `smooth_line` — original line 223.
- `hover` — original line 236.
- `menu_key` — original line 238.
- `menu_keys_begin` — original line 240.
- `menu_keys_end` — original line 268.
- `symbol` — original line 273.
- `button_color` — original line 285.
- `button` — original line 334.
- `drag_button` — original line 338.
- `color_picker` — original line 344.
- `picker_button` — original line 360.
- `automation_display_update` — original line 368.
- `displayed_value` — original line 387.
- `mute_light` — original line 400.
- `editor_button` — original line 429.
- `open_context` — original line 443.
- `preset_directory` — original line 447.
- `capture_control` — original line 463.
- `knob_style` — original line 473.
- `knob` — original line 535.

## waveform.c

80 lines; SHA-256 `a3d568411080d5c022926b98346f36281fc717a7b5a046a773f0af260acfe06e`.

- `merge` — original line 6.
- `scan` — original line 7.
- `waveform_build` — original line 16.
- `waveform_range` — original line 29.
- `blend` — original line 43.
- `envelope_bin` — original line 46.
- `envelope_level` — original line 55.
- `waveform_envelope_region` — original line 60.
- `waveform_envelope` — original line 68.
- `waveform_append` — original line 71.

## waveform.h

13 lines; SHA-256 `c33fcbe64977f1c963884924722ac901285d0612a2dd24914a37aca9984fccb9`.

Public types, constants, interfaces or immutable data; see original file.

## windows.c

69 lines; SHA-256 `eef1232b34e28efb58610dc2e7f1f53356e8349d6ac61ef22c35251c765c0d43`.

- `inside` — original line 4.
- `windows_init` — original line 5.
- `windows_hit` — original line 18.
- `windows_focus` — original line 22.
- `windows_pin` — original line 31.
- `constrain` — original line 32.
- `windows_update` — original line 36.

## windows.h

20 lines; SHA-256 `08017965f74bd637d3a65f0f36d85364aa99f5a8666c1149a1cc5d9da0f0754c`.

Public types, constants, interfaces or immutable data; see original file.
