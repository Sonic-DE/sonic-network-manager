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

#ifndef APNPROFILEMODEL_H
#define APNPROFILEMODEL_H

#include <QObject>

#include <ModemManagerQt/Manager>
#include <ModemManagerQt/GenericTypes>
#include <ModemManagerQt/ModemDevice>

class APNProfile;
class APNProfileModel : public QAbstractListModel
{
    Q_OBJECT
    
public:
    explicit APNProfileModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    
    static APNProfileModel *instance()
    {
        static APNProfileModel *singleton = new APNProfileModel();
        return singleton;
    }
    
    Q_INVOKABLE APNProfile *defaultProfile();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE int count();
    Q_INVOKABLE APNProfile *get(int index);
    
    void refresh(ModemManager::Bearer::List bearers);

private:
    QList<APNProfile *> m_apnprofileList;
};

class APNProfile : public QObject 
{
    Q_OBJECT
    Q_PROPERTY(QString apn READ apn WRITE setApn NOTIFY apnChanged)
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool allowRoaming READ allowRoaming WRITE setAllowRoaming NOTIFY allowRoamingChanged)
    Q_PROPERTY(QString authType READ authType WRITE setAuthType NOTIFY authTypeChanged)
    Q_PROPERTY(QString ipType READ ipType WRITE setIpType NOTIFY ipTypeChanged)

    // Flags:
    // https://www.freedesktop.org/software/ModemManager/api/latest/ModemManager-Flags-and-Enumerations.html
    
    // authType: MMBearerAllowedAuth
    // MM_BEARER_ALLOWED_AUTH_UNKNOWN  = "Unknown"
    // MM_BEARER_ALLOWED_AUTH_NONE     = "None"
    // MM_BEARER_ALLOWED_AUTH_PAP      = "PAP"
    // MM_BEARER_ALLOWED_AUTH_CHAP     = "CHAP
    // MM_BEARER_ALLOWED_AUTH_MSCHAP   = "MSCHAP"
    // MM_BEARER_ALLOWED_AUTH_MSCHAPV2 = "MSCHAPV2"
    // MM_BEARER_ALLOWED_AUTH_EAP      = "EAP"
    
    // ipType: MMBearerIpFamily
    // MM_BEARER_IP_FAMILY_NONE - "None"
    // MM_BEARER_IP_FAMILY_IPV4 - "IPv4"
    // MM_BEARER_IP_FAMILY_IPV6 - "IPv6"
    // MM_BEARER_IP_FAMILY_IPV4V6 - "IPv4/IPv6"
    // MM_BEARER_IP_FAMILY_ANY - "Any"
    
public:
    APNProfile(QObject* parent = nullptr) {}
    APNProfile(QObject* parent, QString apn, QString user, QString password, bool allowRoaming, MMBearerAllowedAuth authType, MMBearerIpFamily ipType, QString uni = "");
    APNProfile(QObject* parent, ModemManager::Bearer::Ptr bearer);
    
    QString authTypeStr(MMBearerAllowedAuth authType);
    MMBearerAllowedAuth authTypeFlag();
    QString ipTypeStr(MMBearerIpFamily ipType);
    MMBearerIpFamily ipTypeFlag();
    
    QString apn();
    void setApn(QString apn);
    QString user();
    void setUser(QString user);
    QString password();
    void setPassword(QString password);
    bool allowRoaming();
    void setAllowRoaming(bool allowRoaming);
    QString authType();
    void setAuthType(QString authType);
    QString ipType();
    void setIpType(QString ipType);
    
    QString uni(); // if this profile was created off of an existing bearer
    
    ModemManager::BearerProperties toBearerProperties();
    
Q_SIGNALS:
    void apnChanged();
    void userChanged();
    void passwordChanged();
    void allowRoamingChanged();
    void authTypeChanged();
    void ipTypeChanged();

private:
    QString m_apn, m_user, m_password, m_authType, m_ipType, m_uni;
    bool m_allowRoaming;
};

#endif // APNPROFILEMODEL_H
