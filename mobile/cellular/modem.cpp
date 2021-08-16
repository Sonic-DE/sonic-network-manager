/*
 *   Copyright 2021 Devin Lin <espidev@gmail.com>
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

#include "modem.h"

#include <utility>

#include <KUser>

#include <modem3gpp.h>

QString networkTypeStr(NetworkManager::GsmSetting::NetworkType networkType)
{
    if (networkType == NetworkManager::GsmSetting::NetworkType::Any) {
        return "Any";
    } else if (networkType == NetworkManager::GsmSetting::NetworkType::GprsEdgeOnly) {
        return "Only 2G";
    } else if (networkType == NetworkManager::GsmSetting::NetworkType::Only3G) {
        return "Only 3G";
    } else if (networkType == NetworkManager::GsmSetting::NetworkType::Only4GLte) {
        return "Only 4G";
    } else if (networkType == NetworkManager::GsmSetting::NetworkType::Prefer2G) {
        return "2G";
    } else if (networkType == NetworkManager::GsmSetting::NetworkType::Prefer3G) {
        return "3G/2G";
    } else if (networkType == NetworkManager::GsmSetting::NetworkType::Prefer4GLte) {
        return "4G/3G/2G";
    }
    return "Any";
}

NetworkManager::GsmSetting::NetworkType networkTypeFlag(const QString &networkType)
{
    if (networkType == "Any") {
        return NetworkManager::GsmSetting::NetworkType::Any;
    } else if (networkType == "Only 2G") {
        return NetworkManager::GsmSetting::NetworkType::GprsEdgeOnly;
    } else if (networkType == "Only 3G") {
        return NetworkManager::GsmSetting::NetworkType::Only3G;
    } else if (networkType == "Only 4G") {
        return NetworkManager::GsmSetting::NetworkType::Only4GLte;
    } else if (networkType == "2G") {
        return NetworkManager::GsmSetting::NetworkType::Prefer2G;
    } else if (networkType == "3G/2G") {
        return NetworkManager::GsmSetting::NetworkType::Prefer3G;
    } else if (networkType == "4G/3G/2G") {
        return NetworkManager::GsmSetting::NetworkType::Prefer4GLte;
    }
    return NetworkManager::GsmSetting::NetworkType::Any;
}

QString nmDeviceStateStr(NetworkManager::Device::State state)
{
    if (state == NetworkManager::Device::State::UnknownState) return "Unknown";
    else if (state == NetworkManager::Device::State::Unmanaged) return "Unmanaged";
    else if (state == NetworkManager::Device::State::Unavailable) return "Unavailable";
    else if (state == NetworkManager::Device::State::Disconnected) return "Disconnected";
    else if (state == NetworkManager::Device::State::Preparing) return "Preparing";
    else if (state == NetworkManager::Device::State::ConfiguringHardware) return "ConfiguringHardware";
    else if (state == NetworkManager::Device::State::NeedAuth) return "NeedAuth";
    else if (state == NetworkManager::Device::State::ConfiguringIp) return "ConfiguringIp";
    else if (state == NetworkManager::Device::State::CheckingIp) return "CheckingIp";
    else if (state == NetworkManager::Device::State::WaitingForSecondaries) return "WaitingForSecondaries";
    else if (state == NetworkManager::Device::State::Activated) return "Activated";
    else if (state == NetworkManager::Device::State::Deactivating) return "Deactivating";
    else if (state == NetworkManager::Device::State::Failed) return "Failed";
    else return "";
}

Modem::Modem(QObject *parent, ModemManager::ModemDevice::Ptr mmDevice, NetworkManager::ModemDevice::Ptr nmDevice , ModemManager::Modem::Ptr mmInterface, MobileProviders *providers) 
    : QObject{ parent },
      m_mmDevice{ mmDevice },
      m_nmDevice{ nmDevice },
      m_mmInterface{ mmInterface },
      m_providers{ providers }
{
    connect(m_mmDevice.data(), &ModemManager::ModemDevice::simAdded, this, [this]() -> void { Q_EMIT hasSimChanged(); });
    connect(m_mmDevice.data(), &ModemManager::ModemDevice::simRemoved, this, [this]() -> void { Q_EMIT hasSimChanged(); });
    
    // determine type of connection profile
    if (m_nmDevice->currentCapabilities() & NetworkManager::ModemDevice::CdmaEvdo) {
        m_connectionSettingsType = NetworkManager::ConnectionSettings::Cdma;
    } else if ((m_nmDevice->currentCapabilities() & NetworkManager::ModemDevice::GsmUmts) || (m_nmDevice->currentCapabilities() & NetworkManager::ModemDevice::Lte)) {
        m_connectionSettingsType = NetworkManager::ConnectionSettings::Gsm;
    }
    
    // determine if no connections are added, and the default settings should be found
    if (m_nmDevice->availableConnections().empty()) {
        detectProfileSettings();
    }
    
    // add profiles
    refreshProfiles();
    connect(m_nmDevice.data(), &NetworkManager::ModemDevice::availableConnectionChanged, this, [this]() -> void { 
        refreshProfiles();
    });
    connect(m_nmDevice.data(), &NetworkManager::ModemDevice::activeConnectionChanged, this, [this]() -> void {
        Q_EMIT activeConnectionUniChanged();
    });
    
    connect(m_nmDevice.data(), &NetworkManager::ModemDevice::stateChanged, this, [this](NetworkManager::Device::State newstate, NetworkManager::Device::State oldstate, NetworkManager::Device::StateChangeReason reason) -> void {
        qDebug() << "Modem" << m_nmDevice->uni() << "changed state:" << nmDeviceStateStr(oldstate) << "->" << nmDeviceStateStr(newstate);
        Q_EMIT mobileDataActiveChanged();
    });
}

Modem &Modem::operator=(Modem &&other)
{
    swap(other);
    return *this;
}

void Modem::swap(Modem &other)
{
    std::swap(m_mmDevice, other.m_mmDevice);
    std::swap(m_nmDevice, other.m_nmDevice);
    std::swap(m_mmInterface, other.m_mmInterface);
    std::swap(m_connectionSettingsType, other.m_connectionSettingsType);
    std::swap(m_profileList, other.m_profileList);
    std::swap(m_providers, other.m_providers);
}

bool Modem::mobileDataActive()
{
    if (m_nmDevice) {
        if (m_nmDevice->state() == NetworkManager::Device::State::UnknownState ||
            m_nmDevice->state() == NetworkManager::Device::State::Unmanaged ||
            m_nmDevice->state() == NetworkManager::Device::State::Unavailable ||
            m_nmDevice->state() == NetworkManager::Device::State::Disconnected ||
            m_nmDevice->state() == NetworkManager::Device::State::Deactivating ||
            m_nmDevice->state() == NetworkManager::Device::State::Failed) {
                return false;
        } else {
            return m_nmDevice->activeConnection() != nullptr;
        }
    }
    return false;
}

void Modem::setMobileDataActive(bool active)
{
    if (active && !mobileDataActive()) { // turn on mobile data
        // activate connection that already has autoconnect set to true
        for (auto connection : m_nmDevice->availableConnections()) {
            if (connection->settings()->autoconnect()) {
                activateProfile(connection->uuid());
            }
        }
        Q_EMIT mobileDataActiveChanged();
    } else if (!active && mobileDataActive()) { // turn off mobile data
        QDBusPendingReply reply = m_nmDevice->disconnectInterface(); 
        reply.waitForFinished();
        
        if (reply.isError()) {
            qWarning() << "Error disconnecting modem interface" << reply.error().message();
        }
        Q_EMIT mobileDataActiveChanged();
    }
}

bool Modem::hasSim()
{
    return m_mmDevice->sim() != nullptr;
}

QList<ProfileSettings *> &Modem::profileList()
{
    return m_profileList;
}

QString Modem::activeConnectionUni()
{
    if (m_nmDevice->activeConnection() && m_nmDevice->activeConnection()->connection()) {
        return m_nmDevice->activeConnection()->connection()->uuid();
    }
    return QString();
}

void Modem::refreshProfiles()
{
    m_profileList.clear();
    for (auto connection : m_nmDevice->availableConnections()) {
        for (auto setting : connection->settings()->settings()) {
            if (setting.dynamicCast<NetworkManager::GsmSetting>()) {
                m_profileList.append(new ProfileSettings(this, setting.dynamicCast<NetworkManager::GsmSetting>(), connection));
            } else if (setting.dynamicCast<NetworkManager::CdmaSetting>()) {
                m_profileList.append(new ProfileSettings(this, setting.dynamicCast<NetworkManager::CdmaSetting>(), connection));
            }
        }
    }
    Q_EMIT profileListChanged();
}

void Modem::activateProfile(const QString &connectionUni)
{
    qDebug() << "Activating profile on modem" << m_nmDevice->uni() << "for connection" << connectionUni << "."; 
    
    // disable autoconnect for all other connections
    for (auto connection : m_nmDevice->availableConnections()) {
        if (connection->uuid() == connectionUni) {
            connection->settings()->setAutoconnect(true);
        } else {
            connection->settings()->setAutoconnect(false);
        }
    }
    
    // activate connection manually
    QDBusPendingReply<QDBusObjectPath> reply = NetworkManager::activateConnection(connectionUni, m_nmDevice->uni(), "");
    reply.waitForFinished(); // TODO performance?
    if (reply.isError()) {
        qWarning() << "Error activating connection" << reply.error().message();
    }
}

void Modem::addProfile(const QString &name, const QString &apn, const QString &username, const QString &password, const QString &networkType)
{
    NetworkManager::ConnectionSettings::Ptr settings;

    if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Gsm) {
        settings = NetworkManager::ConnectionSettings::Ptr(new NetworkManager::ConnectionSettings(m_connectionSettingsType));
        settings->setId(name);
        settings->setUuid(NetworkManager::ConnectionSettings::createNewUuid());
        settings->setAutoconnect(true);
        settings->addToPermissions(KUser().loginName(), QString());
        
        NetworkManager::GsmSetting::Ptr gsmSetting = settings->setting(NetworkManager::Setting::Gsm).dynamicCast<NetworkManager::GsmSetting>();
        gsmSetting->setApn(apn);
        gsmSetting->setUsername(username);
        gsmSetting->setPassword(password);
        gsmSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
        gsmSetting->setNetworkType(networkTypeFlag(networkType));
        gsmSetting->setHomeOnly(true); // TODO roaming
        
    } else if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Cdma){
        settings = NetworkManager::ConnectionSettings::Ptr(new NetworkManager::ConnectionSettings(m_connectionSettingsType));
        settings->setId(name);
        settings->setUuid(NetworkManager::ConnectionSettings::createNewUuid());
        settings->setAutoconnect(true);
        settings->addToPermissions(KUser().loginName(), QString());
        
        NetworkManager::CdmaSetting::Ptr cdmaSetting = settings->setting(NetworkManager::Setting::Cdma).dynamicCast<NetworkManager::CdmaSetting>();
        cdmaSetting->setUsername(username);
        cdmaSetting->setPassword(password);
        cdmaSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
    }
    
    QDBusPendingReply<QDBusObjectPath> reply = NetworkManager::addAndActivateConnection(settings->toMap(), m_nmDevice->uni(), "");
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Error adding connection" << reply.error().message();
    } else {
        qDebug() << "Successfully added a new connection" << name << "with APN" << apn << ".";
    }
}

void Modem::removeProfile(const QString &connectionUni)
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

void Modem::updateProfile(const QString &connectionUni, const QString &name, const QString &apn, const QString &username, const QString &password, const QString &networkType)
{
    NetworkManager::Connection::Ptr con = NetworkManager::findConnectionByUuid(connectionUni);
    if (con) {
        NetworkManager::ConnectionSettings::Ptr conSettings = con->settings();
        if (conSettings) {
            conSettings->setId(name);
            
            if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Gsm) {
                NetworkManager::GsmSetting::Ptr gsmSetting = conSettings->setting(NetworkManager::Setting::Gsm).dynamicCast<NetworkManager::GsmSetting>();
                gsmSetting->setApn(apn);
                gsmSetting->setUsername(username);
                gsmSetting->setPassword(password);
                gsmSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
                gsmSetting->setNetworkType(networkTypeFlag(networkType));
            } else if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Cdma) {
                NetworkManager::CdmaSetting::Ptr cdmaSetting = conSettings->setting(NetworkManager::Setting::Cdma).dynamicCast<NetworkManager::CdmaSetting>();
                cdmaSetting->setUsername(username);
                cdmaSetting->setPassword(password);
                cdmaSetting->setPasswordFlags(password == "" ? NetworkManager::Setting::NotRequired : NetworkManager::Setting::AgentOwned);
            }
            
            QDBusPendingReply reply = con->update(conSettings->toMap());
            reply.waitForFinished();
            if (reply.isError()) {
                qWarning() << "Error updating connection settings for" << connectionUni << ".";
            } else {
                qDebug() << "Successfully updated connection settings for" << connectionUni << ".";
            }
            
        } else {
            qWarning() << "Could not find connection settings for" << connectionUni << "to update!";
        }
    } else {
        qWarning() << "Could not find connection" << connectionUni << "to update!";
    }
}

void Modem::detectProfileSettings()
{
    if (m_mmDevice && hasSim()) {
        QString op = m_mmDevice->sim()->operatorName();
        
        ModemManager::Modem3gpp::Ptr modem3gpp;
        
        qWarning() << "Detecting profile settings. Operator:" << op;
        
        if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Gsm) {
            modem3gpp = m_mmDevice->interface(ModemManager::ModemDevice::GsmInterface).objectCast<ModemManager::Modem3gpp>();
            
            qWarning() << "Detecting profile settings. MCCMNC:" << modem3gpp->operatorCode() << "Provider:" << m_providers->getProvider(modem3gpp->operatorCode());

            // lookup apns with mccmnc codes
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
        } else if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Cdma) {
            // lookup apns with mccmnc codes
            modem3gpp = m_mmDevice->interface(ModemManager::ModemDevice::CdmaInterface).objectCast<ModemManager::Modem3gpp>();
            
            qWarning() << "Detecting profile settings. MCCMNC:" << modem3gpp->operatorCode() << "Provider:" << m_providers->getProvider(modem3gpp->operatorCode());
            
            QVariantMap cdmaInfo = m_providers->getCdmaInfo(m_providers->getProvider(modem3gpp->operatorCode()));
            // TODO determine what sid is for cdma
            addProfile(op, "", cdmaInfo["username"].toString(), cdmaInfo["password"].toString(), "4G/3G/2G");
        }
    }
}

ProfileSettings::ProfileSettings(QObject* parent, QString name, QString apn, QString user, QString password, NetworkManager::GsmSetting::NetworkType networkType, bool allowRoaming, QString connectionUni)
    : QObject{ parent },
      m_name(name), 
      m_apn(apn), 
      m_user(user), 
      m_password(password), 
      m_networkType(networkTypeStr(networkType)), 
      m_allowRoaming(allowRoaming), 
      m_connectionUni(connectionUni)
{
    setParent(parent);
}

ProfileSettings::ProfileSettings(QObject* parent, NetworkManager::Setting::Ptr setting, NetworkManager::Connection::Ptr connection)
    : QObject{ parent },
      m_connectionUni(connection->uuid())
{
    setParent(parent);
    
    NetworkManager::GsmSetting::Ptr gsmSetting = setting.staticCast<NetworkManager::GsmSetting>();
    
    if (gsmSetting) {
        m_gsm = true;
        m_name = connection->name();
        m_apn = gsmSetting->apn();
        m_user = gsmSetting->username();
        m_password = gsmSetting->password();
        m_networkType = networkTypeStr(gsmSetting->networkType());
        m_allowRoaming = !gsmSetting->homeOnly();
    } else {
        NetworkManager::CdmaSetting::Ptr cdmaSetting = setting.staticCast<NetworkManager::CdmaSetting>();
        m_gsm = false;
        m_name = connection->name();
        m_apn = "";
        m_user = cdmaSetting->username();
        m_password = cdmaSetting->password();
        m_networkType = NetworkManager::GsmSetting::NetworkType::Prefer4GLte; // TODO
        m_allowRoaming = false;
    }
}

QString ProfileSettings::name()
{
    return m_name;
}

QString ProfileSettings::apn()
{
    return m_apn;
}

void ProfileSettings::setApn(QString apn)
{
    if (apn != m_apn) {
        m_apn = apn;
        Q_EMIT apnChanged();
    }
}

QString ProfileSettings::user()
{
    return m_user;
}

void ProfileSettings::setUser(QString user)
{
    if (user != m_user) {
        m_user = user;
        Q_EMIT userChanged();
    }
}

QString ProfileSettings::password()
{
    return m_password;
}

void ProfileSettings::setPassword(QString password)
{
    if (password != m_password) {
        m_password = password; 
        Q_EMIT passwordChanged();
    }
}

bool ProfileSettings::allowRoaming()
{
    return m_allowRoaming;
}

void ProfileSettings::setAllowRoaming(bool allowRoaming)
{
    if (allowRoaming != m_allowRoaming) {
        m_allowRoaming = allowRoaming;
        Q_EMIT allowRoamingChanged();
    }
}

QString ProfileSettings::networkType() {
    return m_networkType;
}

void ProfileSettings::setNetworkType(QString networkType) {
    if (networkType != m_networkType) {
        m_networkType = networkType;
        Q_EMIT networkTypeChanged();
    }
}

QString ProfileSettings::connectionUni()
{
    return m_connectionUni;
}
