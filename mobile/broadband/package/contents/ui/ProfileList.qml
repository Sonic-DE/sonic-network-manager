/*
 *   Copyright 2020 Devin Lin <espidev@gmail.com>
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
import mobilebroadbandkcm 1.0

Kirigami.ScrollablePage {
    id: apnlist
    title: i18n("APNs")
    
    ListView {
        id: profileListView
        model: ProfileModel
     
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            visible: ProfileModel.count() === 0
            text: i18n("No APNs configured")
            icon.name: "globe"
            
            helpfulAction: Kirigami.Action {
                iconName: "list-add"
                text: i18n("Add APN")
                onTriggered: kcm.push("EditProfile.qml", {"profile": null})
            }
        }
        
        delegate: Kirigami.SwipeListItem {
            property ProfileSettings profile: ProfileModel.get(index)
            
            onClicked: kcm.activateProfile(profile.connectionUni)
            
            actions: [
                Kirigami.Action {
                    iconName: "entry-edit"
                    text: i18n("Edit")
                    onTriggered: kcm.push("EditProfile.qml", {"profile": profile})
                },
                Kirigami.Action {
                    iconName: "delete"
                    text: i18n("Delete")
                    onTriggered: kcm.removeProfile(profile.connectionUni)
                }
            ]
            
            contentItem: RowLayout {
                Layout.fillWidth: true
                
                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Heading {
                        level: 3
                        text: profile.name
                    }
                    Controls.Label {
                        text: profile.apn
                    }
                }
                Controls.RadioButton {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    checked: kcm.activeConnectionUni == profile.connectionUni
                    onClicked: kcm.activateProfile(profile.connectionUni)
                }
            }
        }
        
        header: Kirigami.SwipeListItem {
            visible: ProfileModel.count() !== 0
            onClicked: kcm.push("EditProfile.qml", {"profile": null})
            
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
