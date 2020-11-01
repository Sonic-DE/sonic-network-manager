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

#include "apnprofilemodel.h"

APNProfileModel::APNProfileModel(QObject* parent)
{
    setParent(parent);
}

int APNProfileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_apnprofileList.size();
}

int APNProfileModel::count()
{
    return m_apnprofileList.size();
}

QVariant APNProfileModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}

APNProfile *APNProfileModel::get(int index)
{
    if ((index < 0) || (index >= m_apnprofileList.count()))
        return {};
    
    return m_apnprofileList.at(index);
}

void APNProfileModel::remove(int index)
{
    if (index < 0 || index >= m_apnprofileList.size())
        return;
    
    m_apnprofileList.at(index)->deleteLater();
    
    Q_EMIT beginRemoveRows(QModelIndex(), index, index);
    m_apnprofileList.removeAt(index);
    Q_EMIT endRemoveRows();
}

APNProfile *APNProfileModel::defaultProfile()
{
    return new APNProfile(this, "", "", "", false, MM_BEARER_ALLOWED_AUTH_NONE, MM_BEARER_IP_FAMILY_ANY, "");
}

void APNProfileModel::refresh(ModemManager::Bearer::List bearers)
{
    Q_EMIT beginRemoveRows(QModelIndex(), 0, count()-1);
    m_apnprofileList.clear();
    Q_EMIT endRemoveRows();
    
    Q_EMIT beginInsertRows(QModelIndex(), 0, bearers.count());
    for (auto bearer : bearers) {
        m_apnprofileList.append(new APNProfile(this, bearer));
    }
    Q_EMIT endInsertRows();
}

APNProfile::APNProfile(QObject* parent, QString apn, QString user, QString password, bool allowRoaming, MMBearerAllowedAuth authType, MMBearerIpFamily ipType, QString uni)
    : m_apn(user)
    , m_user(user)
    , m_password(password)
    , m_allowRoaming(allowRoaming)
    , m_authType(authTypeStr(authType))
    , m_ipType(ipTypeStr(ipType))
    , m_uni(uni)
{
    setParent(parent);
}

APNProfile::APNProfile(QObject* parent, ModemManager::Bearer::Ptr bearer)
{
    setParent(parent);
    m_uni = bearer->uni();
    QVariantMap props = bearer->properties();
    // TODO these are all guesses
    m_apn = props["apn"].toString();
    m_user = props["user"].toString();
    m_password = props["password"].toString();
    m_allowRoaming = props["allow-roaming"].toBool();
    m_authType = authTypeStr(MMBearerAllowedAuth(props["allowed-auth"].toInt()));
    m_ipType = ipTypeStr(MMBearerIpFamily(props["ip-type"].toInt()));
}

QString APNProfile::authTypeStr(MMBearerAllowedAuth authType)
{
    if (authType == MM_BEARER_ALLOWED_AUTH_UNKNOWN) {
        return "Unknown";
    } else if (authType == MM_BEARER_ALLOWED_AUTH_NONE) {
        return "None";
    } else if (authType == MM_BEARER_ALLOWED_AUTH_PAP) {
        return "PAP";
    } else if (authType == MM_BEARER_ALLOWED_AUTH_CHAP) {
        return "CHAP";
    } else if (authType == MM_BEARER_ALLOWED_AUTH_MSCHAP) {
        return "MSCHAP";
    } else if (authType == MM_BEARER_ALLOWED_AUTH_MSCHAPV2) {
        return "MSCHAPV2";
    } else if (authType == MM_BEARER_ALLOWED_AUTH_EAP) {
        return "EAP";
    }
    return "Unknown";
}

MMBearerAllowedAuth APNProfile::authTypeFlag()
{
    if (m_authType == "Unknown") {
        return MM_BEARER_ALLOWED_AUTH_UNKNOWN;
    } else if (m_authType == "None") {
        return MM_BEARER_ALLOWED_AUTH_NONE;
    } else if (m_authType == "PAP") {
        return MM_BEARER_ALLOWED_AUTH_PAP;
    } else if (m_authType == "CHAP") {
        return MM_BEARER_ALLOWED_AUTH_CHAP;
    } else if (m_authType == "MSCHAP") {
        return MM_BEARER_ALLOWED_AUTH_MSCHAP;
    } else if (m_authType == "MSCHAPV2") {
        return MM_BEARER_ALLOWED_AUTH_MSCHAPV2;
    } else if (m_authType == "EAP") {
        return MM_BEARER_ALLOWED_AUTH_EAP;
    }
    return MM_BEARER_ALLOWED_AUTH_UNKNOWN;
}

QString APNProfile::ipTypeStr(MMBearerIpFamily ipType)
{
    if (ipType == MM_BEARER_IP_FAMILY_NONE) {
        return "None";
    } else if (ipType == MM_BEARER_IP_FAMILY_IPV4) {
        return "IPv4";
    } else if (ipType == MM_BEARER_IP_FAMILY_IPV6) {
        return "IPv6";
    } else if (ipType == MM_BEARER_IP_FAMILY_IPV4V6) {
        return "IPv4/IPv6";
    } else if (ipType == MM_BEARER_IP_FAMILY_ANY) {
        return "Any";
    }
    return "None";
}

MMBearerIpFamily APNProfile::ipTypeFlag()
{
    if (m_ipType == "None") {
        return MM_BEARER_IP_FAMILY_NONE;
    } else if (m_ipType == "IPv4") {
        return MM_BEARER_IP_FAMILY_IPV4;
    } else if (m_ipType == "IPv6") {
        return MM_BEARER_IP_FAMILY_IPV6;
    } else if (m_ipType == "IPv4/IPv6") {
        return MM_BEARER_IP_FAMILY_IPV4V6;
    } else if (m_ipType == "Any") {
        return MM_BEARER_IP_FAMILY_ANY;
    }
    return MM_BEARER_IP_FAMILY_NONE;
}

ModemManager::BearerProperties APNProfile::toBearerProperties()
{
    ModemManager::BearerProperties properties;
    properties.setAllowedAuthentication(authTypeFlag());
    properties.setAllowRoaming(m_allowRoaming);
    properties.setApn(m_apn);
    properties.setIpType(ipTypeFlag());
//     properties.setNumber(""); // TODO idk what this is
//     properties.setRmProtocol(); // idk what this is either
    properties.setPassword(m_password);
    properties.setUser(m_user);
    return properties;
}

QString APNProfile::apn()
{
    return m_apn;
}

void APNProfile::setApn(QString apn)
{
    if (apn != m_apn) {
        m_apn = apn;
        Q_EMIT apnChanged();
    }
}

QString APNProfile::user()
{
    return m_user;
}

void APNProfile::setUser(QString user)
{
    if (user != m_user) {
        m_user = user;
        Q_EMIT userChanged();
    }
}

QString APNProfile::password()
{
    return m_password;
}

void APNProfile::setPassword(QString password)
{
    if (password != m_password) {
        m_password = password; 
        Q_EMIT passwordChanged();
    }
}

bool APNProfile::allowRoaming()
{
    return m_allowRoaming;
}

void APNProfile::setAllowRoaming(bool allowRoaming)
{
    if (allowRoaming != m_allowRoaming) {
        m_allowRoaming = allowRoaming;
        Q_EMIT allowRoamingChanged();
    }
}

QString APNProfile::authType() {
    return m_authType;
}

void APNProfile::setAuthType(QString authType) {
    if (authType != m_authType) {
        m_authType = authType;
        Q_EMIT authTypeChanged();
    }
}

QString APNProfile::ipType() {
    return m_ipType;
}

void APNProfile::setIpType(QString ipType) {
    if (ipType != m_ipType) {
        m_ipType = ipType;
        Q_EMIT ipTypeChanged();
    }
}

QString APNProfile::uni() {
    return m_uni;
}

