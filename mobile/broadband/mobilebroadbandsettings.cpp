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
#include <modem3gpp.h>

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
    
    // parse mobile providers list
    m_providers = new MobileProviders();
    m_providers->fillProvidersList();
    
    if (m_modemDevice) {
        m_modemInterface = m_modemDevice->modemInterface();
        
        // add NetworkManager device
        m_nmModem = NetworkManager::findNetworkInterface(m_modemDevice->uni()).objectCast<NetworkManager::ModemDevice>();
        
        if (m_nmModem) {
            // determine type
            if (m_nmModem->currentCapabilities() & NetworkManager::ModemDevice::CdmaEvdo) {
                m_nmModemType = NetworkManager::ConnectionSettings::Cdma;
            } else if ((m_nmModem->currentCapabilities() & NetworkManager::ModemDevice::GsmUmts) || (m_nmModem->currentCapabilities() & NetworkManager::ModemDevice::Lte)) {
                m_nmModemType = NetworkManager::ConnectionSettings::Gsm;
            }
            
            // determine if no connections are added, and the default settings should be found
            if (m_nmModem->availableConnections().empty()) {
                detectProfileSettings();
            }
            
            // modem device signals
            connect(m_nmModem.data(), &NetworkManager::ModemDevice::availableConnectionChanged, this, [this]() -> void { 
                ProfileModel::instance()->refresh(m_nmModem->availableConnections());
                Q_EMIT allowRoamingChanged();
            });
            connect(m_nmModem.data(), &NetworkManager::ModemDevice::activeConnectionChanged, this, [this]() -> void {
                Q_EMIT allowRoamingChanged();
                Q_EMIT activeConnectionUniChanged();
            });
            connect(m_nmModem.data(), &NetworkManager::ModemDevice::stateChanged, this, [this](NetworkManager::Device::State newstate, NetworkManager::Device::State oldstate, NetworkManager::Device::StateChangeReason reason) -> void {
                Q_EMIT mobileDataActiveChanged();
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
    if (m_nmModem) {
        if (m_nmModem->state() == NetworkManager::Device::State::UnknownState ||
            m_nmModem->state() == NetworkManager::Device::State::Unmanaged ||
            m_nmModem->state() == NetworkManager::Device::State::Unavailable ||
            m_nmModem->state() == NetworkManager::Device::State::Disconnected ||
            m_nmModem->state() == NetworkManager::Device::State::Deactivating ||
            m_nmModem->state() == NetworkManager::Device::State::Failed) {
                return false;
        } else {
            return m_nmModem->activeConnection() != nullptr;
        }
    }
    return false;
}

void MobileBroadbandSettings::setMobileDataActive(bool active)
{
    if (m_nmModem) {
        if (!mobileDataActive() && active) { // turn on mobile data
            // activate connection that already has autoconnect set to true
            for (auto connection : m_nmModem->availableConnections()) {
                if (connection->settings()->autoconnect()) {
                    activateProfile(connection->uuid());
                }
            }
            Q_EMIT mobileDataActiveChanged();
        } else if (mobileDataActive() && !active) { // turn off mobile data
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
    if (m_modemDevice) {
        QString op = m_modemDevice->sim()->operatorName();
        
        // currently we use operator names directly, it may be better to switch to lookup through mcc-mnc ids fetched from ModemManager's 3GPP interface
        // m_providers->getProvider(mccmnc)
        ModemManager::Modem3gpp::Ptr modem3gpp;
        
        qWarning() << "Detecting profile settings. Operator:" << op;
        
        if (m_nmModemType == NetworkManager::ConnectionSettings::Gsm) {
            modem3gpp = m_modemDevice->interface(ModemManager::ModemDevice::GsmInterface).objectCast<ModemManager::Modem3gpp>();
            
            qWarning() << "Detecting profile settings. MCCMNC:" << modem3gpp->operatorCode() << "Provider:" << m_providers->getProvider(modem3gpp->operatorCode());

            QStringList apns = m_providers->getApns(m_providers->getProvider(modem3gpp->operatorCode()));
            
            for (auto apn : apns) {
                QVariantMap apnInfo = m_providers->getApnInfo(apn);
                qWarning() << "Found gsm profile settings. Type:" << apnInfo["usageType"];
                
                // only add mobile data apns (not mms)
                if (apnInfo["usageType"].toString() == "internet") {
                    QString name = op;
                    if (!apnInfo["name"].isNull()) {
                        name += " - " + apnInfo["name"].toString();
                    }
                    
                    addProfile(name, apn, apnInfo["username"].toString(), apnInfo["password"].toString(), "4G/3G/2G");
                }
                // in the future for MMS settings, add else if here for == "mms"
            }
        } else if (m_nmModemType == NetworkManager::ConnectionSettings::Cdma) {
            modem3gpp = m_modemDevice->interface(ModemManager::ModemDevice::CdmaInterface).objectCast<ModemManager::Modem3gpp>();
            
            qWarning() << "Detecting profile settings. MCCMNC:" << modem3gpp->operatorCode() << "Provider:" << m_providers->getProvider(modem3gpp->operatorCode());
            
            QVariantMap cdmaInfo = m_providers->getCdmaInfo(m_providers->getProvider(modem3gpp->operatorCode()));
            // TODO determine what sid is for cdma
            addProfile(op, "", cdmaInfo["username"].toString(), cdmaInfo["password"].toString(), "4G/3G/2G");
        }
    }
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
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << "Error activating connection" << reply.error().message();
        }
    }
}

void MobileBroadbandSettings::addProfile(const QString &name, const QString &apn, const QString &username, const QString &password, const QString &networkType)
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
            gsmSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
            gsmSetting->setNetworkType(ProfileModel::instance()->networkTypeFlag(networkType));
            gsmSetting->setHomeOnly(allowRoaming());
            
        } else if (m_nmModemType == NetworkManager::ConnectionSettings::Cdma){
            settings = NetworkManager::ConnectionSettings::Ptr(new NetworkManager::ConnectionSettings(NetworkManager::ConnectionSettings::Cdma));
            settings->setId(name);
            settings->setUuid(NetworkManager::ConnectionSettings::createNewUuid());
            settings->setAutoconnect(true);
            settings->addToPermissions(KUser().loginName(), QString());
            
            NetworkManager::CdmaSetting::Ptr cdmaSetting = settings->setting(NetworkManager::Setting::Cdma).dynamicCast<NetworkManager::CdmaSetting>();
            cdmaSetting->setUsername(username);
            cdmaSetting->setPassword(password);
            cdmaSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
        }
        
        QDBusPendingReply<QDBusObjectPath> reply = NetworkManager::addConnection(settings->toMap());
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << "Error adding connection" << reply.error().message();
        }
        
        activateProfile(settings->uuid()); // TODO the uuid may have have to be Connection's uuid, not ConnectionSettings's uuid
    }
}

void MobileBroadbandSettings::removeProfile(const QString &connectionUni)
{
    NetworkManager::Connection::Ptr con = NetworkManager::findConnectionByUuid(connectionUni);
    if (con) {
        QDBusPendingReply reply = con->remove();
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << "Error removing connection" << reply.error().message();
        }
    }
}

void MobileBroadbandSettings::updateProfile(const QString &uni, const QString &name, const QString &apn, const QString &username, const QString &password, const QString &networkType)
{
    NetworkManager::Connection::Ptr con = NetworkManager::findConnectionByUuid(uni);
    if (con) {
        NetworkManager::ConnectionSettings::Ptr conSettings = con->settings();
        if (conSettings) {
            conSettings->setId(name);
            
            if (m_nmModemType == NetworkManager::ConnectionSettings::Gsm) {
                NetworkManager::GsmSetting::Ptr gsmSetting = conSettings->setting(NetworkManager::Setting::Gsm).dynamicCast<NetworkManager::GsmSetting>();
                gsmSetting->setApn(apn);
                gsmSetting->setUsername(username);
                gsmSetting->setPassword(password);
                gsmSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
                gsmSetting->setNetworkType(ProfileModel::instance()->networkTypeFlag(networkType));
            } else if (m_nmModemType == NetworkManager::ConnectionSettings::Cdma) {
                NetworkManager::CdmaSetting::Ptr cdmaSetting = conSettings->setting(NetworkManager::Setting::Cdma).dynamicCast<NetworkManager::CdmaSetting>();
                cdmaSetting->setUsername(username);
                cdmaSetting->setPassword(password);
                cdmaSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
            }
        }
    }
}

#include "mobilebroadbandsettings.moc"
