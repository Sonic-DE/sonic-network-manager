/*
    SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Controls
import QtWebEngine

import org.kde.kirigami as Kirigami
import org.kde.plasma.networkmanagement

Kirigami.ApplicationWindow {
    property bool loggedIn: !connectionIconProvider.needsPortal

    title: i18n("Log into Network")

    header: TextField {
        visible: !loggedIn
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        color: Kirigami.Theme.disabledTextColor
        horizontalAlignment: Text.AlignHCenter
        readOnly: true
        text: webView.url
        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
        }
        Kirigami.Separator {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }

    WebEngineView {
        id: webView
        anchors.fill: parent
        url: "https://networkcheck.kde.org"
        visible: !loggedIn
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        visible: loggedIn
        text: i18n("Logged in successfully")
        icon.name: "online"
    }

    ConnectionIcon {
        id: connectionIconProvider
    }

    footer: Button {
        text: "Log in!"
        onClicked: loggedIn = true
    }
}
