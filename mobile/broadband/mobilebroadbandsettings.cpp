/*
 *   Copyright 2018 Martin Kacej <m.kacej@atlas.sk>
 *             2020 Devin Lin <espidev@gmail.com>
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

#include "mobilebroadbandsettings.h"

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>

#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/GsmSetting>
#include <NetworkManagerQt/CdmaSetting>

#include <QQmlEngine>

K_PLUGIN_CLASS_WITH_JSON(MobileBroadbandSettings, "mobilebroadbandsettings.json")

MobileBroadbandSettings::MobileBroadbandSettings(QObject* parent, const QVariantList& args) : KQuickAddons::ConfigModule(parent, args)
{
    KAboutData* about = new KAboutData("kcm_mobile_broadband", i18n("Configure mobile broadband"),
                                       "0.1", QString(), KAboutLicense::GPL);
    about->addAuthor(i18n("Martin Kacej"), QString(), "m.kacej@atlas.sk");
    about->addAuthor(i18n("Devin Lin"), QString(), "espidev@gmail.com");
    setAboutData(about);
    
    qmlRegisterSingletonInstance("mobilebroadbandkcm", 1, 0, "APNProfileModel", APNProfileModel::instance());
    qmlRegisterType<APNProfile>("mobilebroadbandkcm", 1, 0, "APNProfile");
    qmlRegisterType<NetworkType>("mobilebroadbandkcm", 1, 0, "NetworkType");
    
//     m_providers = new MobileProviders();
//     m_providers->getApns();
    
    APNProfileModel::instance();
    
    // update ui bearers list when changed
    connect(this, &MobileBroadbandSettings::bearersChanged, this, &MobileBroadbandSettings::updateBearerProfileModel);
    
    // find modem
    ModemManager::scanDevices();
    m_modemDevice = ModemManager::findModemDevice(getModemDevice()); // TODO check if modem changes?
    
    if (m_modemDevice) {
        m_modemInterface = m_modemDevice->modemInterface();
        
        // add capability list
        if (m_modemInterface) {
            QList<MMModemCapability> caps = m_modemInterface->supportedCapabilities();
            QList<NetworkType *> types;
            
            // prettify capabilities
            if (caps.contains(MM_MODEM_CAPABILITY_5GNR)) {
                types.append(new NetworkType(this, "5G", MM_MODEM_CAPABILITY_5GNR));
            }
            if (caps.contains(MM_MODEM_CAPABILITY_LTE)) {
                types.append(new NetworkType(this, "4G", MM_MODEM_CAPABILITY_LTE));
            }
            if (caps.contains(MM_MODEM_CAPABILITY_CDMA_EVDO) || caps.contains(MM_MODEM_CAPABILITY_GSM_UMTS)) {
                types.append(new NetworkType(this, "3G", QFlags(MM_MODEM_CAPABILITY_CDMA_EVDO) | QFlags(MM_MODEM_CAPABILITY_GSM_UMTS)));
            }
            if (caps.contains(MM_MODEM_CAPABILITY_IRIDIUM)) {
                types.append(new NetworkType(this, i18n("Iridium (Satellite)"), MM_MODEM_CAPABILITY_IRIDIUM));
            }
            if (caps.contains(MM_MODEM_CAPABILITY_POTS)) {
                types.append(new NetworkType(this, "POTS", MM_MODEM_CAPABILITY_POTS));
            }
        }
    }
    
    // find active bearer
    this->updateActiveBearer();
    if (m_modemDevice) {
        connect(m_modemInterface.data(), &ModemManager::Modem::bearersChanged, this, &MobileBroadbandSettings::bearersChanged);
        APNProfileModel::instance()->refresh(m_modemDevice->bearers());
    }
}

MobileBroadbandSettings::~MobileBroadbandSettings()
{
//     delete m_providers;
}

bool MobileBroadbandSettings::mobileDataActive()
{
    return m_mobileDataActive;
}

void MobileBroadbandSettings::setMobileDataActive(bool active)
{
    if (m_mobileDataActive != active) {
        m_mobileDataActive = active;
        Q_EMIT mobileDataActiveChanged();
    }
}

bool MobileBroadbandSettings::allowRoaming()
{
    if (m_bearer) {
        return m_bearer->properties()["allow-roaming"].toBool(); // TODO actually determine qvariantmap key (this is a guess)
    } else {
        return false;
    }
}

void MobileBroadbandSettings::setAllowRoaming(bool allowRoaming)
{
    if (m_bearer) {
        m_bearer->properties()["allow-roaming"] = allowRoaming;
        Q_EMIT bearersChanged(); // TODO may be unnecessary as the modem possibly already emits bearersChanged signal
    }
}

QString MobileBroadbandSettings::activeAPNProfileUni()
{
    if (m_bearer) {
        return m_bearer->uni();
    }
    return QString();
}

void MobileBroadbandSettings::setActiveAPNProfileUni(QString uni)
{
    if (m_modemDevice) {
        ModemManager::Bearer::Ptr bearer = m_modemDevice->findBearer(uni);
        if (bearer == nullptr) return;
        
        QDBusPendingReply<void> reply;
        if (m_bearer) {
            reply = m_bearer->disconnectBearer();
            reply.waitForFinished();
            if (reply.isError()) {
                qWarning() << "Error disconnecting bearer" << reply.error().message();
            }
        }
    
        reply = bearer->connectBearer();
        if (reply.isError()) {
            qWarning() << "Error connecting bearer" << reply.error().message();
        }
    }
}

QList<NetworkType *> MobileBroadbandSettings::capabilities()
{
    return m_capabilities;
}

void MobileBroadbandSettings::toggleCapability(NetworkType *nt)
{
    if (m_modemInterface) {
        ModemManager::Modem::Capabilities cap = m_modemInterface->currentCapabilities();
        cap ^= nt->flag();
        m_modemInterface->setCurrentCapabilities(cap);
        
        nt->setEnabled(m_modemInterface->currentCapabilities() & nt->flag());
    }
}

QString MobileBroadbandSettings::getModemDevice()
{
    ModemManager::ModemDevice::List list = ModemManager::modemDevices();
    if (list.length() == 0) {
        return QString();
    }
    
    ModemManager::ModemDevice::Ptr device;
    foreach (const ModemManager::ModemDevice::Ptr &md, list) {
        ModemManager::Modem::Ptr m = md->modemInterface();
        if (!m->isEnabled())
            continue;
        // TODO powerState ???
        if (m->state() <= MM_MODEM_STATE_REGISTERED)
            continue; // needs inspection
        if (m->accessTechnologies() <= MM_MODEM_ACCESS_TECHNOLOGY_GSM)
            continue;
        if (m->currentCapabilities() <= MM_MODEM_CAPABILITY_GSM_UMTS)
            continue;
        device = md;
    }
    if (device) {
        qWarning() << device->uni() << device->modemInterface()->uni();
        return device->uni();
    }
    return QString();
}

void MobileBroadbandSettings::updateActiveBearer()
{
    m_bearer = nullptr;
    if (!m_modemDevice) return;
    if (m_modemDevice->bearers().empty()) {
        qWarning() << "No bearers in modem found";
        return;
    }
    
    for (const ModemManager::Bearer::Ptr bearer : m_modemDevice->bearers()) {
        // TODO determine what to do if several bearers are connected (if that's possible)?
        if (bearer->isConnected()) {
            m_bearer = bearer;
            
            qInfo() << "Found active bearer with properties:" << bearer->properties();
            
            Q_EMIT allowRoamingChanged();
            Q_EMIT activeBearerChanged();
            
            // TODO determine if this needs to be deleted when nullptr (singleshot)
            connect(bearer.data(), &ModemManager::Bearer::connectedChanged, this, [this]() -> void { 
                updateActiveBearer(); 
            });
            
            break;
        }
    }
}

void MobileBroadbandSettings::updateBearerProfileModel()
{
    if (m_modemDevice) {
        updateActiveBearer();
        APNProfileModel::instance()->refresh(m_modemDevice->bearers());
    }
}


#include "mobilebroadbandsettings.moc"
