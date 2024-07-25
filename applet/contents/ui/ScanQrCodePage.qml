/*
    SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtMultimedia as QtMultimedia

import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

import org.kde.plasma.networkmanagement as PlasmaNM

import org.kde.prison.scanner as PrisonScanner

ColumnLayout {
    id: page

    spacing: Kirigami.Units.smallSpacing

    // TODO camera selection.
    // property Component headerItems: Component {
    //     PlasmaComponents.ToolButton {
    //         icon.name: "camera-web"
    //     }
    // }

    PrisonScanner.VideoScanner {
        id: scanner
        formats: PrisonScanner.Format.QRCode
        videoSink: viewFinder.videoSink
        onResultChanged: (result) => {
            if (result.text) {
                meCardHandler.text = result.text;
            }
        }
    }

    QtMultimedia.CaptureSession {
        camera: QtMultimedia.Camera {
            id: camera
            active: page.QQC.StackView.status === QQC.StackView.Active
        }
        videoOutput: viewFinder
    }

    PlasmaNM.MeCardHandler {
        id: meCardHandler
        onSsidChanged: {
            if (ssid) {
                page.QQC.StackView.view.push("ConnectToQrCodePage.qml", {
                    meCard: meCardHandler.meCard
                });

                reset();
            }
        }
    }

    PlasmaComponents.Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        textFormat: Text.PlainText
        text: i18n("Connect to a WiFi network by scanning its QR code.")
    }

    QtMultimedia.VideoOutput {
        id: viewFinder
        Layout.fillWidth: true
        Layout.fillHeight: true

        PlasmaComponents.Label {
            anchors.fill: parent
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            textFormat: Text.PlainText
            text: camera.errorString
            visible: camera.error !== QtMultimedia.Camera.NoError
        }
    }
}
