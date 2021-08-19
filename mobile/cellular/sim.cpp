/*
 *   Copyright 2021 Devin Lin <espidev@gmail.com>
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

#include "sim.h"

#include <KLocalizedString>

Sim::Sim(QObject *parent, ModemManager::Sim::Ptr sim, ModemManager::Modem::Ptr modem)
    : QObject{ parent },
      m_sim{ sim },
      m_modem{ modem }
{
    connect(m_sim.data(), &ModemManager::Sim::imsiChanged, this, [this]() -> void { Q_EMIT imsiChanged(); });
    connect(m_sim.data(), &ModemManager::Sim::operatorIdentifierChanged, this, [this]() -> void { Q_EMIT operatorIdentifierChanged(); });
    connect(m_sim.data(), &ModemManager::Sim::operatorNameChanged, this, [this]() -> void { Q_EMIT operatorNameChanged(); });
    connect(m_sim.data(), &ModemManager::Sim::simIdentifierChanged, this, [this]() -> void { Q_EMIT simIdentifierChanged(); });
    
    // TODO connect sim unlock
}

bool Sim::locked()
{
    return m_modem->unlockRequired() != MM_MODEM_LOCK_NONE;
}

QString Sim::lockedReason()
{
    switch (m_modem->unlockRequired()) {
        case MM_MODEM_LOCK_UNKNOWN:
            return i18n("Lock reason unknown.");
        case MM_MODEM_LOCK_NONE:
            return i18n("Modem is unlocked.");
        case MM_MODEM_LOCK_SIM_PIN:
            return i18n("SIM requires the PIN code.");
        case MM_MODEM_LOCK_SIM_PIN2:
            return i18n("SIM requires the PIN2 code.");
        case MM_MODEM_LOCK_SIM_PUK:
            return i18n("SIM requires the PUK code.");
        case MM_MODEM_LOCK_SIM_PUK2:
            return i18n("SIM requires the PUK2 code.");
        case MM_MODEM_LOCK_PH_SP_PIN:
            return i18n("Modem requires the service provider PIN code.");
        case MM_MODEM_LOCK_PH_SP_PUK:
            return i18n("Modem requires the service provider PUK code.");
        case MM_MODEM_LOCK_PH_NET_PIN:
            return i18n("Modem requires the network PIN code.");
        case MM_MODEM_LOCK_PH_NET_PUK:
            return i18n("Modem requires the network PUK code.");
        case MM_MODEM_LOCK_PH_SIM_PIN:
            return i18n("Modem requires the PIN code.");
        case MM_MODEM_LOCK_PH_CORP_PIN:
            return i18n("Modem requires the corporate PIN code.");
        case MM_MODEM_LOCK_PH_CORP_PUK:
            return i18n("Modem requires the corporate PUK code.");
        case MM_MODEM_LOCK_PH_FSIM_PIN:
            return i18n("Modem requires the PH-FSIM PIN code.");
        case MM_MODEM_LOCK_PH_FSIM_PUK:
            return i18n("Modem requires the PH-FSIM PUK code.");
        case MM_MODEM_LOCK_PH_NETSUB_PIN:
            return i18n("Modem requires the network subset PIN code.");
        case MM_MODEM_LOCK_PH_NETSUB_PUK:
            return i18n("Modem requires the network subset PUK code.");
    }
    return QString{};
}

QString Sim::imsi()
{
    return m_sim->imsi();
}

QString Sim::eid()
{
    return ""; // TODO add in mm-qt
}

QString Sim::operatorIdentifier()
{
    return m_sim->operatorIdentifier();
}

QString Sim::operatorName()
{
    return m_sim->operatorName();
}

QString Sim::simIdentifier()
{
    return m_sim->simIdentifier();
}

QStringList Sim::emergencyNumbers()
{
    return {}; // TODO add in mm-qt
}

QString Sim::uni()
{
    return m_sim->uni();
}
