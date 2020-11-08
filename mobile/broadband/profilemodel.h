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

#ifndef PROFILEMODEL_H
#define PROFILEMODEL_H

#include <QObject>

#include <ModemManagerQt/Manager>
#include <ModemManagerQt/GenericTypes>
#include <ModemManagerQt/ModemDevice>

#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/GsmSetting>
#include <NetworkManagerQt/CdmaSetting>
#include <NetworkManagerQt/ModemDevice>
#include <NetworkManagerQt/Manager>

class ProfileSettings;
class ProfileModel : public QAbstractListModel
{
    Q_OBJECT
    
public:
    explicit ProfileModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    
    static ProfileModel *instance()
    {
        static ProfileModel *singleton = new ProfileModel();
        return singleton;
    }
    
//     Q_INVOKABLE ProfileSettings *defaultProfile();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE int count();
    Q_INVOKABLE ProfileSettings *get(int index);
    
    
    void refresh(NetworkManager::Connection::List connections);

    QString networkTypeStr(NetworkManager::GsmSetting::NetworkType networkType);
    NetworkManager::GsmSetting::NetworkType networkTypeFlag(const QString &networkType);
    
Q_SIGNALS:
    
private:
    QList<ProfileSettings *> m_profileList;
};

class ProfileSettings : public QObject 
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString apn READ apn WRITE setApn NOTIFY apnChanged)
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString networkType READ networkType WRITE setNetworkType NOTIFY networkTypeChanged)
    Q_PROPERTY(bool allowRoaming READ allowRoaming WRITE setAllowRoaming NOTIFY allowRoamingChanged)
    Q_PROPERTY(QString connectionUni READ connectionUni NOTIFY connectionUniChanged)
    
public:
    ProfileSettings(QObject* parent = nullptr) {}
    ProfileSettings(QObject* parent, QString name, QString apn, QString user, QString password, NetworkManager::GsmSetting::NetworkType networkType, bool allowRoaming, QString connectionUni);
    ProfileSettings(QObject* parent, NetworkManager::Setting::Ptr settings, NetworkManager::Connection::Ptr connection);
    
    QString name();
    QString apn();
    void setApn(QString apn);
    QString user();
    void setUser(QString user);
    QString password();
    void setPassword(QString password);
    QString networkType();
    void setNetworkType(QString ipType);
    bool allowRoaming();
    void setAllowRoaming(bool allowRoaming);
    QString connectionUni();
    
    QVariantMap toSettings();
    
Q_SIGNALS:
    void nameChanged();
    void apnChanged();
    void userChanged();
    void passwordChanged();
    void networkTypeChanged();
    void allowRoamingChanged();
    void connectionUniChanged();

private:
    QString m_name, m_apn, m_user, m_password, m_networkType, m_connectionUni;
    bool m_gsm = false, m_allowRoaming;
};

#endif // PROFILEMODEL_H
