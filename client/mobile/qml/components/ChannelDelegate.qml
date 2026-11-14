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
  property int id: 0
  property string name: ""
  property int userCount: 0
  property int maxUsers: 0
  
  // Klick-Fläche
  MouseArea {
    anchors.fill: parent
    onClicked: root.clicked()
  }
  
  // Inhalt
  RowLayout {
    anchors.fill: parent
    anchors.margins: root.styleData.padding
    
    // Kanal-Icon
    Text {
      text: qsTr("🔊")
      font.pixelSize: root.styleData.largeFontSize
      Layout.alignment: Qt.AlignVCenter
    }
    
    // Kanal-Name
    Text {
      text: root.name
      font.pixelSize: root.styleData.fontSize
      font.bold: true
      color: "#ffffff"
      Layout.fillWidth: true
      elide: Text.ElideMiddle
    }
    
    // Benutzer-Anzahl
    Text {
      text: root.userCount + "/" + root.maxUsers
      font.pixelSize: root.styleData.smallFontSize
      color: "#a0a0a0"
      Layout.alignment: Qt.AlignVCenter
    }
  }
}
