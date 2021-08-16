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

#include "profilemodel.h"

ProfileModel::ProfileModel(QObject* parent)
{
    setParent(parent);
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_profileList.size();
}

int ProfileModel::count()
{
    return m_profileList.size();
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}

ProfileSettings *ProfileModel::get(int index)
{
    if ((index < 0) || (index >= m_profileList.count()))
        return {};
    
    return m_profileList.at(index);
}

void ProfileModel::remove(int index)
{
    if (index < 0 || index >= m_profileList.size())
        return;
    
    m_profileList.at(index)->deleteLater();
    
    Q_EMIT beginRemoveRows(QModelIndex(), index, index);
    m_profileList.removeAt(index);
    Q_EMIT endRemoveRows();
}

void ProfileModel::refresh(NetworkManager::Connection::List connections)
{
    Q_EMIT beginRemoveRows(QModelIndex(), 0, count()-1);
    m_profileList.clear();
    Q_EMIT endRemoveRows();
    
    Q_EMIT beginInsertRows(QModelIndex(), 0, connections.count());
    for (auto connection : connections) {
        for (auto setting : connection->settings()->settings()) {
            if (setting.dynamicCast<NetworkManager::GsmSetting>()) {
                m_profileList.append(new ProfileSettings(this, setting.dynamicCast<NetworkManager::GsmSetting>(), connection));
            } else if (setting.dynamicCast<NetworkManager::CdmaSetting>()) {
                m_profileList.append(new ProfileSettings(this, setting.dynamicCast<NetworkManager::CdmaSetting>(), connection));
            }
        }
    }
    Q_EMIT endInsertRows();
}

QString ProfileModel::networkTypeStr(NetworkManager::GsmSetting::NetworkType networkType)
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

NetworkManager::GsmSetting::NetworkType ProfileModel::networkTypeFlag(const QString &networkType)
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

ProfileSettings::ProfileSettings(QObject* parent, QString name, QString apn, QString user, QString password, NetworkManager::GsmSetting::NetworkType networkType, bool allowRoaming, QString connectionUni)
    : m_name(name)
    , m_apn(apn)
    , m_user(user)
    , m_password(password)
    , m_networkType(ProfileModel::instance()->networkTypeStr(networkType))
    , m_allowRoaming(allowRoaming)
    , m_connectionUni(connectionUni)
{
    setParent(parent);
}

ProfileSettings::ProfileSettings(QObject* parent, NetworkManager::Setting::Ptr setting, NetworkManager::Connection::Ptr connection)
    : m_connectionUni(connection->uuid())
{
    setParent(parent);
    
    NetworkManager::GsmSetting::Ptr gsmSetting = setting.staticCast<NetworkManager::GsmSetting>();
    
    if (gsmSetting) {
        m_gsm = true;
        m_name = connection->name();
        m_apn = gsmSetting->apn();
        m_user = gsmSetting->username();
        m_password = gsmSetting->password();
        m_networkType = ProfileModel::instance()->networkTypeStr(gsmSetting->networkType());
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

QVariantMap ProfileSettings::toSettings()
{
    if (m_gsm) {
        NetworkManager::GsmSetting gsmSetting;
        gsmSetting.setApn(m_apn);
        gsmSetting.setUsername(m_user);
        gsmSetting.setPassword(m_password);
        gsmSetting.setNetworkType(ProfileModel::instance()->networkTypeFlag(m_networkType));
        gsmSetting.setHomeOnly(!m_allowRoaming);
        return gsmSetting.toMap();
    } else {
        NetworkManager::CdmaSetting cdmaSetting;
        cdmaSetting.setUsername(m_user);
        cdmaSetting.setPassword(m_password);
        return cdmaSetting.toMap();
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

