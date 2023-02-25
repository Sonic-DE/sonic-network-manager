/*
    SPDX-FileCopyrightText: 2013-2017 Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // for ContextMenu+MenuItem
import org.kde.plasma.components 3.0 as PlasmaComponents3

MouseArea {
    height: detailsGrid.implicitHeight

    property var details: []

    KQuickControlsAddons.Clipboard {
        id: clipboard
    }

    GridLayout {
        id: detailsGrid
        width: parent.width
        columns: 2
        rowSpacing: PlasmaCore.Units.smallSpacing / 4
        Repeater {
            id: repeater
            model: details.length
                PlasmaComponents3.Label {
                Layout.fillWidth: true
                readonly property bool isContent: index % 2

                elide: isContent ? Text.ElideRight : Text.ElideNone
                font: PlasmaCore.Theme.smallestFont
                horizontalAlignment: isContent ? Text.AlignLeft : Text.AlignRight
                text: isContent ? details[index] : `${details[index]}:`
                textFormat: Text.PlainText
                opacity: isContent ? (pointer.hovered ? 0.6 : 0.8) : 1.0

                HoverHandler {
                    id: pointer
                    cursorShape: Qt.PointingHandCursor
                }
                PlasmaComponents3.Button {
                    text: i18n("Copied")
                    id: copyButton
                    width: parent.width
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: 0
                    Timer {
                        id: timer
                        interval: 1000
                        onTriggered: copyButton.opacity = 0
                    }
                    onPressed: {
                        clipboard.content = details[index];
                        copyButton.opacity = 0.8
                        timer.start();
                    }
                }
            }
        }
    }
    PlasmaComponents3.Button {
        text: i18n("Copy All")
        icon.name: "edit-copy"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        onPressed: {
        var content = ""
        for (var i = 0; i < details.length; i++) {
            content += details[i]
            if ((i+1) % 2 === 0 && i !== details.length - 1) {
                    content += "\n"
                } else {
                    content += ", "
                }
            }
        clipboard.content = content
        }
    }
}