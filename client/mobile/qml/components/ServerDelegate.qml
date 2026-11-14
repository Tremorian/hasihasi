// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: root
  width: parent ? parent.width : 400
  height: root.styleData.itemHeight
  
  color: "#16213e"
  radius: 8
  
  // Signale
  signal clicked()
  
  // Daten
  property string name: ""
  property string url: ""
  property string status: "offline"
  
  // Status-Farbe
  property color statusColor: {
    if (status === "online") return "#00ff88"
    if (status === "offline") return "#ff4444"
    return "#ffaa00"
  }
  
  // Klick-Fläche
  MouseArea {
    anchors.fill: parent
    onClicked: root.clicked()
  }
  
  // Inhalt
  RowLayout {
    anchors.fill: parent
    anchors.margins: root.styleData.padding
    
    // Status-Indikator
    Rectangle {
      width: 16
      height: 16
      radius: 8
      color: root.statusColor
      Layout.alignment: Qt.AlignVCenter
    }
    
    // Server-Name
    Text {
      text: root.name
      font.pixelSize: root.styleData.fontSize
      font.bold: true
      color: "#ffffff"
      Layout.fillWidth: true
      elide: Text.ElideMiddle
    }
    
    // Server-URL
    Text {
      text: root.url
      font.pixelSize: root.styleData.smallFontSize
      color: "#a0a0a0"
      elide: Text.ElideRight
    }
  }
}
