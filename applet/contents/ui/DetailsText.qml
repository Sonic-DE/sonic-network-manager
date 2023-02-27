import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

Item {
    height: detailsGrid.implicitHeight + copyAllButton.height

    property var details: []

    KQuickControlsAddons.Clipboard {
        id: clipboard
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onPressed: {
            for (var i = 0; i < details.length; i += 2) {
                if (mouseX >= detailsGrid.children[i].x && mouseX < detailsGrid.children[i + 1].x + detailsGrid.children[i + 1].width &&
                    mouseY >= detailsGrid.children[i].y && mouseY < detailsGrid.children[i].y + detailsGrid.children[i].height) {
                    clipboard.content = details[i + 1];
                    break;
                }
            }
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
                Layout.fillWidth: true
                readonly property bool isContent: index % 2

                elide: isContent ? Text.ElideRight : Text.ElideNone
                font: PlasmaCore.Theme.smallestFont
                horizontalAlignment: isContent ? Text.AlignLeft : Text.AlignRight
                text: isContent ? details[index] : `${details[index]}:`
                textFormat: Text.PlainText
                opacity: isContent ? 0.8 : 1.0

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                    onHoveredChanged: {
                        if (isContent) {
                            opacity = hovered ? 0.5 : 0.8
                        }
                    }
                }
            }
        }
    }
    PlasmaComponents3.Button {
        id: copyAllButton
        text: i18n("Copy All")
        icon.name: "edit-copy"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        visible: details.length > 2
        Timer {
            id: copyAllTimer
            interval: 1000
            onTriggered: {
                copyAllButton.text = i18n("Copy All")
                copyAllButton.opacity = 1
                }
            }
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
        text = i18n("Copied!")
        opacity = 0.8
        copyAllTimer.start()
        }
    }
}