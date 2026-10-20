# SPDX-License-Identifier: GPL-3.0-only
# App resources and optional shared SDL runtime. All writes are inside bundle.
file(MAKE_DIRECTORY "${BUNDLE}/Contents/Resources" "${BUNDLE}/Contents/Frameworks")
foreach(part fonts presets branding)
  file(COPY "${SOURCE}/assets/${part}" DESTINATION "${BUNDLE}/Contents/Resources")
endforeach()
file(COPY "${SOURCE}/samples" DESTINATION "${BUNDLE}/Contents/Resources")
file(COPY "${SOURCE}/LICENSE" "${SOURCE}/docs/third-party.md"
  DESTINATION "${BUNDLE}/Contents/Resources")
file(COPY "${SOURCE}/docs/licenses" DESTINATION "${BUNDLE}/Contents/Resources")
file(COPY "${SOURCE}/assets/branding/libreloop.icns" DESTINATION "${BUNDLE}/Contents/Resources")
if(SDL_LIBRARY)
get_filename_component(sdl_name "${SDL_LIBRARY}" NAME)
configure_file("${SDL_LIBRARY}" "${BUNDLE}/Contents/Frameworks/${sdl_name}" COPYONLY)
execute_process(COMMAND /usr/bin/otool -L "${EXECUTABLE}"
  OUTPUT_VARIABLE dependencies RESULT_VARIABLE status)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "Cannot inspect development runtime dependencies")
endif()
string(REGEX MATCH "\n[ \t]+([^\n]+${sdl_name}) [(]" reference "${dependencies}")
set(original_reference "${CMAKE_MATCH_1}")
if(NOT original_reference)
  message(FATAL_ERROR "SDL runtime reference missing from development executable")
endif()
execute_process(COMMAND /usr/bin/install_name_tool -change "${original_reference}"
  "@executable_path/../Frameworks/${sdl_name}" "${EXECUTABLE}"
  RESULT_VARIABLE status ERROR_VARIABLE reason)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "Cannot relocate development SDL runtime: ${reason}")
endif()
endif()
execute_process(COMMAND /usr/bin/codesign --force --sign - "${BUNDLE}"
  RESULT_VARIABLE status ERROR_VARIABLE reason)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "Cannot apply local development signature: ${reason}")
endif()
