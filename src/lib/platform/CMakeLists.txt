# SPDX-FileCopyrightText: (C) 2024 - 2026 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

if(UNIX AND NOT APPLE AND NOT ANDROID)
  pkg_check_modules(LIBEI REQUIRED QUIET "libei-1.0 >= ${REQUIRED_LIBEI_VERSION}")
  message(STATUS "libei version: ${LIBEI_VERSION}")

  pkg_check_modules(LIBPORTAL REQUIRED QUIET "libportal >= ${REQUIRED_LIBPORTAL_VERSION}")
  message(STATUS "libportal version: ${LIBPORTAL_VERSION}")

  # Check if we have the restore ability in the IC portal
  include(CMakePushCheckState)
  include(CheckSymbolExists)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};${LIBPORTAL_INCLUDE_DIRS};")
  set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES};${LIBPORTAL_LINK_LIBRARIES};")
  check_symbol_exists(xdp_input_capture_session_get_restore_token "libportal/inputcapture.h" HAVE_IC_RESTORE)
  check_symbol_exists(xdp_session_request_clipboard "libportal/portal-helpers.h;libportal/clipboard.h" HAVE_LP_CLIPBOARD)
  cmake_pop_check_state()
endif()

if(WIN32)
  set(PLATFORM_SOURCES
    IMSWindowsClipboardFacade.h
    MSWindowsClipboard.cpp
    MSWindowsClipboard.h
    MSWindowsClipboardAnyTextConverter.cpp
    MSWindowsClipboardAnyTextConverter.h
    MSWindowsClipboardBitmapConverter.cpp
    MSWindowsClipboardBitmapConverter.h
    MSWindowsClipboardFacade.cpp
    MSWindowsClipboardFacade.h
    MSWindowsClipboardHTMLConverter.cpp
    MSWindowsClipboardHTMLConverter.h
    MSWindowsClipboardUTF16Converter.cpp
    MSWindowsClipboardUTF16Converter.h
    MSWindowsDebugOutputter.cpp
    MSWindowsDebugOutputter.h
    MSWindowsDesks.cpp
    MSWindowsDesks.h
    MSWindowsEventQueueBuffer.cpp
    MSWindowsEventQueueBuffer.h
    MSWindowsHook.cpp
    MSWindowsHook.h
    MSWindowsKeyState.cpp
    MSWindowsKeyState.h
    MSWindowsPowerManager.cpp
    MSWindowsPowerManager.h
    MSWindowsProcess.cpp
    MSWindowsProcess.h
    MSWindowsScreen.cpp
    MSWindowsScreen.h
    MSWindowsScreenSaver.cpp
    MSWindowsScreenSaver.h
    MSWindowsSession.cpp
    MSWindowsSession.h
    MSWindowsWatchdog.cpp
    MSWindowsWatchdog.h
  )
elseif(APPLE)
  set(PLATFORM_SOURCES
    IOSXKeyResource.cpp
    IOSXKeyResource.h
    OSXAutoTypes.h
    OSXClipboard.cpp
    OSXClipboard.h
    OSXClipboardAnyBitmapConverter.cpp
    OSXClipboardAnyBitmapConverter.h
    OSXClipboardAnyTextConverter.cpp
    OSXClipboardAnyTextConverter.h
    OSXClipboardBMPConverter.cpp
    OSXClipboardBMPConverter.h
    OSXClipboardHTMLConverter.cpp
    OSXClipboardHTMLConverter.h
    OSXClipboardTextConverter.cpp
    OSXClipboardTextConverter.h
    OSXClipboardUTF8Converter.cpp
    OSXClipboardUTF8Converter.h
    OSXClipboardUTF16Converter.cpp
    OSXClipboardUTF16Converter.h
    OSXCocoaApp.h
    OSXCocoaApp.m
    OSXEventQueueBuffer.cpp
    OSXEventQueueBuffer.h
    OSXKeyLayoutResource.cpp
    OSXKeyLayoutResource.h
    OSXKeyState.cpp
    OSXKeyState.h
    OSXMediaKeySupport.h
    OSXMediaKeySupport.m
    OSXPasteboardPeeker.h
    OSXPasteboardPeeker.m
    OSXPowerManager.cpp
    OSXPowerManager.h
    OSXScreen.h
    OSXScreen.mm
    OSXScreenSaver.cpp
    OSXScreenSaver.h
    OSXScreenSaverControl.h
    OSXScreenSaverUtil.h
    OSXScreenSaverUtil.m
  )
elseif(UNIX)
  set(PLATFORM_SOURCES)
  if(NOT ANDROID)
    list(APPEND PLATFORM_SOURCES
      XDGPortalRegistry.h
      XDGPowerManager.cpp
      XDGPowerManager.h
      XDGKeyUtil.h
      XDGKeyUtil.cpp
    )
  endif()
  if(BUILD_ADB_CLIENT_SUPPORT)
    list(APPEND PLATFORM_SOURCES
      AdbInputBridge.cpp
      AdbInputBridge.h
      AdbKeyState.cpp
      AdbKeyState.h
      AdbScreen.cpp
      AdbScreen.h
    )
  endif()
  if (BUILD_X11_SUPPORT)
    configure_file(XWindowsConfig.h.in XWindowsConfig.h @ONLY)
    list(APPEND PLATFORM_SOURCES
      XWindowsClipboard.cpp
      XWindowsClipboard.h
      XWindowsClipboardAnyBitmapConverter.cpp
      XWindowsClipboardAnyBitmapConverter.h
      XWindowsClipboardBMPConverter.cpp
      XWindowsClipboardBMPConverter.h
      XWindowsClipboardHTMLConverter.cpp
      XWindowsClipboardHTMLConverter.h
      XWindowsClipboardTextConverter.cpp
      XWindowsClipboardTextConverter.h
      XWindowsClipboardUCS2Converter.cpp
      XWindowsClipboardUCS2Converter.h
      XWindowsClipboardUTF8Converter.cpp
      XWindowsClipboardUTF8Converter.h
      XWindowsEventQueueBuffer.cpp
      XWindowsEventQueueBuffer.h
      XWindowsKeyState.cpp
      XWindowsKeyState.h
      XWindowsScreen.cpp
      XWindowsScreen.h
      XWindowsScreenSaver.cpp
      XWindowsScreenSaver.h
      XWindowsUtil.cpp
      XWindowsUtil.h
      ${CMAKE_CURRENT_BINARY_DIR}/XWindowsConfig.h
    )
  endif()

  if(LIBEI_FOUND)
    list(APPEND PLATFORM_SOURCES
      EiClipboard.cpp
      EiClipboard.h
      EiEventQueueBuffer.cpp
      EiEventQueueBuffer.h
      EiKeyState.cpp
      EiKeyState.h
      EiScreen.cpp
      EiScreen.h
    )
    # The Portal sources also require EI.
    if(LIBPORTAL_FOUND)
      list(APPEND PLATFORM_SOURCES
        PortalInputCapture.cpp
        PortalInputCapture.h
        PortalRemoteDesktop.cpp
        PortalRemoteDesktop.h
      )
      if(HAVE_LP_CLIPBOARD)
        list(APPEND PLATFORM_SOURCES
          PortalClipboard.cpp
          PortalClipboard.h
        )
      endif()
    endif()
  endif()
endif()

# wayland.h is included to check for wayland support
add_library(platform STATIC ${PLATFORM_SOURCES})

target_link_libraries(platform PUBLIC client ${libs})

if(UNIX)
  target_link_libraries(platform PRIVATE net app)

  if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(platform PUBLIC ${COCOA_LIBRARY})
  else()
    if(LIBXKBCOMMON_VERSION AND ${LIBXKBCOMMON_VERSION} VERSION_GREATER_EQUAL "1.10")
      target_compile_definitions(platform PRIVATE HAVE_XKB_KEYMAP_MOD_GET_MASK=1)
    endif()
    if(LIBEI_FOUND)
      target_compile_definitions(platform PUBLIC WINAPI_LIBEI)
    endif()
    if(LIBPORTAL_FOUND)
      target_compile_definitions(platform PUBLIC WINAPI_LIBPORTAL)
    endif()
    if(BUILD_ADB_CLIENT_SUPPORT)
      target_compile_definitions(platform PUBLIC WINAPI_ADB=1)
      find_package(Java REQUIRED COMPONENTS Development)
      find_program(D8_EXECUTABLE d8 REQUIRED)
      set(ADB_BRIDGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/adb-bridge")
      set(ADB_BRIDGE_DEX "${ADB_BRIDGE_DIR}/classes.dex")
      set(ADB_BRIDGE_JAR "${ADB_BRIDGE_DIR}/input-bridge.jar")
      add_custom_command(
        OUTPUT "${ADB_BRIDGE_DEX}"
        BYPRODUCTS "${ADB_BRIDGE_JAR}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADB_BRIDGE_DIR}/classes"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ADB_BRIDGE_DIR}/classes"
        COMMAND ${Java_JAVAC_EXECUTABLE} --release 8
          -d "${ADB_BRIDGE_DIR}/classes"
          "${CMAKE_SOURCE_DIR}/src/android/org/deskflow/android/InputBridge.java"
        COMMAND ${Java_JAR_EXECUTABLE} --create --file "${ADB_BRIDGE_JAR}"
          -C "${ADB_BRIDGE_DIR}/classes" .
        COMMAND ${D8_EXECUTABLE} --min-api 26 --output "${ADB_BRIDGE_DIR}"
          "${ADB_BRIDGE_JAR}"
        DEPENDS "${CMAKE_SOURCE_DIR}/src/android/org/deskflow/android/InputBridge.java"
        VERBATIM
      )
      add_custom_target(adb-input-bridge ALL DEPENDS "${ADB_BRIDGE_DEX}")
      add_dependencies(platform adb-input-bridge)
      target_compile_definitions(platform PRIVATE
        DESKFLOW_ADB_BRIDGE_PATH="${ADB_BRIDGE_DEX}")
      install(FILES "${ADB_BRIDGE_DEX}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/android"
        RENAME deskflow-input-bridge.dex)
    endif()
    if (HAVE_IC_RESTORE)
      target_compile_definitions(platform PUBLIC HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE)
    endif()
    if (HAVE_LP_CLIPBOARD)
      target_compile_definitions(platform PUBLIC HAVE_LIBPORTAL_CLIPBOARD)
    endif()
    target_include_directories(platform PUBLIC ${LIBEI_INCLUDE_DIRS} ${LIBPORTAL_INCLUDE_DIRS})
    target_link_libraries(platform PRIVATE Qt6::Gui ${LIBXKBCOMMON_LINK_LIBRARIES})
    if(NOT ANDROID)
      target_link_libraries(platform PUBLIC Qt6::DBus)
    endif()
    target_link_libraries(platform PRIVATE ${LIBEI_LINK_LIBRARIES} ${LIBPORTAL_LINK_LIBRARIES})
  endif()
endif()
