/*
    SPDX-FileCopyrightText: 2013 Lukas Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "wificonnectionwidget.h"
#include "editlistdialog.h"
#include "listvalidator.h"
#include "plasma_nm_editor.h"
#include "ui_wificonnectionwidget.h"

#include <KAcceleratorManager>
#include <KLocalizedString>
#include <NetworkManagerQt/Device>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Utils>
#include <NetworkManagerQt/WirelessDevice>
#include <QRandomGenerator>
#include <QtCrypto>

WifiConnectionWidget::WifiConnectionWidget(const NetworkManager::Setting::Ptr &setting,
                                          const NetworkManager::Setting::Ptr &securitySetting,
                                          const NetworkManager::Security8021xSetting::Ptr &setting8021x,
                                          QWidget *parent,
                                          Qt::WindowFlags f)
    : SettingWidget(setting, parent, f)
    , m_ui(new Ui::WifiConnectionWidget)
    , m_wifiSecurity(securitySetting.staticCast<NetworkManager::WirelessSecuritySetting>())
{
    m_ui->setupUi(this);

    m_ui->leapPassword->setPasswordOptionsEnabled(true);
    m_ui->psk->setPasswordOptionsEnabled(true);
    m_ui->wepKey->setPasswordOptionsEnabled(true);
    m_ui->password8021x->setPasswordOptionsEnabled(true);
    m_ui->privateKeyPassword8021x->setPasswordOptionsEnabled(true);

    // Setup 802.1x validators
    altSubjectValidator8021x = new QRegularExpressionValidator(
        QRegularExpression(
            QLatin1String("^(DNS:[a-zA-Z0-9_-]+\\.[a-zA-Z0-9_.-]+|EMAIL:[a-zA-Z0-9._-]+@[a-zA-Z0-9_-]+\\.[a-zA-Z0-9_.-]+|URI:[a-zA-Z0-9.+-]+:.+|)$")),
        this);
    serversValidator8021x = new QRegularExpressionValidator(QRegularExpression(QLatin1String("^[a-zA-Z0-9_-]+\\.[a-zA-Z0-9_.-]+$")), this);

    auto altSubjectListValidator = new ListValidator(this);
    altSubjectListValidator->setInnerValidator(altSubjectValidator8021x);
    m_ui->subjectMatch8021x->setValidator(altSubjectListValidator);

    auto serverListValidator = new ListValidator(this);
    serverListValidator->setInnerValidator(serversValidator8021x);
    m_ui->connectToServers8021x->setValidator(serverListValidator);

    // WPA3 Enterprise is available in NM 1.30+
    if (!NetworkManager::checkVersion(1, 30, 0)) {
        m_ui->securityCombo->removeItem(8);
    }

    // WPA3 Personal is available in NM 1.16+
    if (!NetworkManager::checkVersion(1, 16, 0)) {
        m_ui->securityCombo->removeItem(7);
    }

    // Wi-Fi connections
    connect(m_ui->btnRandomMacAddr, &QPushButton::clicked, this, &WifiConnectionWidget::generateRandomClonedMac);
    connect(m_ui->SSIDCombo, &SsidComboBox::ssidChanged, this, QOverload<>::of(&WifiConnectionWidget::ssidChanged));
    connect(m_ui->modeComboBox, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::modeChanged);
    connect(m_ui->band, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::bandChanged);

    // Security connections
    connect(m_ui->securityCombo, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::securityChanged);
    connect(m_ui->wepIndex, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::setWepKey);

    // 802.1x button connections
    connect(m_ui->btnAltSubjectMatches8021x, &QPushButton::clicked, this, &WifiConnectionWidget::altSubjectMatches8021xButtonClicked);
    connect(m_ui->btnConnectToServers8021x, &QPushButton::clicked, this, &WifiConnectionWidget::connectToServers8021xButtonClicked);

    // 802.1x auth method change connection
    connect(m_ui->auth8021x, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::auth8021xChanged);

    // Connect for setting check
    watchChangedSetting();

    // Connect for validity check
    connect(m_ui->macAddress, &HwAddrComboBox::hwAddressChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->BSSIDCombo, &BssidComboBox::bssidChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->wepKey, &PasswordField::textChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->wepKey, &PasswordField::passwordOptionChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->leapUsername, &KLineEdit::textChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->leapPassword, &PasswordField::textChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->leapPassword, &PasswordField::passwordOptionChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->psk, &PasswordField::textChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->psk, &PasswordField::passwordOptionChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->wepIndex, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->securityCombo, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::slotWidgetChanged);

    // 802.1x signal connections - TODO: Add remaining connections
    connect(m_ui->auth8021x, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->username8021x, &KLineEdit::textChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->password8021x, &PasswordField::textChanged, this, &WifiConnectionWidget::slotWidgetChanged);
    connect(m_ui->password8021x, &PasswordField::passwordOptionChanged, this, &WifiConnectionWidget::slotWidgetChanged);

    KAcceleratorManager::manage(this);

    // Initialize security widget visibility
    securityChanged(m_ui->securityCombo->currentIndex());

    if (setting) {
        loadConfig(setting);
    }
    if (securitySetting && !securitySetting->isNull()) {
        loadSecurityConfig(securitySetting);
    }
    if (setting8021x && !setting8021x->isNull()) {
        load8021xConfig(setting8021x);
    }
}

WifiConnectionWidget::~WifiConnectionWidget()
{
    delete m_ui;
}

void WifiConnectionWidget::loadConfig(const NetworkManager::Setting::Ptr &setting)
{
    NetworkManager::WirelessSetting::Ptr wifiSetting = setting.staticCast<NetworkManager::WirelessSetting>();

    m_ui->SSIDCombo->init(QString::fromUtf8(wifiSetting->ssid()));

    if (wifiSetting->mode() != NetworkManager::WirelessSetting::Infrastructure) {
        m_ui->modeComboBox->setCurrentIndex(wifiSetting->mode());
    }
    modeChanged(wifiSetting->mode());

    m_ui->BSSIDCombo->init(NetworkManager::macAddressAsString(wifiSetting->bssid()), QString::fromUtf8(wifiSetting->ssid()));

    m_ui->band->setCurrentIndex(wifiSetting->band());
    if (wifiSetting->band() != NetworkManager::WirelessSetting::Automatic) {
        m_ui->channel->setCurrentIndex(m_ui->channel->findData(wifiSetting->channel()));
    }

    m_ui->macAddress->init(NetworkManager::Device::Wifi, NetworkManager::macAddressAsString(wifiSetting->macAddress()));

    if (!wifiSetting->clonedMacAddress().isEmpty()) {
        m_ui->clonedMacAddress->setText(NetworkManager::macAddressAsString(wifiSetting->clonedMacAddress()));
    }

    if (wifiSetting->mtu()) {
        m_ui->mtu->setValue(wifiSetting->mtu());
    }

    if (wifiSetting->hidden()) {
        m_ui->hiddenNetwork->setChecked(true);
    }
}

QVariantMap WifiConnectionWidget::setting() const
{
    NetworkManager::WirelessSetting wifiSetting;

    wifiSetting.setSsid(m_ui->SSIDCombo->ssid().toUtf8());

    wifiSetting.setMode(static_cast<NetworkManager::WirelessSetting::NetworkMode>(m_ui->modeComboBox->currentIndex()));

    wifiSetting.setBssid(NetworkManager::macAddressFromString(m_ui->BSSIDCombo->bssid()));

    if (wifiSetting.mode() != NetworkManager::WirelessSetting::Infrastructure && m_ui->band->currentIndex() != 0) {
        wifiSetting.setBand((NetworkManager::WirelessSetting::FrequencyBand)m_ui->band->currentIndex());
        wifiSetting.setChannel(m_ui->channel->itemData(m_ui->channel->currentIndex()).toUInt());
    }

    wifiSetting.setMacAddress(NetworkManager::macAddressFromString(m_ui->macAddress->hwAddress()));

    if (!m_ui->clonedMacAddress->text().isEmpty() && m_ui->clonedMacAddress->text() != ":::::") {
        wifiSetting.setClonedMacAddress(NetworkManager::macAddressFromString(m_ui->clonedMacAddress->text()));
    }

    if (m_ui->mtu->value()) {
        wifiSetting.setMtu(m_ui->mtu->value());
    }

    wifiSetting.setHidden(m_ui->hiddenNetwork->isChecked());

    return wifiSetting.toMap();
}

void WifiConnectionWidget::generateRandomClonedMac()
{
    QByteArray mac;
    auto generator = QRandomGenerator::global();
    mac.resize(6);
    for (int i = 0; i < 6; i++) {
        const int random = generator->bounded(255);
        mac[i] = random;
    }

    // Disable the multicast bit and enable the locally administered bit.
    mac[0] = mac[0] & ~0x1;
    mac[0] = mac[0] | 0x2;

    m_ui->clonedMacAddress->setText(NetworkManager::macAddressAsString(mac));
}

void WifiConnectionWidget::ssidChanged()
{
    m_ui->BSSIDCombo->init(m_ui->BSSIDCombo->bssid(), m_ui->SSIDCombo->ssid());
    slotWidgetChanged();

    // Emit that SSID has changed so we can pre-configure wireless security
    const QString ssid = m_ui->SSIDCombo->ssid();
    Q_EMIT ssidChanged(ssid);
    // Auto-detect security type from SSID
    onSsidChanged(ssid);
}

void WifiConnectionWidget::modeChanged(int mode)
{
    if (mode == NetworkManager::WirelessSetting::Infrastructure) {
        m_ui->BSSIDLabel->setVisible(true);
        m_ui->BSSIDCombo->setVisible(true);
        m_ui->bandLabel->setVisible(false);
        m_ui->band->setVisible(false);
        m_ui->channelLabel->setVisible(false);
        m_ui->channel->setVisible(false);
    } else {
        m_ui->BSSIDLabel->setVisible(false);
        m_ui->BSSIDCombo->setVisible(false);
        m_ui->bandLabel->setVisible(true);
        m_ui->band->setVisible(true);
        m_ui->channelLabel->setVisible(true);
        m_ui->channel->setVisible(true);
    }
}

void WifiConnectionWidget::bandChanged(int band)
{
    m_ui->channel->clear();

    if (band == NetworkManager::WirelessSetting::Automatic) {
        m_ui->channel->setEnabled(false);
    } else {
        fillChannels((NetworkManager::WirelessSetting::FrequencyBand)band);
        m_ui->channel->setEnabled(true);
    }
}

void WifiConnectionWidget::fillChannels(NetworkManager::WirelessSetting::FrequencyBand band)
{
    QList<QPair<int, int>> channels;

    if (band == NetworkManager::WirelessSetting::A) {
        channels = NetworkManager::getAFreqs();
    } else if (band == NetworkManager::WirelessSetting::Bg) {
        channels = NetworkManager::getBFreqs();
    } else {
        qCWarning(PLASMA_NM_EDITOR_LOG) << Q_FUNC_INFO << "Unhandled band number" << band;
        return;
    }

    QListIterator<QPair<int, int>> i(channels);
    while (i.hasNext()) {
        QPair<int, int> channel = i.next();
        m_ui->channel->addItem(i18n("%1 (%2 MHz)", channel.first, channel.second), channel.first);
    }
}

bool WifiConnectionWidget::isValid() const
{
    if (m_ui->SSIDCombo->currentText().isEmpty() || !m_ui->macAddress->isValid() || !m_ui->BSSIDCombo->isValid()) {
        return false;
    }

    // Check security validity
    const int securityIndex = m_ui->securityCombo->currentIndex();
    if (securityIndex == WepHex) {
        return NetworkManager::wepKeyIsValid(m_ui->wepKey->text(), NetworkManager::WirelessSecuritySetting::Hex)
            || m_ui->wepKey->passwordOption() == PasswordField::AlwaysAsk;
    } else if (securityIndex == WepPassphrase) {
        return NetworkManager::wepKeyIsValid(m_ui->wepKey->text(), NetworkManager::WirelessSecuritySetting::Passphrase)
            || m_ui->wepKey->passwordOption() == PasswordField::AlwaysAsk;
    } else if (securityIndex == Leap) {
        return !m_ui->leapUsername->text().isEmpty()
            && (!m_ui->leapPassword->text().isEmpty() || m_ui->leapPassword->passwordOption() == PasswordField::AlwaysAsk);
    } else if (securityIndex == WpaPsk) {
        return NetworkManager::wpaPskIsValid(m_ui->psk->text()) || m_ui->psk->passwordOption() == PasswordField::AlwaysAsk;
    } else if (securityIndex == DynamicWep || securityIndex == WpaEap || securityIndex == Wpa3SuiteB192) {
        return isValid8021x();
    } else if (securityIndex == SAE) {
        return !m_ui->psk->text().isEmpty() || m_ui->psk->passwordOption() == PasswordField::AlwaysAsk;
    }

    return true;
}

bool WifiConnectionWidget::securityEnabled() const
{
    return m_ui->securityCombo->currentIndex() > 0;
}

bool WifiConnectionWidget::enabled8021x() const
{
    const int index = m_ui->securityCombo->currentIndex();
    return index == DynamicWep || index == WpaEap || index == Wpa3SuiteB192;
}

void WifiConnectionWidget::loadSecrets(const NetworkManager::Setting::Ptr &setting)
{
    if (setting->type() == NetworkManager::Setting::WirelessSecurity) {
        loadSecuritySecrets(setting);
    } else if (setting->type() == NetworkManager::Setting::Security8021x) {
        NetworkManager::Security8021xSetting::Ptr security8021xSetting = setting.staticCast<NetworkManager::Security8021xSetting>();
        if (security8021xSetting) {
            const QString password = security8021xSetting->password();
            const QList<NetworkManager::Security8021xSetting::EapMethod> eapMethods = security8021xSetting->eapMethods();

            // Load password for methods that use it
            if (!password.isEmpty() && !eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodTls)) {
                m_ui->password8021x->setText(password);
            }

            // Load private key password for TLS
            if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodTls)) {
                const QString privateKeyPassword = security8021xSetting->privateKeyPassword();
                if (!privateKeyPassword.isEmpty()) {
                    m_ui->privateKeyPassword8021x->setText(privateKeyPassword);
                }
            }
        }
    }
}

void WifiConnectionWidget::loadSecurityConfig(const NetworkManager::Setting::Ptr &setting)
{
    NetworkManager::WirelessSecuritySetting::Ptr wifiSecurity = setting.staticCast<NetworkManager::WirelessSecuritySetting>();

    const NetworkManager::WirelessSecuritySetting::KeyMgmt keyMgmt = wifiSecurity->keyMgmt();
    const NetworkManager::WirelessSecuritySetting::AuthAlg authAlg = wifiSecurity->authAlg();

    if (keyMgmt == NetworkManager::WirelessSecuritySetting::Unknown) {
        m_ui->securityCombo->setCurrentIndex(None); // None
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::Wep) {
        if (wifiSecurity->wepKeyType() == NetworkManager::WirelessSecuritySetting::Hex
            || wifiSecurity->wepKeyType() == NetworkManager::WirelessSecuritySetting::NotSpecified) {
            m_ui->securityCombo->setCurrentIndex(WepHex); // WEP Hex
        } else {
            m_ui->securityCombo->setCurrentIndex(WepPassphrase);
        }
        const int keyIndex = static_cast<int>(wifiSecurity->wepTxKeyindex());
        m_ui->wepIndex->setCurrentIndex(keyIndex);

        if (wifiSecurity->authAlg() == NetworkManager::WirelessSecuritySetting::Open) {
            m_ui->wepAuth->setCurrentIndex(0);
        } else {
            m_ui->wepAuth->setCurrentIndex(1);
        }

        if (wifiSecurity->wepKeyFlags().testFlag(NetworkManager::Setting::None)) {
            m_ui->wepKey->setPasswordOption(PasswordField::StoreForAllUsers);
        } else if (wifiSecurity->wepKeyFlags().testFlag(NetworkManager::Setting::AgentOwned)) {
            m_ui->wepKey->setPasswordOption(PasswordField::StoreForUser);
        } else {
            m_ui->wepKey->setPasswordOption(PasswordField::AlwaysAsk);
        }
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::Ieee8021x && authAlg == NetworkManager::WirelessSecuritySetting::Leap) {
        m_ui->securityCombo->setCurrentIndex(Leap); // LEAP
        m_ui->leapUsername->setText(wifiSecurity->leapUsername());
        m_ui->leapPassword->setText(wifiSecurity->leapPassword());

        if (wifiSecurity->leapPasswordFlags().testFlag(NetworkManager::Setting::None)) {
            m_ui->leapPassword->setPasswordOption(PasswordField::StoreForAllUsers);
        } else if (wifiSecurity->leapPasswordFlags().testFlag(NetworkManager::Setting::AgentOwned)) {
            m_ui->leapPassword->setPasswordOption(PasswordField::StoreForUser);
        } else {
            m_ui->leapPassword->setPasswordOption(PasswordField::AlwaysAsk);
        }
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::Ieee8021x) {
        m_ui->securityCombo->setCurrentIndex(DynamicWep); // Dynamic WEP
        // done in the widget

    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::WpaPsk) {
        m_ui->securityCombo->setCurrentIndex(WpaPsk); // WPA

        if (wifiSecurity->pskFlags().testFlag(NetworkManager::Setting::None)) {
            m_ui->psk->setPasswordOption(PasswordField::StoreForAllUsers);
        } else if (wifiSecurity->pskFlags().testFlag(NetworkManager::Setting::AgentOwned)) {
            m_ui->psk->setPasswordOption(PasswordField::StoreForUser);
        } else {
            m_ui->psk->setPasswordOption(PasswordField::AlwaysAsk);
        }
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::WpaEap) {
        m_ui->securityCombo->setCurrentIndex(WpaEap); // WPA2 Enterprise
        // done in the widget
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::SAE) {
        m_ui->securityCombo->setCurrentIndex(SAE); // WPA3

        if (wifiSecurity->pskFlags().testFlag(NetworkManager::Setting::None)) {
            m_ui->psk->setPasswordOption(PasswordField::StoreForAllUsers);
        } else if (wifiSecurity->pskFlags().testFlag(NetworkManager::Setting::AgentOwned)) {
            m_ui->psk->setPasswordOption(PasswordField::StoreForUser);
        } else {
            m_ui->psk->setPasswordOption(PasswordField::AlwaysAsk);
        }
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::WpaEapSuiteB192) {
        m_ui->securityCombo->setCurrentIndex(Wpa3SuiteB192); // WPA3 Enterprise Suite B 192
        // done in the widget
    } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::OWE) {
        m_ui->securityCombo->setCurrentIndex(OWE); // Enhanced Open (OWE)
    }

    if (keyMgmt != NetworkManager::WirelessSecuritySetting::Ieee8021x && keyMgmt != NetworkManager::WirelessSecuritySetting::WpaEap
        && keyMgmt != NetworkManager::WirelessSecuritySetting::WpaEapSuiteB192) {
        loadSecuritySecrets(setting);
    }
}

void WifiConnectionWidget::loadSecuritySecrets(const NetworkManager::Setting::Ptr &setting)
{
    const NetworkManager::WirelessSecuritySetting::KeyMgmt keyMgmt = m_wifiSecurity->keyMgmt();
    const NetworkManager::WirelessSecuritySetting::AuthAlg authAlg = m_wifiSecurity->authAlg();

    NetworkManager::WirelessSecuritySetting::Ptr wifiSecurity = setting.staticCast<NetworkManager::WirelessSecuritySetting>();
    if (wifiSecurity) {
        if (keyMgmt == NetworkManager::WirelessSecuritySetting::Wep) {
            m_wifiSecurity->secretsFromMap(wifiSecurity->secretsToMap());
            const int keyIndex = static_cast<int>(m_wifiSecurity->wepTxKeyindex());
            setWepKey(keyIndex);
        } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::Ieee8021x && authAlg == NetworkManager::WirelessSecuritySetting::Leap) {
            const QString leapPassword = wifiSecurity->leapPassword();
            if (!leapPassword.isEmpty()) {
                m_ui->leapPassword->setText(leapPassword);
            }
        } else if (keyMgmt == NetworkManager::WirelessSecuritySetting::WpaPsk || keyMgmt == NetworkManager::WirelessSecuritySetting::SAE) {
            const QString psk = wifiSecurity->psk();
            if (!psk.isEmpty()) {
                m_ui->psk->setText(psk);
            }
        }
    }
}

QVariantMap WifiConnectionWidget::securitySetting() const
{
    NetworkManager::WirelessSecuritySetting wifiSecurity;

    const int securityIndex = m_ui->securityCombo->currentIndex();
    if (securityIndex == None) {
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::Unknown);
    } else if (securityIndex == WepHex || securityIndex == WepPassphrase) { // WEP
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::Wep);
        if (securityIndex == WepHex) {
            wifiSecurity.setWepKeyType(NetworkManager::WirelessSecuritySetting::Hex);
        } else {
            wifiSecurity.setWepKeyType(NetworkManager::WirelessSecuritySetting::Passphrase);
        }
        const int keyIndex = m_ui->wepIndex->currentIndex();
        const QString wepKey = m_ui->wepKey->text();
        wifiSecurity.setWepTxKeyindex(keyIndex);
        if (keyIndex == 0) {
            wifiSecurity.setWepKey0(wepKey);
        } else if (keyIndex == 1) {
            wifiSecurity.setWepKey1(wepKey);
        } else if (keyIndex == 2) {
            wifiSecurity.setWepKey2(wepKey);
        } else if (keyIndex == 3) {
            wifiSecurity.setWepKey3(wepKey);
        }

        if (m_ui->wepKey->passwordOption() == PasswordField::StoreForAllUsers) {
            wifiSecurity.setWepKeyFlags(NetworkManager::Setting::None);
        } else if (m_ui->wepKey->passwordOption() == PasswordField::StoreForUser) {
            wifiSecurity.setWepKeyFlags(NetworkManager::Setting::AgentOwned);
        } else {
            wifiSecurity.setWepKeyFlags(NetworkManager::Setting::NotSaved);
        }

        if (m_ui->wepAuth->currentIndex() == 0) {
            wifiSecurity.setAuthAlg(NetworkManager::WirelessSecuritySetting::Open);
        } else {
            wifiSecurity.setAuthAlg(NetworkManager::WirelessSecuritySetting::Shared);
        }
    } else if (securityIndex == Leap) { // LEAP
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::Ieee8021x);
        wifiSecurity.setAuthAlg(NetworkManager::WirelessSecuritySetting::Leap);
        wifiSecurity.setLeapUsername(m_ui->leapUsername->text());
        wifiSecurity.setLeapPassword(m_ui->leapPassword->text());

        if (m_ui->leapPassword->passwordOption() == PasswordField::StoreForAllUsers) {
            wifiSecurity.setLeapPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->leapPassword->passwordOption() == PasswordField::StoreForUser) {
            wifiSecurity.setLeapPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            wifiSecurity.setLeapPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (securityIndex == DynamicWep) { // Dynamic WEP
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::Ieee8021x);
    } else if (securityIndex == WpaPsk) { // WPA
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::WpaPsk);
        wifiSecurity.setPsk(m_ui->psk->text());

        if (m_ui->psk->passwordOption() == PasswordField::StoreForAllUsers) {
            wifiSecurity.setPskFlags(NetworkManager::Setting::None);
        } else if (m_ui->psk->passwordOption() == PasswordField::StoreForUser) {
            wifiSecurity.setPskFlags(NetworkManager::Setting::AgentOwned);
        } else {
            wifiSecurity.setPskFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (securityIndex == WpaEap) { // WPA2 Enterprise
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::WpaEap);
    } else if (securityIndex == SAE) { // WPA3 Personal
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::SAE);
        wifiSecurity.setPsk(m_ui->psk->text());

        if (m_ui->psk->passwordOption() == PasswordField::StoreForAllUsers) {
            wifiSecurity.setPskFlags(NetworkManager::Setting::None);
        } else if (m_ui->psk->passwordOption() == PasswordField::StoreForUser) {
            wifiSecurity.setPskFlags(NetworkManager::Setting::AgentOwned);
        } else {
            wifiSecurity.setPskFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (securityIndex == Wpa3SuiteB192) { // WPA3 Enterprise Suite B 192
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::WpaEapSuiteB192);
        wifiSecurity.setPmf(NetworkManager::WirelessSecuritySetting::RequiredPmf);
    } else if (securityIndex == OWE) { // Enhanced Open (OWE)
        wifiSecurity.setKeyMgmt(NetworkManager::WirelessSecuritySetting::OWE);
    }

    return wifiSecurity.toMap();
}

QVariantMap WifiConnectionWidget::setting8021x() const
{
    const int securityIndex = m_ui->securityCombo->currentIndex();
    if (securityIndex != DynamicWep && securityIndex != WpaEap && securityIndex != Wpa3SuiteB192) {
        return {};
    }

    NetworkManager::Security8021xSetting setting;

    NetworkManager::Security8021xSetting::EapMethod method =
        static_cast<NetworkManager::Security8021xSetting::EapMethod>(m_ui->auth8021x->itemData(m_ui->auth8021x->currentIndex()).toInt());

    setting.setEapMethods(QList<NetworkManager::Security8021xSetting::EapMethod>() << method);

    // Common fields
    if (!m_ui->username8021x->text().isEmpty()) {
        setting.setIdentity(m_ui->username8021x->text());
    }

    // Method-specific configuration
    if (method == NetworkManager::Security8021xSetting::EapMethodMd5) {
        if (!m_ui->password8021x->text().isEmpty()) {
            setting.setPassword(m_ui->password8021x->text());
        }
        if (m_ui->password8021x->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password8021x->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        if (!m_ui->domain8021x->text().isEmpty()) {
            setting.setDomainSuffixMatch(m_ui->domain8021x->text());
        }

        if (m_ui->userCert8021x->url().isValid()) {
            const auto formattingOption = m_ui->userCert8021x->url().scheme() == "file" ? QUrl::PrettyDecoded : QUrl::FullyEncoded;
            setting.setClientCertificate(m_ui->userCert8021x->url().toString(formattingOption).toUtf8().append('\0'));
        }

        if (m_ui->caCert8021x->url().isValid()) {
            const auto formattingOption = m_ui->caCert8021x->url().scheme() == "file" ? QUrl::PrettyDecoded : QUrl::FullyEncoded;
            setting.setCaCertificate(m_ui->caCert8021x->url().toString(formattingOption).toUtf8().append('\0'));
        }

        QStringList altsubjectmatches = m_ui->altSubjectMatches8021x->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString &match : m_ui->connectToServers8021x->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QString tempstr = QLatin1String("DNS:") + match;
            if (!altsubjectmatches.contains(tempstr)) {
                altsubjectmatches.append(tempstr);
            }
        }
        setting.setSubjectMatch(m_ui->subjectMatch8021x->text());
        setting.setAltSubjectMatches(altsubjectmatches);

        if (m_ui->privateKey8021x->url().isValid()) {
            const auto formattingOption = m_ui->privateKey8021x->url().scheme() == "file" ? QUrl::PrettyDecoded : QUrl::FullyEncoded;
            setting.setPrivateKey(m_ui->privateKey8021x->url().toString(formattingOption).toUtf8().append('\0'));
        }

        if (!m_ui->privateKeyPassword8021x->text().isEmpty()) {
            setting.setPrivateKeyPassword(m_ui->privateKeyPassword8021x->text());
        }

        QCA::Initializer init;
        QCA::ConvertResult convRes;

        // Try if the private key is in pkcs12 format bundled with client certificate
        if (QCA::isSupported("pkcs12")) {
            QCA::KeyBundle keyBundle = QCA::KeyBundle::fromFile(m_ui->privateKey8021x->url().path(), m_ui->privateKeyPassword8021x->text().toUtf8(), &convRes);
            // Set client certificate to the same path as private key
            if (convRes == QCA::ConvertGood && keyBundle.privateKey().canDecrypt()) {
                setting.setClientCertificate(m_ui->privateKey8021x->url().toString().toUtf8().append('\0'));
            }
        }

        if (m_ui->privateKeyPassword8021x->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPrivateKeyPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->privateKeyPassword8021x->passwordOption() == PasswordField::StoreForUser) {
            setting.setPrivateKeyPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPrivateKeyPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodLeap || method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        if (!m_ui->password8021x->text().isEmpty()) {
            setting.setPassword(m_ui->password8021x->text());
        }
        if (m_ui->password8021x->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password8021x->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        if (!m_ui->anonymousIdentity8021x->text().isEmpty()) {
            setting.setAnonymousIdentity(m_ui->anonymousIdentity8021x->text());
        }

        if (!m_ui->pacProvisioning8021x->isChecked()) {
            setting.setPhase1FastProvisioning(NetworkManager::Security8021xSetting::FastProvisioningDisabled);
        } else {
            setting.setPhase1FastProvisioning(static_cast<NetworkManager::Security8021xSetting::FastProvisioning>(m_ui->pacMethod8021x->currentIndex() + 1));
        }

        if (m_ui->pacFile8021x->url().isValid()) {
            setting.setPacFile(QFile::encodeName(m_ui->pacFile8021x->url().toLocalFile()));
        }

        if (m_ui->innerAuth8021x->currentIndex() == 0) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodGtc);
        } else {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschapv2);
        }

        if (!m_ui->password8021x->text().isEmpty()) {
            setting.setPassword(m_ui->password8021x->text());
        }

        if (m_ui->password8021x->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password8021x->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls) {
        if (!m_ui->anonymousIdentity8021x->text().isEmpty()) {
            setting.setAnonymousIdentity(m_ui->anonymousIdentity8021x->text());
        }

        if (!m_ui->domain8021x->text().isEmpty()) {
            setting.setDomainSuffixMatch(m_ui->domain8021x->text());
        }

        if (m_ui->caCert8021x->url().isValid()) {
            setting.setCaCertificate(m_ui->caCert8021x->url().toString().toUtf8().append('\0'));
        }

        const int innerAuth = m_ui->innerAuth8021x->currentIndex();
        if (innerAuth == 0) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodPap);
        } else if (innerAuth == 1) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschap);
        } else if (innerAuth == 2) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschapv2);
        } else if (innerAuth == 3) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodChap);
        }

        if (!m_ui->password8021x->text().isEmpty()) {
            setting.setPassword(m_ui->password8021x->text());
        }

        if (m_ui->password8021x->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password8021x->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        if (!m_ui->anonymousIdentity8021x->text().isEmpty()) {
            setting.setAnonymousIdentity(m_ui->anonymousIdentity8021x->text());
        }

        if (!m_ui->domain8021x->text().isEmpty()) {
            setting.setDomainSuffixMatch(m_ui->domain8021x->text());
        }

        if (m_ui->caCert8021x->url().isValid()) {
            setting.setCaCertificate(m_ui->caCert8021x->url().toString().toUtf8().append('\0'));
        }

        setting.setPhase1PeapVersion(static_cast<NetworkManager::Security8021xSetting::PeapVersion>(m_ui->peapVersion8021x->currentIndex() - 1));
        const int innerAuth = m_ui->innerAuth8021x->currentIndex();
        if (innerAuth == 0) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschapv2);
        } else if (innerAuth == 1) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMd5);
        } else if (innerAuth == 2) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodGtc);
        }

        if (!m_ui->password8021x->text().isEmpty()) {
            setting.setPassword(m_ui->password8021x->text());
        }

        if (m_ui->password8021x->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password8021x->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    }

    return setting.toMap();
}

void WifiConnectionWidget::setStoreSecretsSystemWide(bool system)
{
    PasswordField::PasswordOption option = system ? PasswordField::StoreForAllUsers : PasswordField::StoreForUser;

    m_ui->wepKey->setPasswordOption(option);
    m_ui->leapPassword->setPasswordOption(option);
    m_ui->psk->setPasswordOption(option);
    m_ui->password8021x->setPasswordOption(option);
    m_ui->privateKeyPassword8021x->setPasswordOption(option);
}

void WifiConnectionWidget::onSsidChanged(const QString &ssid)
{
    for (const NetworkManager::Device::Ptr &device : NetworkManager::networkInterfaces()) {
        if (device->type() == NetworkManager::Device::Wifi) {
            NetworkManager::WirelessDevice::Ptr wifiDevice = device.staticCast<NetworkManager::WirelessDevice>();
            if (wifiDevice) {
                for (const NetworkManager::WirelessNetwork::Ptr &wifiNetwork : wifiDevice->networks()) {
                    if (wifiNetwork && wifiNetwork->ssid() == ssid) {
                        NetworkManager::AccessPoint::Ptr ap = wifiNetwork->referenceAccessPoint();
                        NetworkManager::WirelessSecurityType securityType =
                            NetworkManager::findBestWirelessSecurity(wifiDevice->wirelessCapabilities(),
                                                                     true,
                                                                     (wifiDevice->mode() == NetworkManager::WirelessDevice::Adhoc),
                                                                     ap->capabilities(),
                                                                     ap->wpaFlags(),
                                                                     ap->rsnFlags());
                        switch (securityType) {
                        case NetworkManager::WirelessSecurityType::StaticWep:
                            m_ui->securityCombo->setCurrentIndex(WepHex);
                            break;
                        case NetworkManager::WirelessSecurityType::DynamicWep:
                            m_ui->securityCombo->setCurrentIndex(DynamicWep);
                            break;
                        case NetworkManager::WirelessSecurityType::Leap:
                            m_ui->securityCombo->setCurrentIndex(Leap);
                            break;
                        case NetworkManager::WirelessSecurityType::WpaPsk:
                            m_ui->securityCombo->setCurrentIndex(WpaPsk);
                            break;
                        case NetworkManager::WirelessSecurityType::Wpa2Psk:
                            m_ui->securityCombo->setCurrentIndex(WpaPsk);
                            break;
                        case NetworkManager::WirelessSecurityType::WpaEap:
                            m_ui->securityCombo->setCurrentIndex(WpaEap);
                            break;
                        case NetworkManager::WirelessSecurityType::Wpa2Eap:
                            m_ui->securityCombo->setCurrentIndex(WpaEap);
                            break;
                        case NetworkManager::WirelessSecurityType::SAE:
                            m_ui->securityCombo->setCurrentIndex(SAE);
                            break;
                        case NetworkManager::WirelessSecurityType::Wpa3SuiteB192:
                            m_ui->securityCombo->setCurrentIndex(Wpa3SuiteB192);
                            break;
                        case NetworkManager::WirelessSecurityType::OWE:
                            m_ui->securityCombo->setCurrentIndex(OWE);
                            break;
                        default:
                            m_ui->securityCombo->setCurrentIndex(None);
                        }

                        return;
                    }
                }
            }
        }
    }

    // Reset to none security if we don't find any AP or Wifi device
    m_ui->securityCombo->setCurrentIndex(None);
}

void WifiConnectionWidget::setWepKey(int keyIndex)
{
    if (keyIndex == 0) {
        m_ui->wepKey->setText(m_wifiSecurity->wepKey0());
    } else if (keyIndex == 1) {
        m_ui->wepKey->setText(m_wifiSecurity->wepKey1());
    } else if (keyIndex == 2) {
        m_ui->wepKey->setText(m_wifiSecurity->wepKey2());
    } else if (keyIndex == 3) {
        m_ui->wepKey->setText(m_wifiSecurity->wepKey3());
    }
}

void WifiConnectionWidget::securityChanged(int index)
{
    // Hide all security-specific rows first
    m_ui->formLayout->setRowVisible(m_ui->wepKey, false);
    m_ui->formLayout->setRowVisible(m_ui->wepIndex, false);
    m_ui->formLayout->setRowVisible(m_ui->wepAuth, false);
    m_ui->formLayout->setRowVisible(m_ui->leapUsername, false);
    m_ui->formLayout->setRowVisible(m_ui->leapPassword, false);
    m_ui->formLayout->setRowVisible(m_ui->psk, false);

    // Hide all 802.1x rows
    m_ui->formLayout->setRowVisible(m_ui->auth8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->username8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->password8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->domain8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->userCert8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->caCert8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->subjectMatch8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->altSubjectMatches8021xLabel, false);
    m_ui->formLayout->setRowVisible(m_ui->connectToServers8021xLabel, false);
    m_ui->formLayout->setRowVisible(m_ui->privateKey8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->privateKeyPassword8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->pacProvisioning8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->pacFile8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->innerAuth8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->peapVersion8021x, false);

    // Show appropriate rows based on security type
    if (index == None || index == OWE) {
        // No additional widgets needed
    } else if (index == WepHex || index == WepPassphrase) {
        m_ui->formLayout->setRowVisible(m_ui->wepKey, true);
        m_ui->formLayout->setRowVisible(m_ui->wepIndex, true);
        m_ui->formLayout->setRowVisible(m_ui->wepAuth, true);
    } else if (index == Leap) {
        m_ui->formLayout->setRowVisible(m_ui->leapUsername, true);
        m_ui->formLayout->setRowVisible(m_ui->leapPassword, true);
    } else if (index == DynamicWep || index == WpaEap || index == Wpa3SuiteB192) {
        // Show 802.1x authentication selector
        m_ui->formLayout->setRowVisible(m_ui->auth8021x, true);

        // Configure authentication methods based on security type
        // Clear and rebuild the combo box to avoid index conflicts
        m_ui->auth8021x->clear();

        if (index == WpaEap) {
            // WPA/WPA2 Enterprise: TLS, LEAP, PWD, FAST, TTLS, PEAP (no MD5)
            m_ui->auth8021x->addItem(i18n("TLS"), NetworkManager::Security8021xSetting::EapMethodTls);
            m_ui->auth8021x->addItem(i18n("LEAP"), NetworkManager::Security8021xSetting::EapMethodLeap);
            m_ui->auth8021x->addItem(i18n("PWD"), NetworkManager::Security8021xSetting::EapMethodPwd);
            m_ui->auth8021x->addItem(i18n("FAST"), NetworkManager::Security8021xSetting::EapMethodFast);
            m_ui->auth8021x->addItem(i18n("TTLS"), NetworkManager::Security8021xSetting::EapMethodTtls);
            m_ui->auth8021x->addItem(i18n("PEAP"), NetworkManager::Security8021xSetting::EapMethodPeap);
            m_ui->auth8021x->setCurrentIndex(m_ui->auth8021x->findData(NetworkManager::Security8021xSetting::EapMethodPeap));
        } else if (index == Wpa3SuiteB192) {
            // WPA3 Enterprise 192-bit: Only TLS
            m_ui->auth8021x->addItem(i18n("TLS"), NetworkManager::Security8021xSetting::EapMethodTls);
            m_ui->auth8021x->setCurrentIndex(0);
        } else {
            // Dynamic WEP: MD5, TLS, PWD, FAST, TTLS, PEAP (no LEAP)
            m_ui->auth8021x->addItem(i18n("MD5"), NetworkManager::Security8021xSetting::EapMethodMd5);
            m_ui->auth8021x->addItem(i18n("TLS"), NetworkManager::Security8021xSetting::EapMethodTls);
            m_ui->auth8021x->addItem(i18n("PWD"), NetworkManager::Security8021xSetting::EapMethodPwd);
            m_ui->auth8021x->addItem(i18n("FAST"), NetworkManager::Security8021xSetting::EapMethodFast);
            m_ui->auth8021x->addItem(i18n("TTLS"), NetworkManager::Security8021xSetting::EapMethodTtls);
            m_ui->auth8021x->addItem(i18n("PEAP"), NetworkManager::Security8021xSetting::EapMethodPeap);
            m_ui->auth8021x->setCurrentIndex(m_ui->auth8021x->findData(NetworkManager::Security8021xSetting::EapMethodPeap));
        }

        // Call auth8021xChanged to show method-specific fields
        auth8021xChanged(m_ui->auth8021x->currentIndex());
    } else if (index == WpaPsk || index == SAE) {
        m_ui->formLayout->setRowVisible(m_ui->psk, true);
    }

    KAcceleratorManager::manage(this);
}

void WifiConnectionWidget::altSubjectMatches8021xButtonClicked()
{
    QPointer<EditListDialog> editor = new EditListDialog(this);
    editor->setAttribute(Qt::WA_DeleteOnClose);

    editor->setItems(m_ui->subjectMatch8021x->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts));
    editor->setWindowTitle(i18n("Alternative Subject Matches"));
    editor->setToolTip(
        i18n("<qt>This entry must be one of:<ul><li>DNS: &lt;name or ip address&gt;</li><li>EMAIL: &lt;email&gt;</li><li>URI: &lt;uri, e.g. "
             "https://www.kde.org&gt;</li></ul></qt>"));
    editor->setValidator(altSubjectValidator8021x);

    connect(editor.data(), &QDialog::accepted, [editor, this]() {
        m_ui->subjectMatch8021x->setText(editor->items().join(QLatin1String(", ")));
    });
    editor->setModal(true);
    editor->show();
}

void WifiConnectionWidget::connectToServers8021xButtonClicked()
{
    QPointer<EditListDialog> editor = new EditListDialog(this);
    editor->setAttribute(Qt::WA_DeleteOnClose);

    editor->setItems(m_ui->connectToServers8021x->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts));
    editor->setWindowTitle(i18n("Connect to these servers only"));
    editor->setValidator(serversValidator8021x);

    connect(editor.data(), &QDialog::accepted, [editor, this]() {
        m_ui->connectToServers8021x->setText(editor->items().join(QLatin1String(", ")));
    });
    editor->setModal(true);
    editor->show();
}

void WifiConnectionWidget::auth8021xChanged(int index)
{
    Q_UNUSED(index);

    NetworkManager::Security8021xSetting::EapMethod method =
        static_cast<NetworkManager::Security8021xSetting::EapMethod>(m_ui->auth8021x->itemData(m_ui->auth8021x->currentIndex()).toInt());

    // Hide all 802.1x rows first
    m_ui->formLayout->setRowVisible(m_ui->username8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->password8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->domain8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->userCert8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->caCert8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->subjectMatch8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->altSubjectMatches8021xLabel, false);
    m_ui->formLayout->setRowVisible(m_ui->connectToServers8021xLabel, false);
    m_ui->formLayout->setRowVisible(m_ui->privateKey8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->privateKeyPassword8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->pacProvisioning8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->pacFile8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->innerAuth8021x, false);
    m_ui->formLayout->setRowVisible(m_ui->peapVersion8021x, false);

    // Update username label text based on method
    if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        m_ui->username8021xLabel->setText(i18n("Identity:"));
    } else {
        m_ui->username8021xLabel->setText(i18n("Username:"));
    }

    // Show rows based on authentication method
    if (method == NetworkManager::Security8021xSetting::EapMethodMd5) {
        // MD5: Username, Password
        m_ui->formLayout->setRowVisible(m_ui->username8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->password8021x, true);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        // TLS: Identity, Domain, User cert, CA cert, Subject match, Alt subject matches, Servers, Private key, Private key password
        m_ui->formLayout->setRowVisible(m_ui->username8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->domain8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->userCert8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->caCert8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->subjectMatch8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->altSubjectMatches8021xLabel, true);
        m_ui->formLayout->setRowVisible(m_ui->connectToServers8021xLabel, true);
        m_ui->formLayout->setRowVisible(m_ui->privateKey8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->privateKeyPassword8021x, true);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodLeap || method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        // LEAP/PWD: Username, Password
        m_ui->formLayout->setRowVisible(m_ui->username8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->password8021x, true);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        // FAST: Anonymous identity, PAC provisioning, PAC file, Inner auth, Username, Password
        m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->pacProvisioning8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->pacFile8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->innerAuth8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->username8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->password8021x, true);

        // Setup FAST inner auth options
        m_ui->innerAuth8021x->clear();
        m_ui->innerAuth8021x->addItem(i18n("GTC"));
        m_ui->innerAuth8021x->addItem(i18n("MSCHAPv2"));
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls) {
        // TTLS: Anonymous identity, Domain, CA cert, Inner auth, Username, Password
        m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->domain8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->caCert8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->innerAuth8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->username8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->password8021x, true);

        // Setup TTLS inner auth options
        m_ui->innerAuth8021x->clear();
        m_ui->innerAuth8021x->addItem(i18n("PAP"));
        m_ui->innerAuth8021x->addItem(i18n("MSCHAP"));
        m_ui->innerAuth8021x->addItem(i18n("MSCHAPv2"));
        m_ui->innerAuth8021x->addItem(i18n("CHAP"));
    } else if (method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        // PEAP: Anonymous identity, Domain, CA cert, PEAP version, Inner auth, Username, Password
        m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->domain8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->caCert8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->peapVersion8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->innerAuth8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->username8021x, true);
        m_ui->formLayout->setRowVisible(m_ui->password8021x, true);

        // Setup PEAP inner auth options
        m_ui->innerAuth8021x->clear();
        m_ui->innerAuth8021x->addItem(i18n("MSCHAPv2"));
        m_ui->innerAuth8021x->addItem(i18n("MD5"));
        m_ui->innerAuth8021x->addItem(i18n("GTC"));
    }

    KAcceleratorManager::manage(this);
}

bool WifiConnectionWidget::isValid8021x() const
{
    NetworkManager::Security8021xSetting::EapMethod method =
        static_cast<NetworkManager::Security8021xSetting::EapMethod>(m_ui->auth8021x->itemData(m_ui->auth8021x->currentIndex()).toInt());

    if (method == NetworkManager::Security8021xSetting::EapMethodMd5 || method == NetworkManager::Security8021xSetting::EapMethodLeap
        || method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        return !m_ui->username8021x->text().isEmpty()
            && (!m_ui->password8021x->text().isEmpty() || m_ui->password8021x->passwordOption() == PasswordField::AlwaysAsk);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        if (m_ui->username8021x->text().isEmpty()) {
            return false;
        }

        if (!m_ui->privateKey8021x->url().isValid()) {
            return false;
        }

        if (m_ui->privateKeyPassword8021x->passwordOption() == PasswordField::AlwaysAsk) {
            return true;
        }

        if (m_ui->privateKeyPassword8021x->text().isEmpty()) {
            return false;
        }

        QCA::Initializer init;
        QCA::ConvertResult convRes;

        // Try if the private key is in pkcs12 format bundled with client certificate
        if (QCA::isSupported("pkcs12")) {
            QCA::KeyBundle keyBundle = QCA::KeyBundle::fromFile(m_ui->privateKey8021x->url().path(), m_ui->privateKeyPassword8021x->text().toUtf8(), &convRes);
            // We can return the result of decryption when we managed to import the private key
            if (convRes == QCA::ConvertGood) {
                return keyBundle.privateKey().canDecrypt();
            }
        }

        // If the private key is not in pkcs12 format, we need client certificate to be set
        if (!m_ui->userCert8021x->url().isValid()) {
            return false;
        }

        // Try if the private key is in PEM format and return the result of decryption if we managed to open it
        QCA::PrivateKey key = QCA::PrivateKey::fromPEMFile(m_ui->privateKey8021x->url().path(), m_ui->privateKeyPassword8021x->text().toUtf8(), &convRes);
        if (convRes == QCA::ConvertGood) {
            return key.canDecrypt();
        }

        // TODO Try other formats (DER - mainly used in Windows)
        // TODO Validate other certificates??
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        if (!m_ui->pacProvisioning8021x->isChecked() && !m_ui->pacFile8021x->url().isValid()) {
            return false;
        }
        return !m_ui->username8021x->text().isEmpty()
            && (!m_ui->password8021x->text().isEmpty() || m_ui->password8021x->passwordOption() == PasswordField::AlwaysAsk);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls || method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        return !m_ui->username8021x->text().isEmpty()
            && (!m_ui->password8021x->text().isEmpty() || m_ui->password8021x->passwordOption() == PasswordField::AlwaysAsk);
    }

    return true;
}

void WifiConnectionWidget::load8021xConfig(const NetworkManager::Security8021xSetting::Ptr &setting)
{
    const QList<NetworkManager::Security8021xSetting::EapMethod> eapMethods = setting->eapMethods();
    if (eapMethods.isEmpty()) {
        return;
    }

    const NetworkManager::Security8021xSetting::EapMethod method = eapMethods.first();
    const NetworkManager::Security8021xSetting::AuthMethod phase2AuthMethod = setting->phase2AuthMethod();

    // Set password options based on password flags
    if (setting->passwordFlags().testFlag(NetworkManager::Setting::None)) {
        m_ui->password8021x->setPasswordOption(PasswordField::StoreForAllUsers);
        m_ui->privateKeyPassword8021x->setPasswordOption(PasswordField::StoreForAllUsers);
    } else if (setting->passwordFlags().testFlag(NetworkManager::Setting::AgentOwned)) {
        m_ui->password8021x->setPasswordOption(PasswordField::StoreForUser);
        m_ui->privateKeyPassword8021x->setPasswordOption(PasswordField::StoreForUser);
    } else {
        m_ui->password8021x->setPasswordOption(PasswordField::AlwaysAsk);
        m_ui->privateKeyPassword8021x->setPasswordOption(PasswordField::AlwaysAsk);
    }

    // Set the authentication method in the combo box
    const int authIndex = m_ui->auth8021x->findData(method);
    if (authIndex >= 0) {
        m_ui->auth8021x->setCurrentIndex(authIndex);
    }

    // Load method-specific configuration
    if (method == NetworkManager::Security8021xSetting::EapMethodMd5) {
        m_ui->username8021x->setText(setting->identity());
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        m_ui->username8021x->setText(setting->identity());
        m_ui->domain8021x->setText(setting->domainSuffixMatch());

        QByteArray clientCert = setting->clientCertificate();
        if (!clientCert.isEmpty()) {
            clientCert.chop(1); // Remove trailing null
            m_ui->userCert8021x->setUrl(QUrl::fromLocalFile(QString::fromUtf8(clientCert)));
        }

        QByteArray caCert = setting->caCertificate();
        if (!caCert.isEmpty()) {
            caCert.chop(1); // Remove trailing null
            m_ui->caCert8021x->setUrl(QUrl::fromLocalFile(QString::fromUtf8(caCert)));
        }

        m_ui->subjectMatch8021x->setText(setting->subjectMatch());

        QStringList altSubjectMatches = setting->altSubjectMatches();
        QStringList displayMatches;
        QStringList serverMatches;

        for (const QString &match : altSubjectMatches) {
            if (match.startsWith(QLatin1String("DNS:"))) {
                serverMatches.append(match.mid(4));
            } else {
                displayMatches.append(match);
            }
        }

        m_ui->altSubjectMatches8021x->setText(displayMatches.join(QLatin1String(", ")));
        m_ui->connectToServers8021x->setText(serverMatches.join(QLatin1String(", ")));

        QByteArray privateKey = setting->privateKey();
        if (!privateKey.isEmpty()) {
            privateKey.chop(1); // Remove trailing null
            m_ui->privateKey8021x->setUrl(QUrl::fromLocalFile(QString::fromUtf8(privateKey)));
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodLeap) {
        m_ui->username8021x->setText(setting->identity());
    } else if (method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        m_ui->username8021x->setText(setting->identity());
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        m_ui->username8021x->setText(setting->identity());
        m_ui->anonymousIdentity8021x->setText(setting->anonymousIdentity());

        const NetworkManager::Security8021xSetting::FastProvisioning provisioning = setting->phase1FastProvisioning();
        m_ui->pacProvisioning8021x->setChecked(provisioning != NetworkManager::Security8021xSetting::FastProvisioningDisabled);
        if (provisioning != NetworkManager::Security8021xSetting::FastProvisioningDisabled) {
            m_ui->pacMethod8021x->setCurrentIndex(static_cast<int>(provisioning) - 1);
        }

        const QString pacFile = setting->pacFile();
        if (!pacFile.isEmpty()) {
            m_ui->pacFile8021x->setUrl(QUrl::fromLocalFile(pacFile));
        }

        if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodGtc) {
            m_ui->innerAuth8021x->setCurrentIndex(0);
        } else {
            m_ui->innerAuth8021x->setCurrentIndex(1);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls) {
        m_ui->username8021x->setText(setting->identity());
        m_ui->anonymousIdentity8021x->setText(setting->anonymousIdentity());
        m_ui->domain8021x->setText(setting->domainSuffixMatch());

        QByteArray caCert = setting->caCertificate();
        if (!caCert.isEmpty()) {
            caCert.chop(1); // Remove trailing null
            m_ui->caCert8021x->setUrl(QUrl::fromLocalFile(QString::fromUtf8(caCert)));
        }

        if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodPap) {
            m_ui->innerAuth8021x->setCurrentIndex(0);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMschap) {
            m_ui->innerAuth8021x->setCurrentIndex(1);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMschapv2) {
            m_ui->innerAuth8021x->setCurrentIndex(2);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodChap) {
            m_ui->innerAuth8021x->setCurrentIndex(3);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        m_ui->username8021x->setText(setting->identity());
        m_ui->anonymousIdentity8021x->setText(setting->anonymousIdentity());
        m_ui->domain8021x->setText(setting->domainSuffixMatch());

        QByteArray caCert = setting->caCertificate();
        if (!caCert.isEmpty()) {
            caCert.chop(1); // Remove trailing null
            m_ui->caCert8021x->setUrl(QUrl::fromLocalFile(QString::fromUtf8(caCert)));
        }

        const NetworkManager::Security8021xSetting::PeapVersion peapVersion = setting->phase1PeapVersion();
        m_ui->peapVersion8021x->setCurrentIndex(static_cast<int>(peapVersion) + 1);

        if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMschapv2) {
            m_ui->innerAuth8021x->setCurrentIndex(0);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMd5) {
            m_ui->innerAuth8021x->setCurrentIndex(1);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodGtc) {
            m_ui->innerAuth8021x->setCurrentIndex(2);
        }
    }

    // Trigger auth8021xChanged to update field visibility
    auth8021xChanged(m_ui->auth8021x->currentIndex());
}

#include "moc_wificonnectionwidget.cpp"
