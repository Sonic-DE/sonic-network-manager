/*
 *   Copyright 2021 Devin Lin <espidev@gmail.com>
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
    id: modemPage
    title: i18n("Modem") + " " + modem.uni
    
    property Modem modem
    
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        
        Controls.Button {
            icon.name: "globe"
            text: "Access point names"
            onClicked: {
                kcm.push("ProfileList.qml", {"modem": kcm.modem});
            }
        }
        
        Controls.Button {
            icon: "network-modem"
            text: modem.isEnabled ? i18n("Disable modem") : i18n("Enable modem")
            onClicked: modem.setEnabled(!modem.isEnabled)
        }
        
        Controls.Button {
            icon: "system-reboot"
            text: i18n("Force Restart")
            onClicked: modem.reset()
        }
        
        Kirigami.FormLayout {

            ColumnLayout {
                Kirigami.FormData.label: i18n("Access Technologies:")
                Repeater {
                    model: modem.accessTechnologies
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Device:")
                text: modem.device
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Device ID:")
                text: modem.deviceIdentifier
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("Drivers:")
                Repeater {
                    model: modem.drivers
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Equipment ID:")
                text: modem.equipmentIdentifier
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Enabled:")
                text: modem.isEnabled
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Manufacturer:")
                text: modem.manufacturer
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Maximum Active Bearers:")
                text: modem.maxActiveBearers
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Maximum Total Bearers:")
                text: modem.maxBearers
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Model:")
                text: modem.model
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("Owned Numbers:")
                Repeater {
                    model: modem.ownNumbers
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Plugin:")
                text: modem.plugin
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("Ports:")
                Repeater {
                    model: modem.ports
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Power State:")
                text: modem.powerState
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Primary Port:")
                text: modem.primaryPort
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Revision:")
                text: modem.revision
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Signal Quality:")
                text: modem.signalQuality
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("SIM Path:")
                text: modem.simPath
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("State:")
                text: modem.state
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("Failure Reason:")
                text: modem.stateFailedReason
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("Supported Capabilities:")
                Repeater {
                    model: modem.supportedCapabilities
                    Controls.Label {
                        text: modelData
                    }
                }
            }
        }
    }
}

