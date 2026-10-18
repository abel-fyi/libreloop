cmake_minimum_required(VERSION 3.20)
project(libreloop VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
include(CTest)
if(WIN32)
  # Native Windows services and miniaudio use Win32 threads, not winpthreads.
  add_library(libreloop_native_threads INTERFACE)
  add_library(Threads::Threads ALIAS libreloop_native_threads)
else()
  find_package(Threads REQUIRED)
endif()
set(ENGINE_DIR src/engine)
set(INTERACTION_DIR src/interaction)
set(PLATFORM_DIR src/platform)
add_library(engine STATIC
  ${ENGINE_DIR}/engine.c ${ENGINE_DIR}/device.c ${ENGINE_DIR}/instrument.c
  ${ENGINE_DIR}/playback_plan.c ${ENGINE_DIR}/midi.c ${ENGINE_DIR}/project_io.c
  ${ENGINE_DIR}/project_assets.c ${ENGINE_DIR}/project_document.c ${ENGINE_DIR}/preset.c
  ${ENGINE_DIR}/audio_timing.c ${ENGINE_DIR}/automation.c ${ENGINE_DIR}/parameter.c
  ${ENGINE_DIR}/sampler.c ${ENGINE_DIR}/sampler_voice.c ${ENGINE_DIR}/fm_synth.c
  ${ENGINE_DIR}/dx7.c ${ENGINE_DIR}/analog.c ${ENGINE_DIR}/chorus.c
  ${ENGINE_DIR}/reverb.c ${ENGINE_DIR}/tube.c ${ENGINE_DIR}/delay.c ${ENGINE_DIR}/equalizer.c ${ENGINE_DIR}/spectrum.c ${ENGINE_DIR}/effects.c ${ENGINE_DIR}/waveform.c)
set(NATIVE_FILES ${PLATFORM_DIR}/files_posix.c ${PLATFORM_DIR}/browser_files_posix.c ${PLATFORM_DIR}/atomic_file.c)
if(WIN32)
  set(NATIVE_FILES ${PLATFORM_DIR}/files_windows.c ${PLATFORM_DIR}/windows_path_math.c ${PLATFORM_DIR}/browser_files_windows.c ${PLATFORM_DIR}/file_io_windows.c ${PLATFORM_DIR}/atomic_file_windows.c)
endif()
add_library(platform_files STATIC
  ${NATIVE_FILES} ${PLATFORM_DIR}/frame_wait.c ${PLATFORM_DIR}/sample_storage.c ${PLATFORM_DIR}/recording_writer.c)
add_library(utf8 STATIC ${INTERACTION_DIR}/utf8.c)
target_include_directories(utf8 PUBLIC ${INTERACTION_DIR})
add_library(interaction STATIC
  ${INTERACTION_DIR}/browser.c ${INTERACTION_DIR}/arrangement.c ${INTERACTION_DIR}/edit_clipboard.c ${INTERACTION_DIR}/piano.c ${INTERACTION_DIR}/piano_frame.c ${INTERACTION_DIR}/midi_session.c ${INTERACTION_DIR}/record_count_in.c ${INTERACTION_DIR}/text_edit.c ${INTERACTION_DIR}/text_legacy.c ${INTERACTION_DIR}/menu_navigation.c ${INTERACTION_DIR}/shortcuts.c ${INTERACTION_DIR}/ruler.c ${INTERACTION_DIR}/grid.c ${INTERACTION_DIR}/rack.c ${INTERACTION_DIR}/control.c ${INTERACTION_DIR}/automation_view.c ${INTERACTION_DIR}/editor_navigation.c ${INTERACTION_DIR}/scroll_input.c ${INTERACTION_DIR}/input.c
  ${INTERACTION_DIR}/effect_chain.c ${INTERACTION_DIR}/navigation.c ${INTERACTION_DIR}/history.c ${INTERACTION_DIR}/file_chooser.c)
add_library(windows STATIC ${INTERACTION_DIR}/windows.c ${INTERACTION_DIR}/studio_layout.c ${INTERACTION_DIR}/studio_workspace.c)
foreach(target engine platform_files interaction windows)
  target_include_directories(${target} PUBLIC ${ENGINE_DIR} ${INTERACTION_DIR} ${PLATFORM_DIR})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()
# Native file/mapping/writer services stay behind the same engine interfaces.
target_link_libraries(platform_files PUBLIC Threads::Threads)
if(UNIX AND NOT APPLE)
  target_compile_definitions(platform_files PRIVATE _DEFAULT_SOURCE)
endif()
target_link_libraries(engine PUBLIC platform_files)
target_link_libraries(interaction PUBLIC engine utf8)
if(NOT WIN32)
  target_link_libraries(engine PUBLIC m)
  target_link_libraries(windows PUBLIC m)
endif()
add_library(audio STATIC ${PLATFORM_DIR}/audio_miniaudio.c ${PLATFORM_DIR}/recording.c)
target_include_directories(audio PUBLIC ${ENGINE_DIR} ${PLATFORM_DIR})
target_include_directories(audio SYSTEM PRIVATE third_party/miniaudio)
target_link_libraries(audio PUBLIC interaction Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
  target_link_libraries(audio PRIVATE ole32 uuid)
endif()
if(APPLE)
  target_compile_definitions(audio PRIVATE MA_NO_RUNTIME_LINKING)
  target_link_libraries(audio PUBLIC "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
endif()
function(libreloop_test name library)
  if(BUILD_TESTING)
    add_executable(${name}_test tests/${name}_test.c)
    target_link_libraries(${name}_test PRIVATE ${library})
    add_test(NAME ${name} COMMAND ${name}_test)
  endif()
endfunction()
foreach(name reverb reverb_integration tube tube_integration magnetic_knob delay delay_integration engine midi instrument_replace automation pitch_automation audio_clip clip_trim sampler device_baseline chorus equalizer spectrum fm_synth fm_layers dx7 analog preset tempo stereo recording atomic_file project_assets project_document project_format playback_plan device_contract recording_writer waveform)
  libreloop_test(${name} interaction)
endforeach()
foreach(name arrangement edit_clipboard navigation history file_chooser)
  libreloop_test(${name} interaction)
endforeach()
libreloop_test(windows windows)
libreloop_test(studio_layout windows)
libreloop_test(studio_workspace windows)
libreloop_test(piano_resize interaction)
if(TARGET piano_resize_test)
  target_link_libraries(piano_resize_test PRIVATE windows)
endif()
libreloop_test(grid_choice interaction)
libreloop_test(rack_zoom interaction)
if(TARGET rack_zoom_test)
  target_link_libraries(rack_zoom_test PRIVATE windows)
endif()
libreloop_test(studio_navigation interaction)
if(TARGET studio_navigation_test)
  target_link_libraries(studio_navigation_test PRIVATE windows)
endif()
libreloop_test(studio_mixer interaction)
libreloop_test(mixer_view interaction)
libreloop_test(mixer_cables interaction)
libreloop_test(stretch_pitch interaction)
libreloop_test(effect_chain interaction)
libreloop_test(mixer_routing interaction)
libreloop_test(studio_fm_graph interaction)
libreloop_test(instrument_layout interaction)
libreloop_test(velocity_lane interaction)
libreloop_test(transport_readout interaction)
libreloop_test(transport_draw interaction)
libreloop_test(studio_signal_draw interaction)
libreloop_test(automation_viewport interaction)
libreloop_test(mixer_slots interaction)
libreloop_test(rack_fill interaction)
libreloop_test(typing_record interaction)
libreloop_test(record_count_in interaction)
libreloop_test(keyboard_recording interaction)
libreloop_test(midi_settings interaction)
if(TARGET keyboard_recording_test)
 target_link_libraries(keyboard_recording_test PRIVATE windows)
endif()
libreloop_test(pattern_page interaction)
if(TARGET pattern_page_test)
 target_link_libraries(pattern_page_test PRIVATE windows)
endif()
libreloop_test(workspace_preferences interaction)
if(TARGET mixer_slots_test)
 target_link_libraries(mixer_slots_test PRIVATE windows)
 target_link_libraries(workspace_preferences_test PRIVATE windows)
 file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/workspace-prefs)
endif()
foreach(studio_capture transport_draw_test studio_mixer_test studio_fm_graph_test studio_signal_draw_test automation_viewport_test mixer_slots_test workspace_preferences_test rack_fill_test pattern_page_test keyboard_recording_test midi_settings_test)
if(TARGET ${studio_capture})
  target_include_directories(${studio_capture} PRIVATE src/ui src/platform)
  if(APPLE)
    target_link_options(${studio_capture} PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(${studio_capture} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${studio_capture} PRIVATE -Wl,--gc-sections)
  else()
    target_compile_options(${studio_capture} PRIVATE /Gy)
    target_link_options(${studio_capture} PRIVATE /OPT:REF)
  endif()
endif()
endforeach()
foreach(name decoder recording_session audio)
  libreloop_test(${name} audio)
endforeach()
if(TARGET fm_synth_test)
  target_compile_definitions(fm_synth_test PRIVATE FM_FACTORY_PRESET="${CMAKE_CURRENT_SOURCE_DIR}/assets/presets/Electric Piano.llpreset")
endif()
if(TARGET audio_test)
  target_include_directories(audio_test SYSTEM PRIVATE third_party/miniaudio)
  if(APPLE)
    target_compile_definitions(audio_test PRIVATE MA_NO_RUNTIME_LINKING)
  endif()
endif()
if(BUILD_TESTING)
  foreach(name benchmark feature callback)
    add_executable(${name}_capture tests/harness/${name}_capture.c)
    target_link_libraries(${name}_capture PRIVATE audio)
  endforeach()
  target_include_directories(callback_capture SYSTEM PRIVATE third_party/miniaudio)
  if(APPLE)
    target_compile_definitions(callback_capture PRIVATE MA_NO_RUNTIME_LINKING)
  endif()
endif()
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  # The strict Phase 0 comparator remains runnable without the contract flag.
  add_test(NAME original_audio_fixtures
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_fixtures.py
      --build ${CMAKE_CURRENT_BINARY_DIR}
      --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/original
      --report ${CMAKE_CURRENT_BINARY_DIR}/fixture-results.json
      --stretch-duration-contract)
endif()

if(BUILD_TESTING AND APPLE)
  add_library(realtime_guard SHARED tests/realtime_guard_macos.c)
  add_executable(realtime_guard_positive tests/realtime_guard_positive.c)
  target_link_libraries(realtime_guard_positive PRIVATE realtime_guard)
  add_test(NAME realtime_guard_positive
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_realtime_guard.py $<TARGET_FILE:realtime_guard_positive>)
  add_executable(callback_realtime_audit tests/harness/callback_capture.c)
  target_include_directories(callback_realtime_audit PRIVATE tests)
  target_include_directories(callback_realtime_audit SYSTEM PRIVATE third_party/miniaudio)
  target_compile_definitions(callback_realtime_audit PRIVATE MA_NO_RUNTIME_LINKING LIBRELOOP_TEST_RT_GUARD)
  target_link_libraries(callback_realtime_audit PRIVATE audio realtime_guard)
  foreach(mode RANGE 0 6)
    foreach(block 64 512)
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/realtime-audit/${mode}-${block}")
      add_test(NAME realtime_callback_${mode}_${block} COMMAND callback_realtime_audit ${mode} ${block})
      set_tests_properties(realtime_callback_${mode}_${block} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/realtime-audit/${mode}-${block}")
    endforeach()
  endforeach()
endif()
add_executable(benchmark_engine EXCLUDE_FROM_ALL tools/benchmark_engine.c)
target_link_libraries(benchmark_engine PRIVATE engine)

if(BUILD_TESTING)
  add_executable(piano_replay_capture tests/harness/piano_replay_capture.c)
  target_link_libraries(piano_replay_capture PRIVATE interaction)
  add_test(NAME original_piano_replays
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:piano_replay_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/piano)
endif()

if(BUILD_TESTING)
  add_executable(navigation_replay_capture tests/harness/navigation_replay_capture.c)
  target_link_libraries(navigation_replay_capture PRIVATE interaction windows)
  add_test(NAME original_navigation_replays
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_navigation_replays.py
      $<TARGET_FILE:navigation_replay_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/navigation)
endif()
add_library(font_raster STATIC src/ui/font_raster.c src/ui/font_atlas.c src/ui/font_bitmap.c src/ui/font_layout.c)
target_include_directories(font_raster PUBLIC src/ui)
libreloop_test(font_fit font_raster)
target_link_libraries(font_raster PRIVATE utf8)
target_include_directories(font_raster SYSTEM PRIVATE third_party/stb)
if(NOT WIN32)
  target_link_libraries(font_raster PUBLIC m)
endif()
if(BUILD_TESTING)
  add_executable(font_raster_capture tests/harness/font_raster_capture.c)
  target_link_libraries(font_raster_capture PRIVATE font_raster)
  add_test(NAME original_font_raster
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_font_raster.py
      $<TARGET_FILE:font_raster_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/text
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
endif()
include(cmake/dependencies.cmake)
option(LIBRELOOP_EXPERIMENT_CPU_RASTERIZER "Experiment only: CPU rasterizer with SDL blit" OFF)
if(LIBRELOOP_SDL_AVAILABLE)
  add_library(input_sdl STATIC src/platform/input_sdl.c)
  target_include_directories(input_sdl PUBLIC src/platform)
  target_link_libraries(input_sdl PUBLIC interaction PRIVATE ${LIBRELOOP_SDL_TARGET} utf8)
  set(DRAW_IMPLEMENTATION src/platform/draw_sdl.c)
  if(LIBRELOOP_EXPERIMENT_CPU_RASTERIZER)
    set(DRAW_IMPLEMENTATION src/platform/draw_cpu.c)
  endif()
  add_library(draw_sdl STATIC ${DRAW_IMPLEMENTATION} src/platform/draw_text.c)
  target_include_directories(draw_sdl PUBLIC src/platform)
  target_link_libraries(draw_sdl PRIVATE ${LIBRELOOP_SDL_TARGET} font_raster)
  if(NOT WIN32)
    target_link_libraries(draw_sdl PRIVATE m)
  endif()
endif()

if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(draw_sdl_test tests/draw_sdl_test.c)
  target_link_libraries(draw_sdl_test PRIVATE draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME draw_sdl COMMAND draw_sdl_test)
endif()

if(BUILD_TESTING)
  add_executable(text_layout_capture tests/harness/text_layout_capture.c)
  target_link_libraries(text_layout_capture PRIVATE font_raster)
  add_test(NAME original_text_layout
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_text_layout.py
      $<TARGET_FILE:text_layout_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/text-layout
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
endif()

add_library(text_fonts STATIC src/ui/text_fonts.c)
target_include_directories(text_fonts PUBLIC src/ui src/platform)
target_link_libraries(text_fonts PRIVATE font_raster utf8)
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(large_caption_test tests/large_caption_test.c)
  target_link_libraries(large_caption_test PRIVATE text_fonts draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME large_caption COMMAND large_caption_test ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
  add_test(NAME bitmap_caption COMMAND large_caption_test ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/PxPlus_IBM_VGA_8x16.ttf em)
  add_executable(text_cache_capture tests/harness/text_cache_capture.c)
  target_link_libraries(text_cache_capture PRIVATE text_fonts draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME original_text_cache
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_text_layout.py
      $<TARGET_FILE:text_cache_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/text-cache
      ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
endif()

if(BUILD_TESTING)
  add_executable(input_capture tests/harness/input_capture.c)
  target_link_libraries(input_capture PRIVATE interaction)
  add_test(NAME original_input
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:input_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/input)
endif()

if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(input_sdl_test tests/input_sdl_test.c)
  target_link_libraries(input_sdl_test PRIVATE input_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME input_sdl COMMAND input_sdl_test)
endif()

if(BUILD_TESTING)
  add_executable(piano_press_capture tests/harness/piano_press_capture.c)
  target_link_libraries(piano_press_capture PRIVATE interaction)
  add_test(NAME original_piano_presses
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:piano_press_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/piano-press)
endif()

add_library(theme STATIC src/ui/theme.c src/ui/color.c)
target_include_directories(theme PUBLIC src/ui src/platform)
if(NOT WIN32)
  target_link_libraries(theme PRIVATE m)
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(theme_test tests/theme_test.c)
  target_link_libraries(theme_test PRIVATE theme draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME theme COMMAND theme_test)
  add_test(NAME theme_render COMMAND theme_test --render)
endif()

if(BUILD_TESTING)
  libreloop_test(browser interaction)
endif()

if(BUILD_TESTING AND UNIX)
  add_executable(browser_capture tests/harness/browser_capture.c)
  target_link_libraries(browser_capture PRIVATE interaction)
  if(NOT APPLE)
    target_compile_definitions(browser_capture PRIVATE _DEFAULT_SOURCE)
  endif()
  add_test(NAME original_browser
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_browser.py
      $<TARGET_FILE:browser_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/browser)
endif()

if(LIBRELOOP_SDL_AVAILABLE)
  add_library(window_sdl STATIC src/platform/window_sdl.c)
  target_include_directories(window_sdl PUBLIC src/platform)
  target_link_libraries(window_sdl PUBLIC interaction PRIVATE input_sdl draw_sdl ${LIBRELOOP_SDL_TARGET})
  if(APPLE)
    enable_language(OBJC)
    target_sources(window_sdl PRIVATE src/platform/input_macos.m)
    target_link_libraries(window_sdl PRIVATE "-framework Cocoa")
  endif()
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(window_sdl_test tests/window_sdl_test.c)
  target_link_libraries(window_sdl_test PRIVATE window_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME window_sdl COMMAND window_sdl_test)
  set_tests_properties(window_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
endif()

add_library(widget_pixels STATIC src/ui/widget_pixels.c)
target_include_directories(widget_pixels PUBLIC src/ui src/platform)
if(NOT WIN32)
  target_link_libraries(widget_pixels PRIVATE m)
endif()
if(BUILD_TESTING)
  add_executable(widget_pixels_capture tests/harness/widget_pixels_capture.c)
  target_link_libraries(widget_pixels_capture PRIVATE widget_pixels)
  add_test(NAME original_widget_pixels
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:widget_pixels_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/widget-pixels)
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(color_capture tests/harness/color_capture.c)
  target_link_libraries(color_capture PRIVATE theme draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME original_colors
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:color_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/colors)
endif()

add_library(ui_render STATIC src/ui/render.c src/ui/icons.c src/ui/shapes.c)
target_include_directories(ui_render PUBLIC src/ui src/platform src/interaction)
target_link_libraries(ui_render PRIVATE text_fonts widget_pixels theme)
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(scissor_capture tests/harness/scissor_capture.c)
  target_link_libraries(scissor_capture PRIVATE ui_render draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME original_scissor
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:scissor_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scissor)
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(ui_render_test tests/ui_render_test.c)
  target_link_libraries(ui_render_test PRIVATE ui_render draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME ui_render COMMAND ui_render_test ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
endif()
add_library(piano_draw STATIC src/ui/piano_draw.c)
target_include_directories(piano_draw PUBLIC src/ui src/platform src/interaction)
target_link_libraries(piano_draw PUBLIC interaction PRIVATE ui_render theme)
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(piano_draw_test tests/piano_draw_test.c)
  target_link_libraries(piano_draw_test PRIVATE piano_draw draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME piano_draw COMMAND piano_draw_test ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
endif()
add_library(waveform_draw STATIC src/ui/waveform_draw.c)
target_include_directories(waveform_draw PUBLIC src/ui src/platform)
target_link_libraries(waveform_draw PUBLIC engine PRIVATE ui_render theme)
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(waveform_capture tests/harness/waveform_capture.c)
  target_link_libraries(waveform_capture PRIVATE waveform_draw draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME original_waveform_geometry
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:waveform_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/waveform)
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(icon_pack_capture tests/harness/icon_pack_capture.c)
  add_executable(classic_icons_test tests/classic_icons_test.c)
  target_link_libraries(classic_icons_test PRIVATE ui_render draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME classic_icons COMMAND classic_icons_test)
  target_link_libraries(icon_pack_capture PRIVATE ui_render draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME original_icon_packing
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:icon_pack_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/icon-pack)
endif()
if(BUILD_TESTING)
  add_executable(ruler_capture tests/harness/ruler_capture.c)
  target_link_libraries(ruler_capture PRIVATE interaction)
  add_test(NAME original_ruler
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:ruler_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/ruler)
endif()
if(BUILD_TESTING)
  add_executable(rack_capture tests/harness/rack_capture.c)
  target_link_libraries(rack_capture PRIVATE interaction)
  add_test(NAME original_rack
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:rack_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/rack)
endif()
if(LIBRELOOP_SDL_AVAILABLE)
  add_library(thread_sdl STATIC src/platform/thread_sdl.c)
  target_include_directories(thread_sdl PUBLIC src/platform)
  target_link_libraries(thread_sdl PRIVATE ${LIBRELOOP_SDL_TARGET})
  add_library(sampler_service STATIC src/app/sampler_service.c)
  target_include_directories(sampler_service PUBLIC src/app)
  target_link_libraries(sampler_service PUBLIC engine PRIVATE thread_sdl)
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(sampler_service_capture tests/harness/sampler_service_capture.c)
  target_link_libraries(sampler_service_capture PRIVATE sampler_service)
  if(UNIX AND NOT APPLE)
    target_compile_definitions(sampler_service_capture PRIVATE _DEFAULT_SOURCE)
  endif()
  add_test(NAME original_sampler_service
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:sampler_service_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sampler-service)
endif()
if(BUILD_TESTING)
  add_executable(control_capture tests/harness/control_capture.c)
  target_link_libraries(control_capture PRIVATE interaction)
  add_test(NAME original_controls
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:control_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/control)
endif()
add_library(controls_draw STATIC src/ui/controls_draw.c)
target_include_directories(controls_draw PUBLIC src/ui src/platform)
target_link_libraries(controls_draw PUBLIC interaction PRIVATE ui_render theme)
if(BUILD_TESTING)
  add_executable(automation_view_capture tests/harness/automation_view_capture.c)
  target_link_libraries(automation_view_capture PRIVATE interaction)
  add_test(NAME original_automation_view
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:automation_view_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/automation-view)
endif()
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(knob_capture tests/harness/knob_capture.c)
  target_link_libraries(knob_capture PRIVATE controls_draw draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME original_knob_drawing
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:knob_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/knob-drawing)
endif()
add_library(editor_draw STATIC src/ui/editor_draw.c)
target_include_directories(editor_draw PUBLIC src/ui src/platform)
target_link_libraries(editor_draw PUBLIC windows PRIVATE ui_render theme)
if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(editor_piano_scene_test tests/editor_piano_scene_test.c)
  target_link_libraries(editor_piano_scene_test PRIVATE editor_draw piano_draw draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME editor_piano_scene COMMAND editor_piano_scene_test ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf)
endif()

if(BUILD_TESTING)
  add_executable(piano_frame_capture tests/harness/piano_frame_capture.c)
  target_link_libraries(piano_frame_capture PRIVATE interaction)
  add_test(NAME original_piano_frames
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:piano_frame_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/piano-frames)
endif()

if(BUILD_TESTING)
  add_executable(midi_session_capture tests/harness/midi_session_capture.c)
  target_link_libraries(midi_session_capture PRIVATE interaction)
  add_test(NAME original_midi_session
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:midi_session_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/midi-session)
endif()

# Original OS-provided MIDI backends; no additional third-party dependency.
# Desktop composition links this target. Headless engine builds need no devices.
if(APPLE OR WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_library(midi_input STATIC EXCLUDE_FROM_ALL src/platform/midi_input.c)
  target_include_directories(midi_input PUBLIC src/platform src/engine)
  target_link_libraries(midi_input PUBLIC engine Threads::Threads)
  if(APPLE)
    target_link_libraries(midi_input PRIVATE "-framework CoreMIDI" "-framework CoreFoundation")
  elseif(WIN32)
    target_link_libraries(midi_input PRIVATE winmm user32)
  else()
    find_package(ALSA REQUIRED)
    target_compile_definitions(midi_input PRIVATE _POSIX_C_SOURCE=200809L)
    target_include_directories(midi_input PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(midi_input PRIVATE ${ALSA_LIBRARIES})
  endif()
endif()

if(LIBRELOOP_SDL_AVAILABLE)
  add_library(frontend_host STATIC src/platform/frontend_host.c)
  target_include_directories(frontend_host PUBLIC src/ui/frontend src/ui src/platform)
  target_include_directories(frontend_host SYSTEM PRIVATE third_party/stb)
  target_link_libraries(frontend_host PUBLIC window_sdl draw_sdl ui_render text_fonts thread_sdl PRIVATE ${LIBRELOOP_SDL_TARGET})
  add_executable(libreloop
    src/ui/frontend/main.c src/ui/frontend/ui_state.c src/ui/frontend/ui_shell.c
    src/ui/frontend/ui_widgets.c src/ui/frontend/ui_menus.c src/ui/frontend/ui_rack.c
    src/ui/frontend/ui_playlist.c src/ui/frontend/ui_piano.c src/ui/frontend/ui_mixer.c
    src/ui/frontend/ui_instruments.c src/ui/frontend/ui_waveform.c
    src/ui/frontend/app_project.c src/ui/frontend/app_midi.c src/ui/frontend/app_recording.c)
  target_include_directories(libreloop PRIVATE src/ui/frontend src/ui)
  target_compile_definitions(libreloop PRIVATE
    LIBRELOOP_VERSION="${PROJECT_VERSION}-dev"
    LIBRELOOP_SAMPLES="${CMAKE_CURRENT_SOURCE_DIR}/samples"
    LIBRELOOP_FONT="${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/LiberationSans-Regular.ttf"
    LIBRELOOP_STUDIO_FONT="${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/PxPlus_IBM_VGA_8x16.ttf"
    LIBRELOOP_LOGO="${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/logo.png")
  target_link_libraries(libreloop PRIVATE frontend_host audio midi_input windows theme)
  if(MSVC)
    target_compile_options(libreloop PRIVATE /W4)
  else()
    target_compile_options(libreloop PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  if(WIN32)
    if(MSVC)
      target_link_options(libreloop PRIVATE /STACK:8388608)
    else()
      target_link_options(libreloop PRIVATE -Wl,--stack,8388608)
    endif()
  endif()
  if(APPLE)
    if(LIBRELOOP_VALIDATE_SYSTEM_SDL)
      set(LIBRELOOP_MINIMUM_MACOS_VERSION "26.0")
    elseif(CMAKE_OSX_DEPLOYMENT_TARGET)
      set(LIBRELOOP_MINIMUM_MACOS_VERSION "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
      message(FATAL_ERROR "Set CMAKE_OSX_DEPLOYMENT_TARGET for the vendored macOS desktop; do not infer an untested minimum")
    endif()
    set_target_properties(libreloop PROPERTIES
      OUTPUT_NAME LibreLoop MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME LibreLoop
      MACOSX_BUNDLE_GUI_IDENTIFIER org.libreloop.rewrite
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
      MACOSX_BUNDLE_ICON_FILE libreloop.icns
      MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set_property(TARGET libreloop APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/package-macos.cmake")
    set(_sdl_runtime "")
    if(LIBRELOOP_SDL_IS_SHARED)
      set(_sdl_runtime "$<TARGET_FILE:${LIBRELOOP_SDL_TARGET}>")
    endif()
    add_custom_command(TARGET libreloop POST_BUILD
      COMMAND ${CMAKE_COMMAND}
        "-DBUNDLE=$<TARGET_BUNDLE_DIR:libreloop>"
        "-DEXECUTABLE=$<TARGET_FILE:libreloop>"
        "-DSDL_LIBRARY=${_sdl_runtime}"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/package-macos.cmake"
      VERBATIM)
  endif()
endif()

if(BUILD_TESTING AND TARGET libreloop)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/desktop-smoke")
  add_test(NAME desktop_smoke COMMAND libreloop --smoke)
  set_tests_properties(desktop_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/desktop-smoke"
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software"
    TIMEOUT 30)
endif()

if(BUILD_TESTING)
  add_executable(frontend_piano_capture tests/harness/frontend_piano_capture.c)
  target_include_directories(frontend_piano_capture PRIVATE src/ui)
  target_link_libraries(frontend_piano_capture PRIVATE interaction windows)
  if(APPLE)
    target_link_options(frontend_piano_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_piano_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_piano_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME original_frontend_piano
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:frontend_piano_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/piano-press)
endif()

if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE)
  add_executable(host_helpers_capture tests/harness/host_helpers_capture.c)
  target_link_libraries(host_helpers_capture PRIVATE frontend_host)
  add_test(NAME original_host_helpers
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:host_helpers_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/host-helpers)
endif()

if(BUILD_TESTING)
  add_executable(frontend_navigation_capture tests/harness/frontend_navigation_capture.c)
  target_include_directories(frontend_navigation_capture PRIVATE src/ui)
  target_link_libraries(frontend_navigation_capture PRIVATE interaction windows)
  if(APPLE)
    target_link_options(frontend_navigation_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_navigation_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_navigation_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME original_frontend_navigation
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_navigation_replays.py
      $<TARGET_FILE:frontend_navigation_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/navigation)
endif()

if(BUILD_TESTING AND TARGET libreloop)
  add_test(NAME studio_startup_sizes
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/studio_sizes_test.py
      $<TARGET_FILE:libreloop>)
  add_test(NAME visual_preview_state
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/visual_preview_compare.py
      $<TARGET_FILE:libreloop>)
  set_tests_properties(visual_preview_state PROPERTIES TIMEOUT 60)
endif()

if(BUILD_TESTING)
  add_executable(windows_paths_test tests/windows_paths_test.c src/platform/windows_path_math.c)
  target_include_directories(windows_paths_test PRIVATE src/platform)
  add_test(NAME windows_paths COMMAND windows_paths_test)
endif()

if(WIN32)
  target_link_libraries(text_fonts PRIVATE platform_files)
endif()

include(cmake/experiments.cmake)

if(LIBRELOOP_SDL_AVAILABLE AND UNIX)
  add_executable(benchmark_ui EXCLUDE_FROM_ALL tools/benchmark_ui.c)
  target_link_libraries(benchmark_ui PRIVATE editor_draw piano_draw waveform_draw draw_sdl ${LIBRELOOP_SDL_TARGET})
  if(LIBRELOOP_EXPERIMENT_UI_OPT)
    target_compile_options(benchmark_ui PRIVATE "-${LIBRELOOP_EXPERIMENT_UI_OPT}")
  endif()
  if(LIBRELOOP_EXPERIMENT_LTO)
    set_property(TARGET benchmark_ui PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

if(BUILD_TESTING AND LIBRELOOP_SDL_AVAILABLE AND LIBRELOOP_EXPERIMENT_CPU_RASTERIZER)
  add_executable(draw_cpu_test tests/draw_cpu_test.c)
  target_link_libraries(draw_cpu_test PRIVATE draw_sdl ${LIBRELOOP_SDL_TARGET})
  add_test(NAME draw_cpu_experiment COMMAND draw_cpu_test)
endif()

add_executable(compare_images EXCLUDE_FROM_ALL tools/compare_images.c)
target_include_directories(compare_images SYSTEM PRIVATE third_party/stb)
if(NOT WIN32)
  target_link_libraries(compare_images PRIVATE m)
endif()

if(BUILD_TESTING)
  foreach(kind rack ruler)
    add_executable(frontend_${kind}_capture tests/harness/frontend_${kind}_capture.c)
    target_include_directories(frontend_${kind}_capture PRIVATE src/ui)
    target_link_libraries(frontend_${kind}_capture PRIVATE interaction windows)
    if(APPLE)
      target_link_options(frontend_${kind}_capture PRIVATE -Wl,-dead_strip)
    elseif(NOT MSVC)
      target_compile_options(frontend_${kind}_capture PRIVATE -ffunction-sections -fdata-sections)
      target_link_options(frontend_${kind}_capture PRIVATE -Wl,--gc-sections)
    endif()
    add_test(NAME original_frontend_${kind}
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
        $<TARGET_FILE:frontend_${kind}_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/${kind})
  endforeach()
endif()

if(BUILD_TESTING)
  foreach(kind binding_conditions frontend_marker_reset frontend_rack_conditions)
    add_executable(${kind}_capture tests/harness/${kind}_capture.c)
    target_include_directories(${kind}_capture PRIVATE src/ui)
    target_link_libraries(${kind}_capture PRIVATE interaction windows)
    if(APPLE)
      target_link_options(${kind}_capture PRIVATE -Wl,-dead_strip)
    elseif(NOT MSVC)
      target_compile_options(${kind}_capture PRIVATE -ffunction-sections -fdata-sections)
      target_link_options(${kind}_capture PRIVATE -Wl,--gc-sections)
    endif()
    add_test(NAME original_${kind}
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
        $<TARGET_FILE:${kind}_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/${kind})
  endforeach()
endif()

if(BUILD_TESTING)
  add_executable(frontend_clipboard_history_capture tests/harness/frontend_clipboard_history_capture.c)
  target_include_directories(frontend_clipboard_history_capture PRIVATE src/ui)
  target_link_libraries(frontend_clipboard_history_capture PRIVATE interaction windows)
  if(APPLE)
    target_link_options(frontend_clipboard_history_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_clipboard_history_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_clipboard_history_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME original_frontend_clipboard_history
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:frontend_clipboard_history_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/clipboard-history)
endif()

if(BUILD_TESTING)
  add_executable(shortcuts_capture tests/harness/shortcuts_capture.c)
  target_link_libraries(shortcuts_capture PRIVATE interaction)
  add_test(NAME original_shortcuts
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:shortcuts_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/shortcuts)
endif()

if(BUILD_TESTING)
  add_executable(frontend_drop_capture tests/harness/frontend_drop_capture.c)
  target_include_directories(frontend_drop_capture PRIVATE src/ui)
  target_link_libraries(frontend_drop_capture PRIVATE interaction windows)
  if(APPLE)
    target_link_options(frontend_drop_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_drop_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_drop_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME studio_frontend_drop
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:frontend_drop_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/drop --studio)
  add_test(NAME original_frontend_drop
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:frontend_drop_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/drop)
endif()

if(BUILD_TESTING)
  add_executable(frontend_recording_capture tests/harness/frontend_recording_capture.c)
  target_include_directories(frontend_recording_capture PRIVATE src/ui)
  target_link_libraries(frontend_recording_capture PRIVATE interaction windows)
  add_test(NAME original_frontend_recording
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:frontend_recording_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/recording)
endif()

include(cmake/distribution.cmake)
if(BUILD_TESTING AND APPLE AND TARGET libreloop)
  add_test(NAME installed_macos_bundle
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/install_bundle_test.py
      ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_COMMAND})
  set_tests_properties(installed_macos_bundle PROPERTIES TIMEOUT 90)
endif()

if(BUILD_TESTING)
  add_executable(text_edit_capture tests/harness/text_edit_capture.c)
  target_link_libraries(text_edit_capture PRIVATE interaction)
  add_test(NAME original_text_edit
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:text_edit_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/text-edit)
  add_executable(frontend_text_edit_capture tests/harness/frontend_text_edit_capture.c)
  target_include_directories(frontend_text_edit_capture PRIVATE src/ui)
  target_link_libraries(frontend_text_edit_capture PRIVATE interaction windows)
  if(APPLE)
    target_link_options(frontend_text_edit_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_text_edit_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_text_edit_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME original_frontend_text_edit
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:frontend_text_edit_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/text-edit)
  add_test(NAME original_frontend_literals
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_literals_test.py
      ${CMAKE_CURRENT_SOURCE_DIR})
  add_executable(popup_value_capture tests/harness/popup_value_capture.c)
  target_link_libraries(popup_value_capture PRIVATE interaction)
  add_test(NAME original_popup_values
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:popup_value_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/popup-values)
  foreach(kind menu_navigation frontend_menu_navigation)
    add_executable(${kind}_capture tests/harness/${kind}_capture.c)
    target_include_directories(${kind}_capture PRIVATE src/ui)
    target_link_libraries(${kind}_capture PRIVATE interaction windows)
    if(APPLE)
      target_link_options(${kind}_capture PRIVATE -Wl,-dead_strip)
    elseif(NOT MSVC)
      target_compile_options(${kind}_capture PRIVATE -ffunction-sections -fdata-sections)
      target_link_options(${kind}_capture PRIVATE -Wl,--gc-sections)
    endif()
    add_test(NAME original_${kind}
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
        $<TARGET_FILE:${kind}_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/menu-navigation)
  endforeach()
  add_executable(frontend_file_picker_capture tests/harness/frontend_file_picker_capture.c)
  target_include_directories(frontend_file_picker_capture PRIVATE src/ui)
  target_link_libraries(frontend_file_picker_capture PRIVATE interaction windows theme)
  if(APPLE)
    target_link_options(frontend_file_picker_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_file_picker_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_file_picker_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME original_frontend_file_picker
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:frontend_file_picker_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/file-picker)
  add_executable(directory_capture tests/harness/directory_capture.c)
  target_link_libraries(directory_capture PRIVATE platform_files)
  add_test(NAME original_directory_creation
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:directory_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/directory)
  if(TARGET frontend_host)
    add_executable(frontend_directory_capture tests/harness/frontend_directory_capture.c)
    target_link_libraries(frontend_directory_capture PRIVATE frontend_host)
    add_test(NAME original_frontend_directory_creation
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
        $<TARGET_FILE:frontend_directory_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/directory)
  endif()
endif()

if(BUILD_TESTING)
  add_executable(path_text_capture tests/harness/path_text_capture.c)
  target_link_libraries(path_text_capture PRIVATE interaction)
  add_test(NAME original_path_text
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:path_text_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/path-text)
  if(TARGET frontend_host)
    add_executable(frontend_path_text_capture tests/harness/frontend_path_text_capture.c)
    target_link_libraries(frontend_path_text_capture PRIVATE frontend_host)
    add_test(NAME original_frontend_path_text
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
        $<TARGET_FILE:frontend_path_text_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/path-text)
  endif()
endif()

if(BUILD_TESTING AND TARGET frontend_host)
  add_executable(clipboard_sdl_test tests/clipboard_sdl_test.c)
  target_link_libraries(clipboard_sdl_test PRIVATE frontend_host interaction ${LIBRELOOP_SDL_TARGET})
  add_test(NAME clipboard_sdl COMMAND clipboard_sdl_test)
  set_tests_properties(clipboard_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 30)
endif()

if(BUILD_TESTING AND APPLE)
  add_executable(frame_wait_capture tests/harness/frame_wait_capture.c)
  target_include_directories(frame_wait_capture PRIVATE src/platform)
  add_test(NAME original_frame_wait
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_input.py
      $<TARGET_FILE:frame_wait_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/frame-wait)
endif()

if(BUILD_TESTING)
  add_executable(frontend_project_commit_capture tests/harness/frontend_project_commit_capture.c)
  target_include_directories(frontend_project_commit_capture PRIVATE src/ui)
  target_link_libraries(frontend_project_commit_capture PRIVATE interaction windows)
  if(APPLE)
    target_link_options(frontend_project_commit_capture PRIVATE -Wl,-dead_strip)
  elseif(NOT MSVC)
    target_compile_options(frontend_project_commit_capture PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(frontend_project_commit_capture PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME original_frontend_project_commit
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_piano_replays.py
      $<TARGET_FILE:frontend_project_commit_capture> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/interaction/project-commit)
endif()
