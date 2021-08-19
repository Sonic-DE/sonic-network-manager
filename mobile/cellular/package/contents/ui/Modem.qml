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
            text: "Access Point Names"
            onClicked: {
                kcm.push("ProfileList.qml", {"modem": modem});
            }
        }
        
        Controls.Button {
            icon.name: "network-modem"
            text: modem.isEnabled ? i18n("Disable Modem") : i18n("Enable Modem")
            onClicked: modem.setEnabled(!modem.isEnabled)
        }
        
        Controls.Button {
            icon.name: "system-reboot"
            text: i18n("Force Modem Restart")
            onClicked: modem.reset()
        }
        
        Kirigami.FormLayout {
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Uni:</b>")
                text: modem.uni
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("<b>Access Technologies:</b>")
                Repeater {
                    model: modem.accessTechnologies
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Device:</b>")
                text: modem.device
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Device ID:</b>")
                text: modem.deviceIdentifier
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("<b>Drivers:</b>")
                Repeater {
                    model: modem.drivers
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Equipment ID:</b>")
                text: modem.equipmentIdentifier
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Enabled:</b>")
                text: modem.isEnabled
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Manufacturer:</b>")
                text: modem.manufacturer
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Maximum Active Bearers:</b>")
                text: modem.maxActiveBearers
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Maximum Total Bearers:</b>")
                text: modem.maxBearers
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Model:</b>")
                text: modem.model
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("<b>Owned Numbers:</b>")
                Repeater {
                    model: modem.ownNumbers
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Plugin:</b>")
                text: modem.plugin
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("<b>Ports:</b>")
                Repeater {
                    model: modem.ports
                    Controls.Label {
                        text: modelData
                    }
                }
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Power State:</b>")
                text: modem.powerState
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Primary Port:</b>")
                text: modem.primaryPort
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Revision:</b>")
                text: modem.revision
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Signal Quality:</b>")
                text: modem.signalQuality
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>SIM Path:</b>")
                text: modem.simPath
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>State:</b>")
                text: modem.state
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Failure Reason:</b>")
                text: modem.stateFailedReason
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("<b>Supported Capabilities:</b>")
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

