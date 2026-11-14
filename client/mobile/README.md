# HasiHasi Mobile-Client

## Übersicht

Der HasiHasi Mobile-Client ist eine Qt Quick-basierte Mobile-Anwendung für HasiHasi, den Low-Latency Linux Voice Chat. Er unterstützt Android und iOS und bietet eine mobile-optimierte Benutzeroberfläche mit QML.

## Features

- **Mobile-optimierte UI**: Große Buttons, Gesten, responsive Design
- **Android-Support**: Build für Android (API 29+)
- **iOS-Support**: Build für iOS (optional)
- **Push-Benachrichtigungen**: FCM (Android), APNs (iOS)
- **Mobile-Audio**: Headset-Management, Background-Mode
- **Responsive Design**: Automatische Anpassung an verschiedene Bildschirmgrößen
- **Offline-Mode**: Lokaler Cache
- **Batterie-Optimierung**: Adaptive Frame-Rate

## Verzeichnisstruktur

```
client/mobile/
├── CMakeLists.txt              # CMake-Konfiguration
├── android/                    # Android-spezifische Dateien
│   ├── AndroidManifest.xml.in  # Android-Manifest-Vorlage
│   └── libs/                   # Native Libraries
├── ios/                        # iOS-spezifische Dateien
│   └── Info.plist.in           # iOS-Info.plist-Vorlage
├── include/hh/client/          # C++-Header
│   └── push_notification_manager.hpp
├── qml/                        # QML-Dateien
│   ├── Main.qml                # Hauptdatei
│   ├── views/                  # Seiten
│   │   ├── ServerListPage.qml  # Server-Auswahl
│   │   ├── ConnectionPage.qml  # Verbindungsaufbau
│   │   └── ChannelListPage.qml # Kanalliste
│   └── components/             # UI-Komponenten
│       ├── ServerDelegate.qml  # Server-Listenelement
│       └── ChannelDelegate.qml # Kanal-Listenelement
├── resources/                  # Ressourcen
└── src/                        # C++-Quellen
    ├── main.cpp                # Hauptdatei
    └── push_notification_manager.cpp
```

## Build-Anleitung

### Android

1. **Android SDK/NDK installieren**:
   ```bash
   sdkmanager "platforms;android-33" "ndk;25.2.9519653"
   ```

2. **CMake konfigurieren**:
   ```bash
   cmake -B build-android \
     -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/cmake/android-legacy/cmake/AndroidNdkModule.cmake \
     -DANDROID_ABI=arm64-v8a \
     -DANDROID_PLATFORM=android-29 \
     -DHASIHASI_BUILD_MOBILE=ON \
     -DCMAKE_BUILD_TYPE=Release
   ```

3. **Bauen**:
   ```bash
   cmake --build build-android --config Release
   ```

4. **APK installieren**:
   ```bash
   adb install build-android/hasihasi-mobile.apk
   ```

### iOS

1. **Xcode installieren**:
   - App Store herunterladen

2. **CMake konfigurieren**:
   ```bash
   cmake -B build-ios \
     -DCMAKE_TOOLCHAIN_FILE=$HOME/Qt/6.5.0-ios/cmake/Qt6ios.cmake \
     -DHASIHASI_BUILD_MOBILE=ON \
     -DCMAKE_BUILD_TYPE=Release
   ```

3. **Bauen**:
   ```bash
   cmake --build build-ios --config Release
   ```

4. **Xcode-Projekt öffnen**:
   ```bash
   open build-ios/hasihasi-mobile.xcodeproj
   ```

## Push-Benachrichtigungen

### Android (FCM)

1. **Firebase-Projekt erstellen**:
   - [Firebase Console](https://console.firebase.google.com/)
   - Neue App erstellen (Package Name: `local.hasihasi.mobile`)

2. **Server Key konfigurieren**:
   ```cpp
   pushManager.set_fcm_server_key("your-fcm-server-key");
   ```

3. **Token registrieren**:
   ```cpp
   pushManager.register_for_push(device_token);
   ```

### iOS (APNs)

1. **Apple Developer Account**:
   - App ID erstellen
   - Push-Benachrichtigungen aktivieren

2. **Zertifikat erstellen**:
   - [Apple Developer Portal](https://developer.apple.com/)
   - APNs Certificate erstellen (.p12)

3. **Zertifikat konfigurieren**:
   ```cpp
   pushManager.set_apns_certificate("/path/to/cert.pem");
   ```

## Testing

```bash
ctest --test-dir build-android -R test_push_notification
```

## Lizenz

GPL-3.0-or-later

## Autor

Nils Pankoke (Tremorian)
