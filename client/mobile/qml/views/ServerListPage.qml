// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
  id: root
  
  // Signale
  signal serverSelected(string serverName, string serverUrl)
  
  title: qsTr("Serverauswahl")
  
  // Server-Daten
  ListModel {
    id: serverModel
    ListElement {
      name: "Lokaler Server"
      url: "localhost:64738"
      status: "online"
    }
    ListElement {
      name: "Test Server"
      url: "test.hasihasi.local:64738"
      status: "online"
    }
  }
  
  // Kopfzeile
  header: ToolBar {
    Material.background: "#16213e"
    
    RowLayout {
      anchors.fill: parent
      anchors.margins: root.styleData.padding
      
      Text {
        text: qsTr("🐰 HasiHasi")
        font.pixelSize: root.styleData.largeFontSize
        font.bold: true
        color: "#e94560"
      }
      
      Item { Layout.fillWidth: true }
      
      Button {
        text: qsTr("➕")
        font.pixelSize: root.styleData.largeFontSize
        onClicked: stackView.push(ServerAddPage {})
      }
    }
  }
  
  // Server-Liste
  ListView {
    id: serverList
    anchors.fill: parent
    anchors.margins: root.styleData.padding
    model: serverModel
    delegate: ServerDelegate {
      onClicked: {
        root.serverSelected(name, url)
      }
    }
    spacing: root.styleData.padding
    
    ScrollBar.vertical: ScrollBar {
      active: true
    }
  }
  
  // Leerer Zustand
  Label {
    anchors.centerIn: parent
    text: qsTr("Keine Server gefunden")
    font.pixelSize: root.styleData.fontSize
    color: "#a0a0a0"
    visible: serverModel.count === 0
  }
}
