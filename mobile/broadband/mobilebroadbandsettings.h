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

class NetworkType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)

public:
    NetworkType(QObject *parent = nullptr, QString name = "", ModemManager::Modem::Capabilities flag = 0)
        : m_name(name)
        , m_flag(flag)
    {
        setParent(parent);
    }
    
    QString name()
    {
        return m_name;
    }
    bool enabled()
    {
        return m_enabled;
    }
    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
        Q_EMIT enabledChanged();
    }
    ModemManager::Modem::Capabilities flag()
    {
        return m_flag;
    }
    
Q_SIGNALS:
    void nameChanged();
    void enabledChanged();
    
private:
    QString m_name;
    bool m_enabled;
    ModemManager::Modem::Capabilities m_flag;
};

class MobileBroadbandSettings : public KQuickAddons::ConfigModule
{
    Q_OBJECT
    Q_PROPERTY(bool mobileDataActive READ mobileDataActive WRITE setMobileDataActive NOTIFY mobileDataActiveChanged)
    Q_PROPERTY(bool allowRoaming READ allowRoaming WRITE setAllowRoaming NOTIFY allowRoamingChanged)
    Q_PROPERTY(QString activeAPNProfileUni READ activeAPNProfileUni WRITE setActiveAPNProfileUni NOTIFY activeBearerChanged)
    Q_PROPERTY(QList<NetworkType *> capabilities READ capabilities NOTIFY capabilitiesChanged)
    
public:
    MobileBroadbandSettings(QObject *parent, const QVariantList &args);
    virtual ~MobileBroadbandSettings();

    bool mobileDataActive();
    void setMobileDataActive(bool active);
    bool allowRoaming();
    void setAllowRoaming(bool allowRoaming);
    QString activeAPNProfileUni();
    void setActiveAPNProfileUni(QString uni);
    QList<NetworkType *> capabilities();

    Q_INVOKABLE void toggleCapability(NetworkType *nt);
    Q_INVOKABLE QString getModemDevice(); // get modem identifier
    
    // Capability flags: https://www.freedesktop.org/software/ModemManager/api/latest/ModemManager-Flags-and-Enumerations.html#MMModemCapability
    
    void updateActiveBearer();
    void updateBearerProfileModel();
    
Q_SIGNALS:
    void mobileDataActiveChanged();
    void allowRoamingChanged();
    void activeBearerChanged();
    void bearersChanged();
    void capabilitiesChanged();

private:
    bool m_mobileDataActive;
    
    QList<NetworkType *> m_capabilities;
    
    ModemManager::Bearer::Ptr m_bearer = nullptr;
    ModemManager::ModemDevice::Ptr m_modemDevice = nullptr;
    ModemManager::Modem::Ptr m_modemInterface = nullptr;
    MobileProviders* m_providers;
};

#endif // MOBILEBROADBANDSETTINGS_H
