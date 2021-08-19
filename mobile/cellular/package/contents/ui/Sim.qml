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
    id: simPage
    title: i18n("SIM") + " " + sim.uni
    
    property Sim sim
    
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        
        Kirigami.FormLayout {
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Uni:</b>")
                text: sim.uni
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Is Locked:</b>")
                text: sim.locked ? i18n("Yes") : i18n("No")
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Locked Reason:</b>")
                text: sim.lockedReason
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>IMSI:</b>")
                text: sim.imsi
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>EID:</b>")
                text: sim.eid
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Operator ID:</b>")
                text: sim.operatorIdentifier
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>Operator Name:</b>")
                text: sim.operatorName
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("<b>SIM ID:</b>")
                text: sim.simIdentifier
            }
            ColumnLayout {
                Kirigami.FormData.label: i18n("<b>Emergency Numbers:</b>")
                Repeater {
                    model: sim.emergencyNumbers
                    Controls.Label {
                        text: modelData
                    }
                }
            }
        }
    }
}
