// SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
  id: root
  
  title: qsTr("Verbindung")
  
  // Server-Informationen
  property string serverName: ""
  property string serverUrl: ""
  
  // Verbindungsstatus
  property bool isConnected: false
  property bool isConnecting: false
  
  // Kopfzeile
  header: ToolBar {
    Material.background: "#16213e"
    
    RowLayout {
      anchors.fill: parent
      anchors.margins: root.styleData.padding
      
      Text {
        text: qsTr("Verbindung")
        font.pixelSize: root.styleData.largeFontSize
        font.bold: true
        color: "#e94560"
      }
      
      Item { Layout.fillWidth: true }
      
      Button {
        text: qsTr("Abbrechen")
        onClicked: stackView.pop()
        visible: !root.isConnected
      }
    }
  }
  
  // Hauptinhalt
  ColumnLayout {
    anchors.fill: parent
    anchors.margins: root.styleData.padding
    spacing: root.styleData.padding
    
    // Server-Info
    Rectangle {
      Layout.fillWidth: true
      height: 120 * root.scale
      color: "#1a1a2e"
      radius: 8
      
      Column {
        anchors.centerIn: parent
        spacing: 8
        
        Text {
          text: root.serverName
          font.pixelSize: root.styleData.largeFontSize
          font.bold: true
          color: "#ffffff"
          anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Text {
          text: root.serverUrl
          font.pixelSize: root.styleData.fontSize
          color: "#a0a0a0"
          anchors.horizontalCenter: parent.horizontalCenter
        }
      }
    }
    
    // Benutzername-Eingabe
    TextField {
      id: usernameField
      Layout.fillWidth: true
      placeholderText: qsTr("Benutzername")
      font.pixelSize: root.styleData.fontSize
      background: Rectangle {
        color: "#1a1a2e"
        radius: 8
        border.color: "#e94560"
      }
    }
    
    // Passwort-Eingabe
    TextField {
      id: passwordField
      Layout.fillWidth: true
      placeholderText: qsTr("Passwort")
      echoMode: TextInput.Password
      font.pixelSize: root.styleData.fontSize
      background: Rectangle {
        color: "#1a1a2e"
        radius: 8
        border.color: "#e94560"
      }
    }
    
    // Verbinden-Button
    Button {
      id: connectButton
      Layout.fillWidth: true
      height: root.styleData.buttonHeight
      text: root.isConnecting ? qsTr("Verbinde...") : qsTr("Verbinden")
      enabled: !root.isConnecting && usernameField.text.length > 0
      font.pixelSize: root.styleData.largeFontSize
      font.bold: true
      
      background: Rectangle {
        color: parent.enabled ? "#e94560" : "#666666"
        radius: 8
      }
      
      onClicked: {
        root.isConnecting = true
        // Verbindung wird hergestellt
        setTimeout(function() {
          root.isConnecting = false
          root.isConnected = true
          stackView.push(ChannelListPage {
            username: usernameField.text
          })
        }, 2000)
      }
    }
    
    // Ladeanzeige
    BusyIndicator {
      Layout.alignment: Qt.AlignHCenter
      running: root.isConnecting
      visible: root.isConnecting
    }
  }
}
