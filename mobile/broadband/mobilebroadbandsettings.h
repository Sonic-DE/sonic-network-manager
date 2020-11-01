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

#ifndef MOBILEBROADBANDSETTINGS_H
#define MOBILEBROADBANDSETTINGS_H

#include <KQuickAddons/ConfigModule>

#include "mobileproviders.h"
#include "apnprofilemodel.h"

#include <ModemManagerQt/Manager>
#include <ModemManagerQt/GenericTypes>
#include <ModemManagerQt/ModemDevice>

class MobileBroadbandSettings : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(bool mobileDataActive READ mobileDataActive WRITE setMobileDataActive NOTIFY mobileDataActiveChanged)
    Q_PROPERTY(bool allowRoaming READ allowRoaming WRITE setAllowRoaming NOTIFY allowRoamingChanged)
    Q_PROPERTY(bool activeAPNProfileUni READ activeAPNProfileUni WRITE setActiveAPNProfileUni NOTIFY activeBearerChanged)
    
public:
    MobileBroadbandSettings(QObject *parent, const QVariantList &args);
    virtual ~MobileBroadbandSettings();

    bool mobileDataActive();
    void setMobileDataActive(bool active);
    bool allowRoaming();
    void setAllowRoaming(bool allowRoaming);
    QString activeAPNProfileUni();
    void setActiveAPNProfileUni(QString uni);

    Q_INVOKABLE QString getModemDevice(); // get modem identifier
    
    void updateActiveBearer();
    void updateBearerProfileModel();
    
Q_SIGNALS:
    void mobileDataActiveChanged();
    void allowRoamingChanged();
    void activeBearerChanged();
    void bearersChanged();

private:
    bool m_mobileDataActive;
    
    ModemManager::Bearer::Ptr m_bearer = nullptr;
    ModemManager::ModemDevice::Ptr m_modemDevice = nullptr;
    MobileProviders* m_providers;
};

#endif // MOBILEBROADBANDSETTINGS_H
