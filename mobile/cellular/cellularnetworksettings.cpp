/*
 *   Copyright 2018 Martin Kacej <m.kacej@atlas.sk>
 *             2020-2021 Devin Lin <espidev@gmail.com>
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

#include "cellularnetworksettings.h"

#include <KPluginFactory>
#include <KLocalizedString>
#include <KAboutData>
#include <KUser>

#include <QQmlEngine>

K_PLUGIN_CLASS_WITH_JSON(CellularNetworkSettings, "cellularnetworksettings.json")

CellularNetworkSettings::CellularNetworkSettings(QObject* parent, const QVariantList& args) 
    : KQuickAddons::ConfigModule(parent, args),
      m_modem{ nullptr },
      m_modemList{},
      m_providers{ nullptr }
{
    KAboutData* about = new KAboutData("kcm_cellular_network", i18n("Management of cellular networks"),
                                       "0.1", QString(), KAboutLicense::GPL);
    about->addAuthor(i18n("Devin Lin"), QString(), "espidev@gmail.com");
    about->addAuthor(i18n("Martin Kacej"), QString(), "m.kacej@atlas.sk");
    setAboutData(about);
    
    qmlRegisterType<ProfileSettings>("cellularnetworkkcm", 1, 0, "ProfileSettings");
    qmlRegisterType<Modem>("cellularnetworkkcm", 1, 0, "Modem");
    
    // parse mobile providers list
    m_providers = new MobileProviders();
    m_providers->fillProvidersList();
    
    // find modems
    ModemManager::scanDevices();
    
    qDebug() << "Scanning for modems...";
    for (ModemManager::ModemDevice::Ptr device : ModemManager::modemDevices()) {
        ModemManager::Modem::Ptr modem = device->modemInterface();
        NetworkManager::ModemDevice::Ptr nmModem;
        
        for (NetworkManager::Device::Ptr nmDevice : NetworkManager::networkInterfaces()) {
            if (nmDevice->udi() == device->uni()) {
                nmModem = nmDevice.objectCast<NetworkManager::ModemDevice>();
            }
        }
        
        qDebug() << "Found modem:" << device->uni() << modem->uni();
        if (!nmModem) {
            qWarning() << "NetworkManager ModemDevice could not be found for this modem! Ignoring...";
        } else {
            m_modemList.push_back(new Modem(this, device, nmModem, modem, m_providers));
            m_modem = m_modemList[m_modemList.size() - 1]; // TODO
        }
    }
    
    if (m_modemList.empty()) {
        qDebug() << "No modems found.";
    }
}

CellularNetworkSettings::~CellularNetworkSettings()
{
    for (auto p : m_modemList) {
        delete p;
    }
}

bool CellularNetworkSettings::modemFound()
{
    return !m_modemList.empty();
}

bool CellularNetworkSettings::hasSim()
{
    return m_modem && m_modem->hasSim();
}

Modem *CellularNetworkSettings::modem()
{
    return m_modem;
}

#include "cellularnetworksettings.moc"
