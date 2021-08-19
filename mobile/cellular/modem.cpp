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

#include <KLocalizedString>
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
    connect(m_mmInterface.data(), &ModemManager::Modem::accessTechnologiesChanged, this, [this]() -> void { Q_EMIT accessTechnologiesChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::deviceChanged, this, [this]() -> void { Q_EMIT deviceChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::deviceIdentifierChanged, this, [this]() -> void { Q_EMIT deviceIdentifierChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::driversChanged, this, [this]() -> void { Q_EMIT driversChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::equipmentIdentifierChanged, this, [this]() -> void { Q_EMIT equipmentIdentifierChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::manufacturerChanged, this, [this]() -> void { Q_EMIT manufacturerChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::maxActiveBearersChanged, this, [this]() -> void { Q_EMIT maxActiveBearersChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::maxBearersChanged, this, [this]() -> void { Q_EMIT maxBearersChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::modelChanged, this, [this]() -> void { Q_EMIT modelChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::ownNumbersChanged, this, [this]() -> void { Q_EMIT ownNumbersChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::pluginChanged, this, [this]() -> void { Q_EMIT pluginChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::portsChanged, this, [this]() -> void { Q_EMIT portsChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::powerStateChanged, this, [this]() -> void { Q_EMIT powerStateChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::primaryPortChanged, this, [this]() -> void { Q_EMIT primaryPortChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::revisionChanged, this, [this]() -> void { Q_EMIT revisionChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::signalQualityChanged, this, [this]() -> void { Q_EMIT signalQualityChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::simPathChanged, this, [this]() -> void { Q_EMIT simPathChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::stateChanged, this, [this]() -> void { Q_EMIT stateChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::stateFailedReasonChanged, this, [this]() -> void { Q_EMIT stateFailedReasonChanged(); });
    connect(m_mmInterface.data(), &ModemManager::Modem::supportedCapabilitiesChanged, this, [this]() -> void { Q_EMIT supportedCapabilitiesChanged(); });
    
    connect(m_mmDevice.data(), &ModemManager::ModemDevice::simAdded, this, [this]() -> void { Q_EMIT simsChanged(); Q_EMIT hasSimChanged(); });
    connect(m_mmDevice.data(), &ModemManager::ModemDevice::simRemoved, this, [this]() -> void { Q_EMIT simsChanged(); Q_EMIT hasSimChanged(); });
    
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
    std::swap(m_mmInterface, other.m_mmInterface);
    std::swap(m_connectionSettingsType, other.m_connectionSettingsType);
    std::swap(m_profileList, other.m_profileList);
    std::swap(m_providers, other.m_providers);
}

QStringList Modem::accessTechnologies()
{
    QStringList list;
    auto flags = m_mmInterface->accessTechnologies();
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_UNKNOWN) {
        list.push_back("Unknown");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_POTS) {
        list.push_back("POTS");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_GSM) {
        list.push_back("GSM");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT) {
        list.push_back("GSM Compact");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_GPRS) {
        list.push_back("GPRS");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_EDGE) {
        list.push_back("EDGE");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_UMTS) {
        list.push_back("UMTS");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_HSDPA) {
        list.push_back("HSDPA");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_HSUPA) {
        list.push_back("HSUPA");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_HSPA) {
        list.push_back("HSPA");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS) {
        list.push_back("HSPA+");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_1XRTT) {
        list.push_back("CDMA2000 1xRTT");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_EVDO0) {
        list.push_back("CDMA2000 EVDO-0");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_EVDOA) {
        list.push_back("CDMA2000 EVDO-A");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_EVDOB) {
        list.push_back("CDMA2000 EVDO-B");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_LTE) {
        list.push_back("LTE");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_5GNR) {
        list.push_back("5GNR");
    }
    if (flags & MM_MODEM_ACCESS_TECHNOLOGY_ANY) {
        list.push_back("Any");
    }
    return list;
}

QString Modem::device()
{
    return m_mmInterface->device();
}

QString Modem::deviceIdentifier()
{
    return m_mmInterface->deviceIdentifier();
}

QStringList Modem::drivers()
{
    return m_mmInterface->drivers();
}

QString Modem::equipmentIdentifier()
{
    return m_mmInterface->equipmentIdentifier();
}

bool Modem::isEnabled()
{
    return m_mmInterface->isEnabled();
}

QString Modem::manufacturer()
{
    return m_mmInterface->manufacturer();
}

uint Modem::maxActiveBearers()
{
    return m_mmInterface->maxActiveBearers();
}

uint Modem::maxBearers()
{
    return m_mmInterface->maxBearers();
}

QString Modem::model()
{
    return m_mmInterface->model();
}

QStringList Modem::ownNumbers()
{
    return m_mmInterface->ownNumbers();
}

QString Modem::plugin()
{
    return m_mmInterface->plugin();
}

QStringList Modem::ports()
{
    QStringList ports;
    for (auto &port : m_mmInterface->ports()) {
        QString qs = port.name;
        switch (port.type) {
            case MM_MODEM_PORT_TYPE_UNKNOWN:
                qs += " [Unknown]";
                break;
            case MM_MODEM_PORT_TYPE_NET:
                qs += " [Net]";
                break;
            case MM_MODEM_PORT_TYPE_AT:
                qs += " [AT]";
                break;
            case MM_MODEM_PORT_TYPE_QCDM:
                qs += " [QCDM]";
                break;
            case MM_MODEM_PORT_TYPE_GPS:
                qs += " [GPS]";
                break;
            case MM_MODEM_PORT_TYPE_QMI:
                qs += " [QMI]";
                break;
            case MM_MODEM_PORT_TYPE_MBIM:
                qs += " [MBIM]";
                break;
            case MM_MODEM_PORT_TYPE_AUDIO:
                qs += " [Audio]";
                break;
            case MM_MODEM_PORT_TYPE_IGNORED:
                qs += " [Ignored]";
                break;
        }
        ports.push_back(qs);
    }
    return ports;
}

QString Modem::powerState()
{
    switch (m_mmInterface->powerState()) {
        case MM_MODEM_POWER_STATE_UNKNOWN:
            return i18n("Unknown");
        case MM_MODEM_POWER_STATE_OFF:
            return i18n("Off");
        case MM_MODEM_POWER_STATE_LOW:
            return i18n("Low-power mode");
        case MM_MODEM_POWER_STATE_ON:
            return i18n("Full power mode");
    }
    return "";
}

QString Modem::primaryPort()
{
    return m_mmInterface->primaryPort();
}

QString Modem::revision()
{
    return m_mmInterface->revision();
}

uint Modem::signalQuality()
{
    return m_mmInterface->signalQuality().signal;
}

QString Modem::simPath()
{
    return m_mmInterface->simPath();
}

QString Modem::state()
{
    switch (m_mmInterface->state()) {
        case MM_MODEM_STATE_FAILED:
            return i18n("Failed");
        case MM_MODEM_STATE_UNKNOWN:
            return i18n("Unknown");
        case MM_MODEM_STATE_INITIALIZING:
            return i18n("Initializing");
        case MM_MODEM_STATE_LOCKED:
            return i18n("Locked");
        case MM_MODEM_STATE_DISABLED:
            return i18n("Disabled");
        case MM_MODEM_STATE_DISABLING:
            return i18n("Disabling");
        case MM_MODEM_STATE_ENABLING:
            return i18n("Enabling");
        case MM_MODEM_STATE_ENABLED:
            return i18n("Enabled");
        case MM_MODEM_STATE_SEARCHING:
            return i18n("Searching for network provider"); 
        case MM_MODEM_STATE_REGISTERED:
            return i18n("Registered with network provider");
        case MM_MODEM_STATE_DISCONNECTING:
            return i18n("Disconnecting");
        case MM_MODEM_STATE_CONNECTING:
            return i18n("Connecting");
        case MM_MODEM_STATE_CONNECTED:
            return i18n("Connected");
    }
    return "";
}

QString Modem::stateFailedReason()
{
    switch (m_mmInterface->stateFailedReason()) {
        case MM_MODEM_STATE_FAILED_REASON_NONE:
            return i18n("No error.");
        case MM_MODEM_STATE_FAILED_REASON_UNKNOWN:
            return i18n("Unknown error.");
        case MM_MODEM_STATE_FAILED_REASON_SIM_MISSING:
            return i18n("SIM is required but missing.");
        case MM_MODEM_STATE_FAILED_REASON_SIM_ERROR:
            return i18n("SIM is available but unusable.");
    }
    return "";
}

QStringList Modem::supportedCapabilities()
{
    QStringList list;
    for (auto &cap : m_mmInterface->supportedCapabilities()) {
        switch (cap) {
            case MM_MODEM_CAPABILITY_NONE:
                list.push_back("None");
                break;
            case MM_MODEM_CAPABILITY_POTS:
                list.push_back("POTS");
                break;
            case MM_MODEM_CAPABILITY_CDMA_EVDO:
                list.push_back("CDMA/EVDO");
                break;
            case MM_MODEM_CAPABILITY_GSM_UMTS:
                list.push_back("GSM/UMTS");
                break;
            case MM_MODEM_CAPABILITY_LTE:
                list.push_back("LTE");
                break;
            case MM_MODEM_CAPABILITY_IRIDIUM:
                list.push_back("Iridium");
                break;
            case MM_MODEM_CAPABILITY_5GNR:
                list.push_back("5GNR");
                break;
            case MM_MODEM_CAPABILITY_ANY:
                list.push_back("Any");
                break;
        }
    }
    return list;
}

void Modem::reset() 
{
    qDebug() << "Resetting the modem...";
    QDBusPendingReply<void> reply = m_mmInterface->reset();
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "Error resetting the modem:" << reply.error().message();
    }
}

void Modem::setEnabled(bool enabled)
{
    if (enabled != isEnabled()) {
        qDebug() << (enabled ? "Enabling" : "Disabling") << "the modem...";
        QDBusPendingReply<void> reply = m_mmInterface->setEnabled(enabled);
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "Error setting the enabled state of the modem:" << reply.error().message();
        }
    }
}

QString Modem::uni() 
{
    return m_mmInterface->uni();
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

void Modem::addProfile(QString name, QString apn, QString username, QString password, QString networkType)
{
    NetworkManager::ConnectionSettings::Ptr settings;

    if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Gsm) {
        settings = NetworkManager::ConnectionSettings::Ptr{ new NetworkManager::ConnectionSettings(m_connectionSettingsType) };
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
        settings = NetworkManager::ConnectionSettings::Ptr{ new NetworkManager::ConnectionSettings(m_connectionSettingsType) };
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

void Modem::updateProfile(QString connectionUni, QString name, QString apn, QString username, QString password, QString networkType)
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
            
            if (modem3gpp) {
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
            }
        } else if (m_connectionSettingsType == NetworkManager::ConnectionSettings::Cdma) {
            // lookup apns with mccmnc codes
            modem3gpp = m_mmDevice->interface(ModemManager::ModemDevice::CdmaInterface).objectCast<ModemManager::Modem3gpp>();
            
            if (modem3gpp) {
                qWarning() << "Detecting profile settings. MCCMNC:" << modem3gpp->operatorCode() << "Provider:" << m_providers->getProvider(modem3gpp->operatorCode());
                
                QVariantMap cdmaInfo = m_providers->getCdmaInfo(m_providers->getProvider(modem3gpp->operatorCode()));
                // TODO determine what sid is for cdma
                addProfile(op, "", cdmaInfo["username"].toString(), cdmaInfo["password"].toString(), "4G/3G/2G");
            }
        }
    }
}

QList<Sim *> Modem::sims()
{
    if (m_mmDevice->sim()) {
        return { new Sim{ this, m_mmDevice->sim(), m_mmInterface } };
    }
    return {};
}

ModemManager::ModemDevice::Ptr Modem::mmModemDevice()
{
    return m_mmDevice;
}

NetworkManager::ModemDevice::Ptr Modem::nmModemDevice()
{
    return m_nmDevice;
}

ModemManager::Modem::Ptr Modem::mmModemInterface()
{
    return m_mmInterface;
}

ProfileSettings::ProfileSettings(QObject* parent, QString name, QString apn, QString user, QString password, NetworkManager::GsmSetting::NetworkType networkType, bool allowRoaming, QString connectionUni)
    : QObject{ parent },
      m_name(name), 
      m_apn(apn), 
      m_user(user), 
      m_password(password), 
      m_networkType(networkTypeStr(networkType)), 
      m_connectionUni(connectionUni),
      m_allowRoaming(allowRoaming)
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
