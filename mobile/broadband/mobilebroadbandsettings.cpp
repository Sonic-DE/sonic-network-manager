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

#include "mobilebroadbandsettings.h"

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>
#include <KUser>

#include <QQmlEngine>

K_PLUGIN_CLASS_WITH_JSON(MobileBroadbandSettings, "mobilebroadbandsettings.json")

MobileBroadbandSettings::MobileBroadbandSettings(QObject* parent, const QVariantList& args) : KQuickAddons::ConfigModule(parent, args)
{
    KAboutData* about = new KAboutData("kcm_mobile_broadband", i18n("Configure mobile broadband"),
                                       "0.1", QString(), KAboutLicense::GPL);
    about->addAuthor(i18n("Martin Kacej"), QString(), "m.kacej@atlas.sk");
    about->addAuthor(i18n("Devin Lin"), QString(), "espidev@gmail.com");
    setAboutData(about);
    
    qmlRegisterSingletonInstance("mobilebroadbandkcm", 1, 0, "ProfileModel", ProfileModel::instance());
    qmlRegisterType<ProfileSettings>("mobilebroadbandkcm", 1, 0, "ProfileSettings");
    
    // find modem
    ModemManager::scanDevices();
    m_modemDevice = ModemManager::findModemDevice(getModemDevice()); // TODO check if modem changes?
    
    if (m_modemDevice) {
        m_modemInterface = m_modemDevice->modemInterface();
        
        // add NetworkManager device
        m_nmModem = NetworkManager::findNetworkInterface(m_modemDevice->uni()).objectCast<NetworkManager::ModemDevice>();
        
        // determine type
        if (m_nmModem) {
            if (m_nmModem->currentCapabilities() & NetworkManager::ModemDevice::CdmaEvdo) {
                m_nmModemType = NetworkManager::ConnectionSettings::Cdma;
            } else if ((m_nmModem->currentCapabilities() & NetworkManager::ModemDevice::GsmUmts) || (m_nmModem->currentCapabilities() & NetworkManager::ModemDevice::Lte)) {
                m_nmModemType = NetworkManager::ConnectionSettings::Gsm;
            }
            
            // modem device signals
            connect(m_nmModem.data(), &NetworkManager::ModemDevice::availableConnectionChanged, this, [this]() -> void { 
                ProfileModel::instance()->refresh(m_nmModem->availableConnections());
                Q_EMIT allowRoamingChanged();
            });
            connect (m_nmModem.data(), &NetworkManager::ModemDevice::activeConnectionChanged, this, [this]() -> void {
                Q_EMIT allowRoamingChanged();
                Q_EMIT activeConnectionUniChanged();
            });
        }
    }
}

MobileBroadbandSettings::~MobileBroadbandSettings()
{
//     delete m_providers;
}

bool MobileBroadbandSettings::mobileDataActive()
{
    return m_mobileDataActive;
}

void MobileBroadbandSettings::setMobileDataActive(bool active)
{
    if (m_nmModem) {
        if (m_mobileDataActive != active && active) { // turn on mobile data
            m_mobileDataActive = active;
            
            // activate connection that already has autoconnect set to true
            for (auto connection : m_nmModem->availableConnections()) {
                if (connection->settings()->autoconnect()) {
                    activateProfile(connection->uuid());
                }
            }
            Q_EMIT mobileDataActiveChanged();
        } else if (m_mobileDataActive == active && !active) { // turn off mobile data
            QDBusPendingReply reply = m_nmModem->disconnectInterface(); 
            reply.waitForFinished();
            
            if (reply.isError()) {
                qWarning() << "Error disconnecting modem interface" << reply.error().message();
            }
            Q_EMIT mobileDataActiveChanged();
        }
    }
}

bool MobileBroadbandSettings::allowRoaming()
{
    if (m_nmModem && m_nmModem->activeConnection() && m_nmModem->activeConnection()->connection()) {
        if (m_nmModemType == NetworkManager::ConnectionSettings::Gsm) { // gsm
            NetworkManager::Setting::Ptr setting = m_nmModem->activeConnection()->connection()->settings()->setting(NetworkManager::Setting::Gsm);
            NetworkManager::GsmSetting::Ptr gsmSetting = setting.staticCast<NetworkManager::GsmSetting>();
            return !gsmSetting->homeOnly();
        } else { // cdma
            return false; // TODO
        }
    }
    return false;
}

void MobileBroadbandSettings::setAllowRoaming(bool allowRoaming)
{
    if (m_nmModem && m_nmModem->activeConnection() && m_nmModem->activeConnection()->connection()) {
        if (m_nmModemType == NetworkManager::ConnectionSettings::Gsm) { // gsm
            NetworkManager::Setting::Ptr setting = m_nmModem->activeConnection()->connection()->settings()->setting(NetworkManager::Setting::Gsm);
            setting.staticCast<NetworkManager::GsmSetting>()->setHomeOnly(allowRoaming);
        } else { // cdma
            // TODO
        }
    }
}

QString MobileBroadbandSettings::activeConnectionUni()
{
    if (m_nmModem && m_nmModem->activeConnection() && m_nmModem->activeConnection()->connection()) {
        return m_nmModem->activeConnection()->connection()->uuid();
    }
    return QString();
}

void MobileBroadbandSettings::detectProfileSettings()
{
    // TODO
    
    //     m_providers = new MobileProviders();
    //     m_providers->getApns();
}

QString MobileBroadbandSettings::getModemDevice()
{
    ModemManager::ModemDevice::List list = ModemManager::modemDevices();
    if (list.length() == 0) {
        return QString();
    }
    
    ModemManager::ModemDevice::Ptr device;
    foreach (const ModemManager::ModemDevice::Ptr &md, list) {
        ModemManager::Modem::Ptr m = md->modemInterface();
        if (!m->isEnabled())
            continue;
        // TODO powerState ???
        if (m->state() <= MM_MODEM_STATE_REGISTERED)
            continue; // needs inspection
//         if (m->accessTechnologies() <= MM_MODEM_ACCESS_TECHNOLOGY_GSM)
//             continue;
//         if (m->currentCapabilities() <= MM_MODEM_CAPABILITY_GSM_UMTS)
//             continue;
        device = md;
    }
    if (device) {
        qWarning() << device->uni() << device->modemInterface()->uni();
        return device->uni();
    }
    return QString();
}

void MobileBroadbandSettings::activateProfile(const QString &connectionUni)
{
    if (m_nmModem) {
        // disable autoconnect for all other connections
        for (auto connection : m_nmModem->availableConnections()) {
            if (connection->uuid() == connectionUni) {
                connection->settings()->setAutoconnect(true);
            } else {
                connection->settings()->setAutoconnect(false);
            }
        }
        
        // activate connection manually
        QDBusPendingReply<QDBusObjectPath> reply = NetworkManager::activateConnection(connectionUni, m_nmModem->uni(), "");
        reply.waitForFinished(); // TODO async is better
        if (reply.isError()) {
            qWarning() << "Error activating connection" << reply.error().message();
        }
    }
}

void MobileBroadbandSettings::addProfile(const QString &name, const QString &apn, const QString &username, const QString &password, NetworkManager::GsmSetting::NetworkType networkType)
{
    if (m_nmModem) {
        NetworkManager::ConnectionSettings::Ptr settings;
    
        if (m_nmModemType == NetworkManager::ConnectionSettings::Gsm) {
            settings = NetworkManager::ConnectionSettings::Ptr(new NetworkManager::ConnectionSettings(NetworkManager::ConnectionSettings::Gsm));
            settings->setId(name);
            settings->setUuid(NetworkManager::ConnectionSettings::createNewUuid());
            settings->setAutoconnect(true);
            settings->addToPermissions(KUser().loginName(), QString());
            
            NetworkManager::GsmSetting::Ptr gsmSetting = settings->setting(NetworkManager::Setting::Gsm).dynamicCast<NetworkManager::GsmSetting>();
            gsmSetting->setApn(apn);
            gsmSetting->setUsername(username);
            gsmSetting->setPassword(password);
            gsmSetting->setNetworkType(networkType);
            gsmSetting->setHomeOnly(false); // TODO set roaming to user's preferences
            
        } else if (m_nmModemType == NetworkManager::ConnectionSettings::Cdma){
            settings = NetworkManager::ConnectionSettings::Ptr(new NetworkManager::ConnectionSettings(NetworkManager::ConnectionSettings::Cdma));
            settings->setId(name);
            settings->setUuid(NetworkManager::ConnectionSettings::createNewUuid());
            settings->setAutoconnect(true);
            settings->addToPermissions(KUser().loginName(), QString());
            
            NetworkManager::CdmaSetting::Ptr cdmaSetting = settings->setting(NetworkManager::Setting::Cdma).dynamicCast<NetworkManager::CdmaSetting>();
            cdmaSetting->setUsername(username);
            cdmaSetting->setPassword(password);
        }
        
        QDBusPendingReply<QDBusObjectPath> reply = NetworkManager::addConnection(settings->toMap());
        reply.waitForFinished(); // TODO do it async
        if (reply.isError()) {
            qWarning() << "Error adding connection" << reply.error().message();
        }
        
        activateProfile(settings->uuid());
    }
}



#include "mobilebroadbandsettings.moc"
