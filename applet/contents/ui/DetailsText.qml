/*
    SPDX-FileCopyrightText: 2013-2017 Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

Item {
    height: detailsGrid.implicitHeight + copyAllLabel.height

    property var details: []

    KQuickControlsAddons.Clipboard {
        id: clipboard
    }

    Timer {
        id: copiedTimer
        interval: 1000
        onTriggered: {
            copiedTarget.text = copiedTargetBefore;
            copiedTarget = null;
        }
    }

    property var copiedTarget: null
    property var copiedTargetBefore: null
    MouseArea {
        anchors.fill: parent
        onPressed: {
            var content = details.map(function(detail, i) { return (i % 2 === 0) ? detail + ":" : detail }).join("\n");
            if (copiedTarget !== null) {
                copiedTimer.stop();
                copiedTarget.text = copiedTargetBefore;
            }
            if (mouseY >= copyAllLabel.y && mouseY < copyAllLabel.y + copyAllLabel.height) {
                copiedTarget = copyAllLabel;
                copiedTargetBefore = copyAllLabel.text;
                clipboard.content = content;
            } else {
                for (var i = 0; i < details.length; i += 2) {
                    if (mouseX >= detailsGrid.children[i].x && mouseX < detailsGrid.children[i + 1].x + detailsGrid.children[i + 1].width &&
                            mouseY >= detailsGrid.children[i].y && mouseY < detailsGrid.children[i].y + detailsGrid.children[i].height) {
                        copiedTarget = detailsGrid.children[i + 1];
                        copiedTargetBefore = detailsGrid.children[i + 1].text;
                        clipboard.content = details[i + 1];
                        break;
                    }
                }
            }
            copiedTarget.text = "Copied!";
            copiedTimer.start();
        }
    }

    GridLayout {
        id: detailsGrid
        width: parent.width
        columns: 2
        rowSpacing: PlasmaCore.Units.smallSpacing / 4
        Repeater {
            model: details.length
            delegate: PlasmaComponents3.Label {
                readonly property bool isContent: index % 2
                anchors.left: isContent ? detailsGrid.horizontalCenter : undefined
                anchors.right: isContent ? undefined : detailsGrid.horizontalCenter
                elide: isContent ? Text.ElideRight : Text.ElideNone
                font: PlasmaCore.Theme.smallestFont
                horizontalAlignment: isContent ? Text.AlignLeft : Text.AlignRight
                text: isContent ? details[index] : `${details[index]}: `
                textFormat: Text.PlainText
                opacity: isContent ? 0.7 : 1.0
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                    onHoveredChanged: {
                        if (isContent) {
                            opacity = hovered ? 0.5 : 0.7
                        }
                    }
                }
            }
        }
    }
    PlasmaComponents3.Label {
        id: copyAllLabel
        text: i18n("Copy All")
        font: PlasmaCore.Theme.smallestFont
        opacity: 0.6
        topPadding: 5
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        HoverHandler {
            cursorShape: Qt.PointingHandCursor
            target: copyAllLabel
            onHoveredChanged: {
                copyAllLabel.opacity = hovered ? 0.9 : 0.6
            }
        }
    }
}