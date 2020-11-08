/*
 *   Copyright 2018 Martin Kacej <m.kacej@atlas.sk>
 *             2020 Devin Lin <espidev@gmail.com>
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
import org.kde.plasma.networkmanagement 0.2 as PlasmaNM
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kcm 1.2
import mobilebroadbandkcm 1.0

SimpleKCM {
    id: main
    objectName: "mobileDataMain"

    PlasmaNM.Handler {
        id: handler
    }

    PlasmaNM.AvailableDevices {
        id: availableDevices
    }

    PlasmaNM.EnabledConnections {
        id: enabledConnections

        onWwanEnabledChanged: {
            mobileDataCheckbox.checked = mobileDataCheckbox.enabled && enabled
        }

        onWwanHwEnabledChanged: {
            mobileDataCheckbox.enabled = enabled && availableDevices.modemDeviceAvailable
        }
    }
    
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.largeSpacing
        
        visible: !enabledConnections.wwanHwEnabled || !availableDevices.modemDeviceAvailable
        icon.name: "auth-sim-missing"
        text: i18n("Modem not available")
    }
    
    Flickable {
        anchors.left: parent.left
        anchors.right: parent.right
        
        Kirigami.FormLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Kirigami.Units.largeSpacing * 2
            anchors.rightMargin: Kirigami.Units.largeSpacing * 2
            wideMode: false
            visible: enabledConnections.wwanHwEnabled && availableDevices.modemDeviceAvailable
            Kirigami.Heading {
                level: 3
                Kirigami.FormData.isSection: true
                text: i18n("Mobile Data")
            }
            Controls.Switch {
                id: mobileDataCheckbox
                Kirigami.FormData.label: i18n("Mobile data")
                text: checked ? i18n("On") : i18n("Off")
                enabled: enabledConnections.wwanHwEnabled && availableDevices.modemDeviceAvailable
                checked: kcm.mobileDataActive
                onCheckedChanged: kcm.mobileDataActive = checked
            }
            
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("SIM")
                weight: 0
            }
            Controls.Switch {
                Kirigami.FormData.label: i18n("Data roaming")
                text: checked ? i18n("On") : i18n("Off")
                enabled: mobileDataCheckbox.checked
                checked: kcm.allowRoaming
                onCheckedChanged: kcm.allowRoaming = checked
            }
            
            Controls.Button {
                icon.name: "globe"
                text: "Access point names"
                onClicked: {
                    kcm.push("ProfileList.qml");
                }
            }
            
        }
    }
}
