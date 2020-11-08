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
#include "profilemodel.h"

#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/GsmSetting>
#include <NetworkManagerQt/CdmaSetting>
#include <NetworkManagerQt/ModemDevice>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>

#include <ModemManagerQt/Manager>
#include <ModemManagerQt/GenericTypes>
#include <ModemManagerQt/ModemDevice>

class MobileBroadbandSettings : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(bool mobileDataActive READ mobileDataActive WRITE setMobileDataActive NOTIFY mobileDataActiveChanged)
    Q_PROPERTY(bool allowRoaming READ allowRoaming WRITE setAllowRoaming NOTIFY allowRoamingChanged)
    Q_PROPERTY(QString activeConnectionUni READ activeConnectionUni NOTIFY activeConnectionUniChanged)
    
public:
    MobileBroadbandSettings(QObject *parent, const QVariantList &args);
    virtual ~MobileBroadbandSettings();

    bool mobileDataActive();
    void setMobileDataActive(bool active);
    bool allowRoaming();
    void setAllowRoaming(bool allowRoaming);
    QString activeConnectionUni();
    
    void detectProfileSettings(); // detect modem connection settings (ex. apn) and add a new connection
    
    Q_INVOKABLE QString getModemDevice(); // get modem identifier
    Q_INVOKABLE void activateProfile(const QString &connectionUni);
    Q_INVOKABLE void addProfile(const QString &name, const QString &apn, const QString &username, const QString &password, const QString &networkType);
    Q_INVOKABLE void removeProfile(const QString &connectionUni);
    Q_INVOKABLE void updateProfile(const QString &uni, const QString &name, const QString &apn, const QString &username, const QString &password, const QString &networkType);
    
Q_SIGNALS:
    void mobileDataActiveChanged();
    void allowRoamingChanged();
    void activeConnectionUniChanged();

private:
    NetworkManager::ConnectionSettings::ConnectionType m_nmModemType;
    
    NetworkManager::ModemDevice::Ptr m_nmModem = nullptr;
    ModemManager::ModemDevice::Ptr m_modemDevice = nullptr;
    ModemManager::Modem::Ptr m_modemInterface = nullptr;
    MobileProviders* m_providers;
};

#endif // MOBILEBROADBANDSETTINGS_H
