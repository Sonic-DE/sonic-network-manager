/*
 *   Copyright 2017 Martin Kacej <m.kacej@atlas.sk>
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

import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls
import org.kde.plasma.networkmanagement 0.2 as PlasmaNM
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kcm 1.2

ScrollViewKCM {
    id: main

    PlasmaNM.Handler {
        id: handler
    }

    PlasmaNM.EnabledConnections {
        id: enabledConnections
    }

    PlasmaNM.NetworkModel {
        id: connectionModel
    }

    PlasmaNM.MobileProxyModel {
        id: mobileProxyModel
        sourceModel: connectionModel
        showSavedMode: false
    }

    Component.onCompleted: handler.requestScan()

    Timer {
        id: scanTimer
        interval: 10200
        repeat: true
        running: parent.visible

        onTriggered: handler.requestScan()
    }

    header: Kirigami.InlineMessage {
        id: inlineError
        showCloseButton: true

        type: Kirigami.MessageType.Warning
        Connections {
            target: handler
            onConnectionActivationFailed: {
                inlineError.text = message;
                inlineError.visible = true;
            }
        }
    }

    view: ListView {
        id: view

        clip: true
        currentIndex: -1

        section.property: "Section"
        section.delegate: Kirigami.ListSectionHeader {
            text: section
        }

        model: mobileProxyModel
        delegate: ConnectionItemDelegate {}

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: !enabledConnections.wirelessEnabled
            text: i18n("Wi-Fi is disabled")
            helpfulAction: Kirigami.Action {
                iconName: "network-wireless-connected"
                text: i18n("Enable")
                onTriggered: handler.enableWireless(true)
            }
        }
    }

    footer: Row {
        Controls.Button {
            text: i18n("Disable Wi-Fi")
            icon.name: "network-disconnect"
            visible: enabledConnections.wirelessEnabled
            onClicked: handler.enableWireless(false)
        }
        Controls.Button {
            icon.name: "list-add"
            text: i18n("Add custom connection")
            visible: enabledConnections.wirelessEnabled
            onClicked: kcm.push("NetworkSettings.qml")
        }

        Controls.Button {
            icon.name: "document-save"
            text: i18n("Saved Connections")
            checkable: true
            checked: false
            visible: enabledConnections.wirelessEnabled
            onClicked: mobileProxyModel.showSavedMode = !mobileProxyModel.showSavedMode
        }
    }
}
