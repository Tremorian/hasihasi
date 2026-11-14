# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

# Mobile-Client Bibliothek (für Tests)
add_library(hh_mobile_lib
  src/push_notification_manager.cpp
)

target_include_directories(hh_mobile_lib
  PUBLIC
    include
)

target_link_libraries(hh_mobile_lib
  PUBLIC
    Qt6::Quick
    Qt6::Network
    hh_common::hh_common
)

target_compile_features(hh_mobile_lib PRIVATE cxx_std_20)

# Mobile-Client executable
add_executable(hasihasi-mobile
  src/main.cpp
)

target_sources(hasihasi-mobile
  PRIVATE
    qml/Main.qml
    qml/views/ServerListPage.qml
    qml/views/ConnectionPage.qml
    qml/views/ChannelListPage.qml
    qml/components/ServerDelegate.qml
    qml/components/ChannelDelegate.qml
)

target_link_libraries(hasihasi-mobile
  PRIVATE
    hh_mobile_lib
    Qt6::Quick
    Qt6::Network
)

target_compile_definitions(hasihasi-mobile
  PRIVATE
    HH_VERSION="${HH_VERSION}"
    HH_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    HH_BUILD_DATE="${HH_BUILD_DATE}"
)

# QML-Dateien als Ressourcen einbinden
qt6_add_resources(hasihasi-mobile resources/qml.qrc
  PREFIX
    "/qml"
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/ServerListPage.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/ConnectionPage.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/views/ChannelListPage.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/ServerDelegate.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/components/ChannelDelegate.qml
)

# Android-spezifische Konfiguration
if(ANDROID)
  set_target_properties(hasihasi-mobile PROPERTIES
    ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    ANDROID_EXTRA_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/android/libs/*.so
  )
  
  # Android-Manifest
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/android/AndroidManifest.xml
  )
endif()

# iOS-spezifische Konfiguration
if(IOS)
  set_target_properties(hasihasi-mobile PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in
  )
endif()

install(TARGETS hasihasi-mobile
  RUNTIME DESTINATION bin
)
