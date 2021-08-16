/*
 *   Copyright 2020-2021 Devin Lin <espidev@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.12
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.12 as Controls
import org.kde.kirigami 2.12 as Kirigami
import cellularnetworkkcm 1.0

Kirigami.ScrollablePage {
    id: apnlist
    title: i18n("APNs")
    
    property Modem modem
    
    ListView {
        id: profileListView
        model: modem.profiles
     
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            visible: profileListView.count === 0
            text: i18n("No APNs configured")
            icon.name: "globe"
            
            helpfulAction: Kirigami.Action {
                iconName: "list-add"
                text: i18n("Add APN")
                onTriggered: kcm.push("EditProfile.qml", {"modem": apnlist.modem, "profile": null})
            }
        }
        
        delegate: Kirigami.SwipeListItem {
            onClicked: modem.activateProfile(modelData.connectionUni)
            
            actions: [
                Kirigami.Action {
                    iconName: "entry-edit"
                    text: i18n("Edit")
                    onTriggered: kcm.push("EditProfile.qml", {"modem": apnlist.modem, "profile": modelData})
                },
                Kirigami.Action {
                    iconName: "delete"
                    text: i18n("Delete")
                    onTriggered: modem.removeProfile(modelData.connectionUni)
                }
            ]
            
            contentItem: RowLayout {
                Layout.fillWidth: true
                
                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Heading {
                        level: 3
                        text: modelData.name
                    }
                    Controls.Label {
                        text: modelData.apn
                    }
                }
                Controls.RadioButton {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    checked: kcm.activeConnectionUni == modelData.connectionUni
                    onClicked: modem.activateProfile(modelData.connectionUni)
                }
            }
        }
        
        header: Kirigami.SwipeListItem {
            visible: profileListView.count !== 0
            onClicked: kcm.push("EditProfile.qml", {"modem": apnlist.modem, "profile": null})
            
            contentItem: Row {
                anchors.fill: parent
                anchors.leftMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    source: "list-add"
                    height: Kirigami.Units.gridUnit * 1.5
                    width: height
                }
                Kirigami.Heading {
                    level: 3
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.alignment: Qt.AlignLeft
                    text: i18n("Add APN")
                }
            }
        }
    }
}
