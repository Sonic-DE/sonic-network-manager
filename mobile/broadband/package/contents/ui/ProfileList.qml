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
        model: ListModel {
            ListElement {
                name: "Telus SP"
                apn: "sp.telus.com"
            }
            ListElement {
                name: "Public Mobile"
                apn: "sb.mb.com"
            }
        }
        //model: APNProfileModel
     
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            visible: parent.model.count === 0
            text: i18n("No APNs configured")
            icon.name: "globe"
            
            helpfulAction: Kirigami.Action {
                iconName: "list-add"
                text: i18n("Add APN")
                onTriggered: {
                    kcm.push("EditAPN.qml");
                }
            }
        }
        
        delegate: Kirigami.SwipeListItem {
            property ProfileSettings profile: ProfileModel.get(index)
            
            onClicked: {
                
            }
            
            actions: [
                Kirigami.Action {
                    iconName: "entry-edit"
                    text: i18n("Edit")
                    onTriggered: {
                        // TODO
                    }
                },
                Kirigami.Action {
                    iconName: "delete"
                    text: i18n("Delete")
                    onTriggered: {
                        // TODO
                    }
                }
            ]
            
            contentItem: RowLayout {
                Layout.fillWidth: true
                
                ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Heading {
                        level: 3
                        text: model.name
                    }
                    Controls.Label {
                        text: model.apn
                    }
                }
                Controls.RadioButton {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    checked: true // TODO
                    // TODO onClicked
                }
            }
        }
        
        header: Kirigami.SwipeListItem {
            onClicked: kcm.push("EditAPNProfile.qml", {"profile": ProfileModel.defaultProfile()})
            
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
