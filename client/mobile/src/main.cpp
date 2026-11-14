// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QIcon>

#include "hh/client/push_notification_manager.hpp"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("HasiHasi");
  app.setApplicationVersion("0.1.0");
  app.setOrganizationName("HasiHasi");
  app.setOrganizationDomain("hasihasi.local");
  
  // Plattform-spezifische Konfiguration
  QOperatingSystemVersion os = QOperatingSystemVersion::current();
  
  if (os.type() == QOperatingSystemVersion::OS_Android) {
    // Android-spezifische Einstellungen
    app.setApplicationName("HasiHasi Mobile");
  } else if (os.type() == QOperatingSystemVersion::OS_IOS) {
    // iOS-spezifische Einstellungen
    app.setApplicationName("HasiHasi Mobile");
  }
  
  // Push-Benachrichtigungen initialisieren
  hh::client::PushNotificationManager pushManager;
  
  // QML-Engine erstellen
  QQmlApplicationEngine engine;
  
  // QML-Datei laden
  const QUrl url(u"qrc:/qml/Main.qml"_qs);
  QObject::connect(
    &engine,
    &QQmlApplicationEngine::objectCreated,
    &app,
    [url](QObject *obj, const QUrl &objUrl) {
      if (!obj && url == objUrl)
        QCoreApplication::exit(-1);
    },
    Qt::QueuedConnection
  );
  engine.load(url);
  
  return app.exec();
}
