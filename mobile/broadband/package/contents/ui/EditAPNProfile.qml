/*
 *   Copyright 2020 Devin Lin <espidev@gmail.com>
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
import mobilebroadbandkcm 1.0

Kirigami.ScrollablePage {
    id: editAPNPage
    title: i18n("Edit APN")
    
    property APNProfile apnProfile
    
    ColumnLayout {
        Kirigami.FormLayout {
            Controls.TextField {
                Kirigami.FormData.label: i18n("APN:")
                text: apnProfile.apn
            }
            Controls.TextField {
                
            }
        }
    }
}
