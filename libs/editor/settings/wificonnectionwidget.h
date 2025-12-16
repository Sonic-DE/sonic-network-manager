/*
    SPDX-FileCopyrightText: 2013 Lukas Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef PLASMA_NM_WIFI_CONNECTION_WIDGET_H
#define PLASMA_NM_WIFI_CONNECTION_WIDGET_H

#include "plasmanm_editor_export.h"

#include <QWidget>

#include <NetworkManagerQt/Security8021xSetting>
#include <NetworkManagerQt/WirelessSecuritySetting>
#include <NetworkManagerQt/WirelessSetting>

#include <QRegularExpressionValidator>

#include "settingwidget.h"

namespace Ui
{
class WifiConnectionWidget;
}

class PLASMANM_EDITOR_EXPORT WifiConnectionWidget : public SettingWidget
{
    Q_OBJECT

public:
    enum SecurityTypeIndex {
        None = 0,
        WepHex,
        WepPassphrase,
        Leap,
        DynamicWep,
        WpaPsk,
        WpaEap,
        SAE,
        Wpa3SuiteB192,
        OWE
    };

    explicit WifiConnectionWidget(const NetworkManager::Setting::Ptr &setting = NetworkManager::Setting::Ptr(),
                                  const NetworkManager::Setting::Ptr &securitySetting = NetworkManager::Setting::Ptr(),
                                  const NetworkManager::Security8021xSetting::Ptr &setting8021x = NetworkManager::Security8021xSetting::Ptr(),
                                  QWidget *parent = nullptr,
                                  Qt::WindowFlags f = {});
    ~WifiConnectionWidget() override;

    void loadConfig(const NetworkManager::Setting::Ptr &setting) override;
    void loadSecrets(const NetworkManager::Setting::Ptr &setting) override;

    QVariantMap setting() const override;
    QVariantMap securitySetting() const;
    QVariantMap setting8021x() const;

    bool securityEnabled() const;
    bool enabled8021x() const;

    bool isValid() const override;

    void setStoreSecretsSystemWide(bool system);

Q_SIGNALS:
    void ssidChanged(const QString &ssid);

public Q_SLOTS:
    void onSsidChanged(const QString &ssid);

private Q_SLOTS:
    void generateRandomClonedMac();
    void ssidChanged();
    void modeChanged(int mode);
    void bandChanged(int band);
    void securityChanged(int index);
    void setWepKey(int keyIndex);
    void auth8021xChanged(int index);
    void altSubjectMatches8021xButtonClicked();
    void connectToServers8021xButtonClicked();

private:
    Ui::WifiConnectionWidget *const m_ui;
    NetworkManager::WirelessSecuritySetting::Ptr m_wifiSecurity;
    QRegularExpressionValidator *altSubjectValidator8021x = nullptr;
    QRegularExpressionValidator *serversValidator8021x = nullptr;
    bool m_systemWideDefault = false;

    void fillChannels(NetworkManager::WirelessSetting::FrequencyBand band);
    void loadSecurityConfig(const NetworkManager::Setting::Ptr &setting);
    void loadSecuritySecrets(const NetworkManager::Setting::Ptr &setting);
    void load8021xConfig(const NetworkManager::Security8021xSetting::Ptr &setting);
    bool isValid8021x() const;
};

#endif // PLASMA_NM_WIFI_CONNECTION_WIDGET_H
