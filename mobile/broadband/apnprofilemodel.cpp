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
{}

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
    return new APNProfile(this, "", "", "", false, 0, 0, "");
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

APNProfile::APNProfile(QObject* parent, QString apn, QString user, QString password, bool allowRoaming, int authType, int ipType, QString uni)
    : m_apn(user)
    , m_user(user)
    , m_password(password)
    , m_allowRoaming(allowRoaming)
    , m_authType(authType)
    , m_ipType(ipType)
    , m_uni(uni)
{
}

APNProfile::APNProfile(QObject* parent, ModemManager::Bearer::Ptr bearer)
{
    m_uni = bearer->uni();
    QVariantMap props = bearer->properties();
    // TODO these are all guesses
    m_apn = props["apn"].toString();
    m_user = props["user"].toString();
    m_password = props["password"].toString();
    m_allowRoaming = props["allow-roaming"].toBool();
    m_authType = props["allowed-auth"].toUInt();
    m_ipType = props["ip-type"].toUInt();
}


QString APNProfile::apn()
{
    return m_apn;
}

void APNProfile::setApn(QString apn)
{
    if (apn != m_apn) {
        m_apn = apn;
        emit apnChanged();
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
        emit userChanged();
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
        emit passwordChanged();
    }
}

bool APNProfile::allowRoaming()
{
    return allowRoaming;
}

void APNProfile::setAllowRoaming(bool allowRoaming)
{
    if (allowRoaming != m_allowRoaming) {
        m_allowRoaming = allowRoaming;
        emit allowRoamingChanged();
    }
}

int APNProfile::authType() {
    return m_authType;
}

void APNProfile::setAuthType(int authType) {
    if (authType != m_authType) {
        m_authType = authType;
        emit authTypeChanged();
    }
}

int APNProfile::ipType() {
    return m_ipType;
}

void APNProfile::setIpType(int ipType) {
    if (ipType != m_ipType) {
        m_ipType = ipType;
        emit ipTypeChanged();
    }
}

QString APNProfile::uni() {
    return m_uni;
}

ModemManager::BearerProperties APNProfile::toBearerProperties()
{
    ModemManager::BearerProperties properties;
    properties.setAllowedAuthentication(m_authType);
    properties.setAllowRoaming(m_allowRoaming);
    properties.setApn(m_apn);
    properties.setIpType(m_ipType);
//     properties.setNumber(""); // TODO idk what this is
//     properties.setRmProtocol(); // idk what this is either
    properties.setPassword(m_password);
    properties.setUser(m_user);
    return properties;
}

