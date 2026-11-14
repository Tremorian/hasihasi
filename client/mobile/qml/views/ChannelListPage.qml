// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
  id: root
  
  title: qsTr("Kanäle")
  
  // Benutzer-Information
  property string username: ""
  
  // Kanal-Daten
  ListModel {
    id: channelModel
    ListElement {
      name: "Lobby"
      id: 1
      userCount: 5
      maxUsers: 50
    }
    ListElement {
      name: "Meeting"
      id: 2
      userCount: 3
      maxUsers: 20
    }
    ListElement {
      name: "Musik"
      id: 3
      userCount: 0
      maxUsers: 30
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
      
      // Benutzername
      Text {
        text: root.username
        font.pixelSize: root.styleData.fontSize
        color: "#ffffff"
      }
      
      Button {
        text: qsTr("⚙️")
        font.pixelSize: root.styleData.largeFontSize
        onClicked: stackView.push(SettingsPage {})
      }
    }
  }
  
  // Kanal-Liste
  ListView {
    id: channelList
    anchors.fill: parent
    anchors.margins: root.styleData.padding
    model: channelModel
    delegate: ChannelDelegate {
      onClicked: {
        stackView.push(ChannelViewPage {
          channelId: id
          channelName: name
          username: root.username
        })
      }
    }
    spacing: root.styleData.padding
    
    ScrollBar.vertical: ScrollBar {
      active: true
    }
  }
  
  // Untermenü
  footer: ToolBar {
    Material.background: "#16213e"
    
    RowLayout {
      anchors.fill: parent
      
      Button {
        text: qsTr("🎤")
        Layout.fillWidth: true
        font.pixelSize: root.styleData.largeFontSize
        onClicked: /* Stumm umschalten */
      }
      
      Button {
        text: qsTr("💬")
        Layout.fillWidth: true
        font.pixelSize: root.styleData.largeFontSize
        onClicked: stackView.push(ChatPage {})
      }
      
      Button {
        text: qsTr("👥")
        Layout.fillWidth: true
        font.pixelSize: root.styleData.largeFontSize
        onClicked: stackView.push(UserListPage {})
      }
    }
  }
}
