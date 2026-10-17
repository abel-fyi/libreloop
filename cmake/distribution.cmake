# SPDX-License-Identifier: GPL-3.0-only
# Relocatable desktop layout; no deployment/publishing operation.
if(TARGET libreloop)
  if(APPLE)
    install(TARGETS libreloop BUNDLE DESTINATION .)
    # CMake's install-time runtime-path edits invalidate the build signature.
    # Sign the final staged bundle only after those edits have completed.
    install(CODE [[
      execute_process(COMMAND /usr/bin/codesign --force --sign -
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/LibreLoop.app"
        RESULT_VARIABLE status ERROR_VARIABLE reason)
      if(NOT status EQUAL 0)
        message(FATAL_ERROR "Cannot sign installed LibreLoop bundle: ${reason}")
      endif()
    ]])
  else()
    # Keep this explicit layout aligned with resource_paths: bin/../share.
    install(TARGETS libreloop RUNTIME DESTINATION bin)
    install(DIRECTORY assets/fonts assets/presets assets/branding samples DESTINATION share/libreloop)
    install(FILES LICENSE docs/third-party.md DESTINATION share/libreloop)
    install(DIRECTORY docs/licenses DESTINATION share/libreloop)
    if(LIBRELOOP_SDL_IS_SHARED)
      if(WIN32)
        install(FILES "$<TARGET_FILE:${LIBRELOOP_SDL_TARGET}>" DESTINATION bin)
      else()
        install(FILES "$<TARGET_FILE:${LIBRELOOP_SDL_TARGET}>" DESTINATION lib/libreloop)
        # The loader asks for the SONAME, which can differ from the real file.
        install(FILES "$<TARGET_FILE:${LIBRELOOP_SDL_TARGET}>"
          DESTINATION lib/libreloop RENAME "$<TARGET_SONAME_FILE_NAME:${LIBRELOOP_SDL_TARGET}>")
        set_property(TARGET libreloop PROPERTY INSTALL_RPATH "$ORIGIN/../lib/libreloop")
      endif()
    endif()
  endif()
endif()
