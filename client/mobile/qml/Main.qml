// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
  id: root
  width: 414  // iPhone 14 Pro Max Breite
  height: 896  // iPhone 14 Pro Max Höhe
  minimumWidth: 320
  minimumHeight: 480
  title: qsTr("HasiHasi")
  visible: true
  color: "#1a1a2e"  // Dunkler Hintergrund für Dark Mode
  
  // Responsive Design
  property real scale: Math.min(width / 414, height / 896)
  property bool isLargeScreen: width > 600
  
  // Hauptansicht
  StackView {
    id: stackView
    anchors.fill: parent
    initialItem: ServerListPage {
      id: serverListPage
      onServerSelected: {
        stackView.push(ConnectionPage {
          serverName: selectedServerName
          serverUrl: selectedServerUrl
        })
      }
    }
  }
  
  // Globale Stile
  styleData: {
    // Adaptive Schriftgrößen basierend auf Skalierung
    const baseFontSize = 14 * root.scale
    const largeFontSize = 24 * root.scale
    const smallFontSize = 12 * root.scale
    
    return {
      "fontSize": baseFontSize,
      "largeFontSize": largeFontSize,
      "smallFontSize": smallFontSize,
      "buttonHeight": 48 * root.scale,
      "itemHeight": 56 * root.scale,
      "padding": 16 * root.scale
    }
  }
}
