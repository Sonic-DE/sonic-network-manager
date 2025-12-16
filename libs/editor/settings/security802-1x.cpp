/*
    SPDX-FileCopyrightText: 2013 Lukas Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "security802-1x.h"
#include "editlistdialog.h"
#include "listvalidator.h"
#include "ui_802-1x.h"

#include <KAcceleratorManager>
#include <KLocalizedString>

#include <QtCrypto>

Security8021x::Security8021x(const NetworkManager::Setting::Ptr &setting, Type type, QWidget *parent, Qt::WindowFlags f)
    : SettingWidget(setting, parent, f)
    , m_setting(setting.staticCast<NetworkManager::Security8021xSetting>())
    , m_ui(new Ui::Security8021x)
{
    m_ui->setupUi(this);

    // Enable password options for all password fields
    m_ui->password->setPasswordOptionsEnabled(true);
    m_ui->privateKeyPassword->setPasswordOptionsEnabled(true);

    // Configure authentication method combo based on type
    if (type == WirelessWpaEap) {
        m_ui->auth->removeItem(0); // Remove MD5
        m_ui->auth->setItemData(0, NetworkManager::Security8021xSetting::EapMethodTls);
        m_ui->auth->setItemData(1, NetworkManager::Security8021xSetting::EapMethodLeap);
        m_ui->auth->setItemData(2, NetworkManager::Security8021xSetting::EapMethodPwd);
        m_ui->auth->setItemData(3, NetworkManager::Security8021xSetting::EapMethodFast);
        m_ui->auth->setItemData(4, NetworkManager::Security8021xSetting::EapMethodTtls);
        m_ui->auth->setItemData(5, NetworkManager::Security8021xSetting::EapMethodPeap);
    } else if (type == WirelessWpaEapSuiteB192) {
        // Remove all methods except TLS
        m_ui->auth->removeItem(6); // Protected EAP (PEAP)
        m_ui->auth->removeItem(5); // Tunneled TLS (TTLS)
        m_ui->auth->removeItem(4); // FAST
        m_ui->auth->removeItem(3); // PWD
        m_ui->auth->removeItem(2); // LEAP
        m_ui->auth->removeItem(0); // MD5
        m_ui->auth->setItemData(0, NetworkManager::Security8021xSetting::EapMethodTls);
    } else {
        m_ui->auth->removeItem(2); // Remove LEAP
        m_ui->auth->setItemData(0, NetworkManager::Security8021xSetting::EapMethodMd5);
        m_ui->auth->setItemData(1, NetworkManager::Security8021xSetting::EapMethodTls);
        m_ui->auth->setItemData(2, NetworkManager::Security8021xSetting::EapMethodPwd);
        m_ui->auth->setItemData(3, NetworkManager::Security8021xSetting::EapMethodFast);
        m_ui->auth->setItemData(4, NetworkManager::Security8021xSetting::EapMethodTtls);
        m_ui->auth->setItemData(5, NetworkManager::Security8021xSetting::EapMethodPeap);
    }

    // Set default authentication method
    if (type == WirelessWpaEapSuiteB192) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodTls));
    } else {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodPeap));
    }

    // Setup validators
    altSubjectValidator = new QRegularExpressionValidator(
        QRegularExpression(
            QLatin1String("^(DNS:[a-zA-Z0-9_-]+\\.[a-zA-Z0-9_.-]+|EMAIL:[a-zA-Z0-9._-]+@[a-zA-Z0-9_-]+\\.[a-zA-Z0-9_.-]+|URI:[a-zA-Z0-9.+-]+:.+|)$")),
        this);
    serversValidator = new QRegularExpressionValidator(QRegularExpression(QLatin1String("^[a-zA-Z0-9_-]+\\.[a-zA-Z0-9_.-]+$")), this);

    auto altSubjectListValidator = new ListValidator(this);
    altSubjectListValidator->setInnerValidator(altSubjectValidator);
    m_ui->subjectMatch->setValidator(altSubjectListValidator);

    auto serverListValidator = new ListValidator(this);
    serverListValidator->setInnerValidator(serversValidator);
    m_ui->connectToServers->setValidator(serverListValidator);

    // Connect button signals
    connect(m_ui->btnAltSubjectMatches, &QPushButton::clicked, this, &Security8021x::altSubjectMatchesButtonClicked);
    connect(m_ui->btnConnectToServers, &QPushButton::clicked, this, &Security8021x::connectToServersButtonClicked);

    // Connect auth changed signal to update visibility
    connect(m_ui->auth, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &Security8021x::currentAuthChanged);

    // Connect for setting check
    watchChangedSetting();

    // Connect for validity check
    connect(m_ui->auth, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &Security8021x::slotWidgetChanged);
    connect(m_ui->username, &KLineEdit::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->password, &PasswordField::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->password, &PasswordField::passwordOptionChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->domain, &KLineEdit::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->anonymousIdentity, &KLineEdit::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->userCert, &KUrlRequester::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->caCert, &KUrlRequester::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->subjectMatch, &QLineEdit::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->altSubjectMatches, &QLineEdit::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->connectToServers, &QLineEdit::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->privateKey, &KUrlRequester::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->privateKeyPassword, &PasswordField::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->privateKeyPassword, &PasswordField::passwordOptionChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->pacProvisioning, &QCheckBox::stateChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->pacMethod, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &Security8021x::slotWidgetChanged);
    connect(m_ui->pacFile, &KUrlRequester::textChanged, this, &Security8021x::slotWidgetChanged);
    connect(m_ui->innerAuth, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &Security8021x::slotWidgetChanged);
    connect(m_ui->peapVersion, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &Security8021x::slotWidgetChanged);

    KAcceleratorManager::manage(this);

    // Initialize visibility based on default auth method
    currentAuthChanged(m_ui->auth->currentIndex());

    if (setting) {
        loadConfig(setting);
    }
}

Security8021x::~Security8021x()
{
    delete m_ui;
}

void Security8021x::loadConfig(const NetworkManager::Setting::Ptr &setting)
{
    NetworkManager::Security8021xSetting::Ptr securitySetting = setting.staticCast<NetworkManager::Security8021xSetting>();

    const QList<NetworkManager::Security8021xSetting::EapMethod> eapMethods = securitySetting->eapMethods();
    const NetworkManager::Security8021xSetting::AuthMethod phase2AuthMethod = securitySetting->phase2AuthMethod();

    // Set password option globally
    if (securitySetting->passwordFlags().testFlag(NetworkManager::Setting::None)) {
        setPasswordOption(PasswordField::StoreForAllUsers);
    } else if (securitySetting->passwordFlags().testFlag(NetworkManager::Setting::AgentOwned)) {
        setPasswordOption(PasswordField::StoreForUser);
    } else {
        setPasswordOption(PasswordField::AlwaysAsk);
    }

    // Load configuration based on EAP method
    if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodMd5)) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodMd5));
        m_ui->username->setText(securitySetting->identity());
    } else if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodTls)) {
        QStringList servers;
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodTls));
        m_ui->username->setText(securitySetting->identity());
        m_ui->domain->setText(securitySetting->domainSuffixMatch());
        m_ui->userCert->setUrl(QUrl::fromLocalFile(securitySetting->clientCertificate().removeLast()));
        m_ui->caCert->setUrl(QUrl::fromLocalFile(securitySetting->caCertificate().removeLast()));
        m_ui->subjectMatch->setText(securitySetting->subjectMatch());
        m_ui->altSubjectMatches->setText(securitySetting->altSubjectMatches().join(QLatin1String(", ")));
        for (const QString &match : securitySetting->altSubjectMatches()) {
            if (match.startsWith(QLatin1String("DNS:"))) {
                servers.append(match.right(match.length() - 4));
            }
        }
        m_ui->connectToServers->setText(servers.join(QLatin1String(", ")));
        m_ui->privateKey->setUrl(QUrl::fromLocalFile(securitySetting->privateKey().removeLast()));
    } else if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodLeap)) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodLeap));
        m_ui->username->setText(securitySetting->identity());
    } else if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodPwd)) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodPwd));
        m_ui->username->setText(securitySetting->identity());
    } else if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodFast)) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodFast));
        m_ui->anonymousIdentity->setText(securitySetting->anonymousIdentity());
        m_ui->pacProvisioning->setChecked((int)securitySetting->phase1FastProvisioning() > 0);
        m_ui->pacMethod->setCurrentIndex(securitySetting->phase1FastProvisioning() - 1);
        m_ui->pacFile->setUrl(QUrl::fromLocalFile(securitySetting->pacFile()));

        // Setup FAST inner auth combo
        m_ui->innerAuth->clear();
        m_ui->innerAuth->addItem(i18n("GTC"));
        m_ui->innerAuth->addItem(i18n("MSCHAPv2"));
        if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodGtc) {
            m_ui->innerAuth->setCurrentIndex(0);
        } else {
            m_ui->innerAuth->setCurrentIndex(1);
        }
        m_ui->username->setText(securitySetting->identity());
    } else if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodTtls)) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodTtls));
        m_ui->anonymousIdentity->setText(securitySetting->anonymousIdentity());
        m_ui->domain->setText(securitySetting->domainSuffixMatch());
        m_ui->caCert->setUrl(QUrl::fromLocalFile(securitySetting->caCertificate().removeLast()));

        // Setup TTLS inner auth combo
        m_ui->innerAuth->clear();
        m_ui->innerAuth->addItem(i18n("PAP"));
        m_ui->innerAuth->addItem(i18n("MSCHAP"));
        m_ui->innerAuth->addItem(i18n("MSCHAPv2"));
        m_ui->innerAuth->addItem(i18n("CHAP"));
        if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodPap) {
            m_ui->innerAuth->setCurrentIndex(0);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMschap) {
            m_ui->innerAuth->setCurrentIndex(1);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMschapv2) {
            m_ui->innerAuth->setCurrentIndex(2);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodChap) {
            m_ui->innerAuth->setCurrentIndex(3);
        }
        m_ui->username->setText(securitySetting->identity());
    } else if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodPeap)) {
        m_ui->auth->setCurrentIndex(m_ui->auth->findData(NetworkManager::Security8021xSetting::EapMethodPeap));
        m_ui->anonymousIdentity->setText(securitySetting->anonymousIdentity());
        m_ui->domain->setText(securitySetting->domainSuffixMatch());
        m_ui->caCert->setUrl(QUrl::fromLocalFile(securitySetting->caCertificate().removeLast()));
        m_ui->peapVersion->setCurrentIndex(securitySetting->phase1PeapVersion() + 1);

        // Setup PEAP inner auth combo
        m_ui->innerAuth->clear();
        m_ui->innerAuth->addItem(i18n("MSCHAPv2"));
        m_ui->innerAuth->addItem(i18n("MD5"));
        m_ui->innerAuth->addItem(i18n("GTC"));
        if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMschapv2) {
            m_ui->innerAuth->setCurrentIndex(0);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodMd5) {
            m_ui->innerAuth->setCurrentIndex(1);
        } else if (phase2AuthMethod == NetworkManager::Security8021xSetting::AuthMethodGtc) {
            m_ui->innerAuth->setCurrentIndex(2);
        }
        m_ui->username->setText(securitySetting->identity());
    }

    loadSecrets(setting);
}

void Security8021x::loadSecrets(const NetworkManager::Setting::Ptr &setting)
{
    NetworkManager::Security8021xSetting::Ptr securitySetting = setting.staticCast<NetworkManager::Security8021xSetting>();

    const QString password = securitySetting->password();
    const QList<NetworkManager::Security8021xSetting::EapMethod> eapMethods = securitySetting->eapMethods();

    // Load password for methods that use it
    if (!password.isEmpty() && !eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodTls)) {
        m_ui->password->setText(password);
    }

    // Load private key password for TLS
    if (eapMethods.contains(NetworkManager::Security8021xSetting::EapMethodTls)) {
        const QString privateKeyPassword = securitySetting->privateKeyPassword();
        if (!privateKeyPassword.isEmpty()) {
            m_ui->privateKeyPassword->setText(privateKeyPassword);
        }
    }
}

QVariantMap Security8021x::setting() const
{
    NetworkManager::Security8021xSetting setting;

    NetworkManager::Security8021xSetting::EapMethod method =
        static_cast<NetworkManager::Security8021xSetting::EapMethod>(m_ui->auth->itemData(m_ui->auth->currentIndex()).toInt());

    setting.setEapMethods(QList<NetworkManager::Security8021xSetting::EapMethod>() << method);

    // Common fields
    if (!m_ui->username->text().isEmpty()) {
        setting.setIdentity(m_ui->username->text());
    }

    // Method-specific configuration
    if (method == NetworkManager::Security8021xSetting::EapMethodMd5) {
        if (!m_ui->password->text().isEmpty()) {
            setting.setPassword(m_ui->password->text());
        }
        if (m_ui->password->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        if (!m_ui->domain->text().isEmpty()) {
            setting.setDomainSuffixMatch(m_ui->domain->text());
        }

        if (m_ui->userCert->url().isValid()) {
            const auto formattingOption = m_ui->userCert->url().scheme() == "file" ? QUrl::PrettyDecoded : QUrl::FullyEncoded;
            setting.setClientCertificate(m_ui->userCert->url().toString(formattingOption).toUtf8().append('\0'));
        }

        if (m_ui->caCert->url().isValid()) {
            const auto formattingOption = m_ui->caCert->url().scheme() == "file" ? QUrl::PrettyDecoded : QUrl::FullyEncoded;
            setting.setCaCertificate(m_ui->caCert->url().toString(formattingOption).toUtf8().append('\0'));
        }

        QStringList altsubjectmatches = m_ui->altSubjectMatches->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString &match : m_ui->connectToServers->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QString tempstr = QLatin1String("DNS:") + match;
            if (!altsubjectmatches.contains(tempstr)) {
                altsubjectmatches.append(tempstr);
            }
        }
        setting.setSubjectMatch(m_ui->subjectMatch->text());
        setting.setAltSubjectMatches(altsubjectmatches);

        if (m_ui->privateKey->url().isValid()) {
            const auto formattingOption = m_ui->privateKey->url().scheme() == "file" ? QUrl::PrettyDecoded : QUrl::FullyEncoded;
            setting.setPrivateKey(m_ui->privateKey->url().toString(formattingOption).toUtf8().append('\0'));
        }

        if (!m_ui->privateKeyPassword->text().isEmpty()) {
            setting.setPrivateKeyPassword(m_ui->privateKeyPassword->text());
        }

        QCA::Initializer init;
        QCA::ConvertResult convRes;

        // Try if the private key is in pkcs12 format bundled with client certificate
        if (QCA::isSupported("pkcs12")) {
            QCA::KeyBundle keyBundle = QCA::KeyBundle::fromFile(m_ui->privateKey->url().path(), m_ui->privateKeyPassword->text().toUtf8(), &convRes);
            // Set client certificate to the same path as private key
            if (convRes == QCA::ConvertGood && keyBundle.privateKey().canDecrypt()) {
                setting.setClientCertificate(m_ui->privateKey->url().toString().toUtf8().append('\0'));
            }
        }

        if (m_ui->privateKeyPassword->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPrivateKeyPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->privateKeyPassword->passwordOption() == PasswordField::StoreForUser) {
            setting.setPrivateKeyPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPrivateKeyPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodLeap || method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        if (!m_ui->password->text().isEmpty()) {
            setting.setPassword(m_ui->password->text());
        }
        if (m_ui->password->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        if (!m_ui->anonymousIdentity->text().isEmpty()) {
            setting.setAnonymousIdentity(m_ui->anonymousIdentity->text());
        }

        if (!m_ui->pacProvisioning->isChecked()) {
            setting.setPhase1FastProvisioning(NetworkManager::Security8021xSetting::FastProvisioningDisabled);
        } else {
            setting.setPhase1FastProvisioning(static_cast<NetworkManager::Security8021xSetting::FastProvisioning>(m_ui->pacMethod->currentIndex() + 1));
        }

        if (m_ui->pacFile->url().isValid()) {
            setting.setPacFile(QFile::encodeName(m_ui->pacFile->url().toLocalFile()));
        }

        if (m_ui->innerAuth->currentIndex() == 0) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodGtc);
        } else {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschapv2);
        }

        if (!m_ui->password->text().isEmpty()) {
            setting.setPassword(m_ui->password->text());
        }

        if (m_ui->password->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls) {
        if (!m_ui->anonymousIdentity->text().isEmpty()) {
            setting.setAnonymousIdentity(m_ui->anonymousIdentity->text());
        }

        if (!m_ui->domain->text().isEmpty()) {
            setting.setDomainSuffixMatch(m_ui->domain->text());
        }

        if (m_ui->caCert->url().isValid()) {
            setting.setCaCertificate(m_ui->caCert->url().toString().toUtf8().append('\0'));
        }

        const int innerAuth = m_ui->innerAuth->currentIndex();
        if (innerAuth == 0) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodPap);
        } else if (innerAuth == 1) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschap);
        } else if (innerAuth == 2) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschapv2);
        } else if (innerAuth == 3) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodChap);
        }

        if (!m_ui->password->text().isEmpty()) {
            setting.setPassword(m_ui->password->text());
        }

        if (m_ui->password->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    } else if (method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        if (!m_ui->anonymousIdentity->text().isEmpty()) {
            setting.setAnonymousIdentity(m_ui->anonymousIdentity->text());
        }

        if (!m_ui->domain->text().isEmpty()) {
            setting.setDomainSuffixMatch(m_ui->domain->text());
        }

        if (m_ui->caCert->url().isValid()) {
            setting.setCaCertificate(m_ui->caCert->url().toString().toUtf8().append('\0'));
        }

        setting.setPhase1PeapVersion(static_cast<NetworkManager::Security8021xSetting::PeapVersion>(m_ui->peapVersion->currentIndex() - 1));
        const int innerAuth = m_ui->innerAuth->currentIndex();
        if (innerAuth == 0) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMschapv2);
        } else if (innerAuth == 1) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodMd5);
        } else if (innerAuth == 2) {
            setting.setPhase2AuthMethod(NetworkManager::Security8021xSetting::AuthMethodGtc);
        }

        if (!m_ui->password->text().isEmpty()) {
            setting.setPassword(m_ui->password->text());
        }

        if (m_ui->password->passwordOption() == PasswordField::StoreForAllUsers) {
            setting.setPasswordFlags(NetworkManager::Setting::None);
        } else if (m_ui->password->passwordOption() == PasswordField::StoreForUser) {
            setting.setPasswordFlags(NetworkManager::Setting::AgentOwned);
        } else {
            setting.setPasswordFlags(NetworkManager::Setting::NotSaved);
        }
    }

    return setting.toMap();
}

void Security8021x::setPasswordOption(PasswordField::PasswordOption option)
{
    m_ui->password->setPasswordOption(option);
    m_ui->privateKeyPassword->setPasswordOption(option);
}

void Security8021x::altSubjectMatchesButtonClicked()
{
    QPointer<EditListDialog> editor = new EditListDialog(this);
    editor->setAttribute(Qt::WA_DeleteOnClose);

    editor->setItems(m_ui->subjectMatch->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts));
    editor->setWindowTitle(i18n("Alternative Subject Matches"));
    editor->setToolTip(
        i18n("<qt>This entry must be one of:<ul><li>DNS: &lt;name or ip address&gt;</li><li>EMAIL: &lt;email&gt;</li><li>URI: &lt;uri, e.g. "
             "https://www.kde.org&gt;</li></ul></qt>"));
    editor->setValidator(altSubjectValidator);

    connect(editor.data(), &QDialog::accepted, [editor, this]() {
        m_ui->subjectMatch->setText(editor->items().join(QLatin1String(", ")));
    });
    editor->setModal(true);
    editor->show();
}

void Security8021x::connectToServersButtonClicked()
{
    QPointer<EditListDialog> editor = new EditListDialog(this);
    editor->setAttribute(Qt::WA_DeleteOnClose);

    editor->setItems(m_ui->connectToServers->text().remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts));
    editor->setWindowTitle(i18n("Connect to these servers only"));
    editor->setValidator(serversValidator);

    connect(editor.data(), &QDialog::accepted, [editor, this]() {
        m_ui->connectToServers->setText(editor->items().join(QLatin1String(", ")));
    });
    editor->setModal(true);
    editor->show();
}

bool Security8021x::isValid() const
{
    NetworkManager::Security8021xSetting::EapMethod method =
        static_cast<NetworkManager::Security8021xSetting::EapMethod>(m_ui->auth->itemData(m_ui->auth->currentIndex()).toInt());

    if (method == NetworkManager::Security8021xSetting::EapMethodMd5 || method == NetworkManager::Security8021xSetting::EapMethodLeap
        || method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        return !m_ui->username->text().isEmpty() && (!m_ui->password->text().isEmpty() || m_ui->password->passwordOption() == PasswordField::AlwaysAsk);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        if (m_ui->username->text().isEmpty()) {
            return false;
        }

        if (!m_ui->privateKey->url().isValid()) {
            return false;
        }

        if (m_ui->privateKeyPassword->passwordOption() == PasswordField::AlwaysAsk) {
            return true;
        }

        if (m_ui->privateKeyPassword->text().isEmpty()) {
            return false;
        }

        QCA::Initializer init;
        QCA::ConvertResult convRes;

        // Try if the private key is in pkcs12 format bundled with client certificate
        if (QCA::isSupported("pkcs12")) {
            QCA::KeyBundle keyBundle = QCA::KeyBundle::fromFile(m_ui->privateKey->url().path(), m_ui->privateKeyPassword->text().toUtf8(), &convRes);
            // We can return the result of decryption when we managed to import the private key
            if (convRes == QCA::ConvertGood) {
                return keyBundle.privateKey().canDecrypt();
            }
        }

        // If the private key is not in pkcs12 format, we need client certificate to be set
        if (!m_ui->userCert->url().isValid()) {
            return false;
        }

        // Try if the private key is in PEM format and return the result of decryption if we managed to open it
        QCA::PrivateKey key = QCA::PrivateKey::fromPEMFile(m_ui->privateKey->url().path(), m_ui->privateKeyPassword->text().toUtf8(), &convRes);
        if (convRes == QCA::ConvertGood) {
            return key.canDecrypt();
        }

        // TODO Try other formats (DER - mainly used in Windows)
        // TODO Validate other certificates??
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        if (!m_ui->pacProvisioning->isChecked() && !m_ui->pacFile->url().isValid()) {
            return false;
        }
        return !m_ui->username->text().isEmpty() && (!m_ui->password->text().isEmpty() || m_ui->password->passwordOption() == PasswordField::AlwaysAsk);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls || method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        return !m_ui->username->text().isEmpty() && (!m_ui->password->text().isEmpty() || m_ui->password->passwordOption() == PasswordField::AlwaysAsk);
    }

    return true;
}

void Security8021x::currentAuthChanged(int index)
{
    Q_UNUSED(index);

    NetworkManager::Security8021xSetting::EapMethod method =
        static_cast<NetworkManager::Security8021xSetting::EapMethod>(m_ui->auth->itemData(m_ui->auth->currentIndex()).toInt());

    // Hide all rows first
    m_ui->formLayout->setRowVisible(m_ui->username, false);
    m_ui->formLayout->setRowVisible(m_ui->password, false);
    m_ui->formLayout->setRowVisible(m_ui->domain, false);
    m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity, false);
    m_ui->formLayout->setRowVisible(m_ui->userCert, false);
    m_ui->formLayout->setRowVisible(m_ui->caCert, false);
    m_ui->formLayout->setRowVisible(m_ui->subjectMatch, false);
    m_ui->formLayout->setRowVisible(m_ui->altSubjectMatches, false);
    m_ui->formLayout->setRowVisible(m_ui->connectToServers, false);
    m_ui->formLayout->setRowVisible(m_ui->privateKey, false);
    m_ui->formLayout->setRowVisible(m_ui->privateKeyPassword, false);
    m_ui->formLayout->setRowVisible(m_ui->pacProvisioning, false);
    m_ui->formLayout->setRowVisible(m_ui->pacFile, false);
    m_ui->formLayout->setRowVisible(m_ui->innerAuth, false);
    m_ui->formLayout->setRowVisible(m_ui->peapVersion, false);

    // Update username label text based on method
    if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        m_ui->usernameLabel->setText(i18n("Identity:"));
    } else {
        m_ui->usernameLabel->setText(i18n("Username:"));
    }

    // Show rows based on authentication method
    if (method == NetworkManager::Security8021xSetting::EapMethodMd5) {
        // MD5: Username, Password
        m_ui->formLayout->setRowVisible(m_ui->username, true);
        m_ui->formLayout->setRowVisible(m_ui->password, true);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTls) {
        // TLS: Identity, Domain, User cert, CA cert, Subject match, Alt subject matches, Servers, Private key, Private key password
        m_ui->formLayout->setRowVisible(m_ui->username, true);
        m_ui->formLayout->setRowVisible(m_ui->domain, true);
        m_ui->formLayout->setRowVisible(m_ui->userCert, true);
        m_ui->formLayout->setRowVisible(m_ui->caCert, true);
        m_ui->formLayout->setRowVisible(m_ui->subjectMatch, true);
        m_ui->formLayout->setRowVisible(m_ui->altSubjectMatches, true);
        m_ui->formLayout->setRowVisible(m_ui->connectToServers, true);
        m_ui->formLayout->setRowVisible(m_ui->privateKey, true);
        m_ui->formLayout->setRowVisible(m_ui->privateKeyPassword, true);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodLeap || method == NetworkManager::Security8021xSetting::EapMethodPwd) {
        // LEAP/PWD: Username, Password
        m_ui->formLayout->setRowVisible(m_ui->username, true);
        m_ui->formLayout->setRowVisible(m_ui->password, true);
    } else if (method == NetworkManager::Security8021xSetting::EapMethodFast) {
        // FAST: Anonymous identity, PAC provisioning, PAC file, Inner auth, Username, Password
        m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity, true);
        m_ui->formLayout->setRowVisible(m_ui->pacProvisioning, true);
        m_ui->formLayout->setRowVisible(m_ui->pacFile, true);
        m_ui->formLayout->setRowVisible(m_ui->innerAuth, true);
        m_ui->formLayout->setRowVisible(m_ui->username, true);
        m_ui->formLayout->setRowVisible(m_ui->password, true);

        // Setup FAST inner auth options
        m_ui->innerAuth->clear();
        m_ui->innerAuth->addItem(i18n("GTC"));
        m_ui->innerAuth->addItem(i18n("MSCHAPv2"));
    } else if (method == NetworkManager::Security8021xSetting::EapMethodTtls) {
        // TTLS: Anonymous identity, Domain, CA cert, Inner auth, Username, Password
        m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity, true);
        m_ui->formLayout->setRowVisible(m_ui->domain, true);
        m_ui->formLayout->setRowVisible(m_ui->caCert, true);
        m_ui->formLayout->setRowVisible(m_ui->innerAuth, true);
        m_ui->formLayout->setRowVisible(m_ui->username, true);
        m_ui->formLayout->setRowVisible(m_ui->password, true);

        // Setup TTLS inner auth options
        m_ui->innerAuth->clear();
        m_ui->innerAuth->addItem(i18n("PAP"));
        m_ui->innerAuth->addItem(i18n("MSCHAP"));
        m_ui->innerAuth->addItem(i18n("MSCHAPv2"));
        m_ui->innerAuth->addItem(i18n("CHAP"));
    } else if (method == NetworkManager::Security8021xSetting::EapMethodPeap) {
        // PEAP: Anonymous identity, Domain, CA cert, PEAP version, Inner auth, Username, Password
        m_ui->formLayout->setRowVisible(m_ui->anonymousIdentity, true);
        m_ui->formLayout->setRowVisible(m_ui->domain, true);
        m_ui->formLayout->setRowVisible(m_ui->caCert, true);
        m_ui->formLayout->setRowVisible(m_ui->peapVersion, true);
        m_ui->formLayout->setRowVisible(m_ui->innerAuth, true);
        m_ui->formLayout->setRowVisible(m_ui->username, true);
        m_ui->formLayout->setRowVisible(m_ui->password, true);

        // Setup PEAP inner auth options
        m_ui->innerAuth->clear();
        m_ui->innerAuth->addItem(i18n("MSCHAPv2"));
        m_ui->innerAuth->addItem(i18n("MD5"));
        m_ui->innerAuth->addItem(i18n("GTC"));
    }

    KAcceleratorManager::manage(this);
}

#include "moc_security802-1x.cpp"
