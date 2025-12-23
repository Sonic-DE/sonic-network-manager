/*
    SPDX-FileCopyrightText: 2025 Alexander Wilms <f.alexander.wilms@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "connectionstatuswidget.h"
#include "connectiondetails.h"

#include <KLocalizedString>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>

#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/Device>

ConnectionStatusWidget::ConnectionStatusWidget(const QString &connectionUuid, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_connectionUuid(connectionUuid)
{
    m_stackedLayout = new QStackedLayout(this);
    m_stackedLayout->setSizeConstraint(QLayout::SetMinimumSize);
    m_stackedLayout->setContentsMargins(0, 0, 0, 0);

    // Page 0: Disconnected state
    auto *disconnectedPage = new QWidget(this);
    auto *disconnectedLayout = new QVBoxLayout(disconnectedPage);
    m_disconnectedLabel = new QLabel(i18n("Disconnected"), this);
    disconnectedLayout->addStretch(1);
    disconnectedLayout->addWidget(m_disconnectedLabel, 0, Qt::AlignHCenter);
    disconnectedLayout->addStretch(1);
    m_stackedLayout->addWidget(disconnectedPage);

    // Page 1: Details state with scroll area
    auto *detailsPage = new QWidget(this);
    auto *detailsPageLayout = new QVBoxLayout(detailsPage);
    detailsPageLayout->setContentsMargins(0, 0, 0, 0);

    // Create scroll area
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    // Container for the form (inside scroll area)
    m_formContainer = new QWidget();

    // Horizontal layout to center the form
    auto *horizontalLayout = new QHBoxLayout(m_formContainer);
    horizontalLayout->addStretch(1);

    auto *formWidget = new QWidget();
    m_containerLayout = new QVBoxLayout(formWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);

    // Create QFormLayout for connection details
    m_detailsLayout = new QFormLayout();
    m_detailsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_detailsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_detailsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    m_detailsLayout->setVerticalSpacing(2); // Small spacing between rows
    m_containerLayout->addLayout(m_detailsLayout);

    horizontalLayout->addWidget(formWidget);
    horizontalLayout->addStretch(1);

    scrollArea->setWidget(m_formContainer);
    detailsPageLayout->addWidget(scrollArea);

    m_stackedLayout->addWidget(detailsPage);

    updateConnectionDetails();
}

ConnectionStatusWidget::~ConnectionStatusWidget() = default;

void ConnectionStatusWidget::setConnectionUuid(const QString &uuid)
{
    if (m_connectionUuid != uuid) {
        m_connectionUuid = uuid;
        updateConnectionDetails();
    }
}

void ConnectionStatusWidget::setDetailsSource(QObject *networkModelItem)
{
    m_detailsSource = networkModelItem;
    m_connection = nullptr;
    m_device = nullptr;
    updateConnectionDetails();
}

void ConnectionStatusWidget::setConnectionAndDevice(const NetworkManager::Connection::Ptr &connection, const NetworkManager::Device::Ptr &device)
{
    m_connection = connection;
    m_device = device;
    m_detailsSource = nullptr;
    updateConnectionDetails();
}

QList<ConnectionDetails::ConnectionDetailSection> ConnectionStatusWidget::getConnectionDetails() const
{
    // If a details source was set (NetworkModelItem from applet), call its detailsList() method
    if (m_detailsSource) {
        QVariant result;
        bool success = QMetaObject::invokeMethod(m_detailsSource, "detailsList", Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
        if (success && result.canConvert<QList<ConnectionDetails::ConnectionDetailSection>>()) {
            return result.value<QList<ConnectionDetails::ConnectionDetailSection>>();
        }
    }

    // If connection and device were set directly (KCM), use ConnectionDetails helper
    if (m_connection && m_device) {
        return ConnectionDetails::getConnectionDetails(m_connection, m_device);
    }

    // Fallback: no details source or connection/device
    return {};
}

void ConnectionStatusWidget::updateConnectionDetails()
{
    // Clear existing form rows before populating
    while (m_detailsLayout->rowCount() > 0) {
        m_detailsLayout->removeRow(0);
    }

    const auto detailsList = getConnectionDetails();

    if (detailsList.isEmpty()) {
        m_stackedLayout->setCurrentIndex(0); // Show "Disconnected" label
    } else {
        bool firstSection = true;
        // Populate the details form
        for (const auto &section : detailsList) {
            const QString &sectionName = section.title;
            const auto &items = section.details;

            // Add some spacing before the section (except for first section)
            if (!firstSection) {
                m_detailsLayout->addItem(new QSpacerItem(0, 12, QSizePolicy::Minimum, QSizePolicy::Fixed));
            }
            firstSection = false;

            QLabel *sectionLabel = new QLabel(sectionName, this);
            QFont sectionFont = sectionLabel->font();
            sectionFont.setBold(true);
            sectionFont.setPointSize(sectionFont.pointSize() + 1);
            sectionLabel->setFont(sectionFont);
            sectionLabel->setAlignment(Qt::AlignHCenter);
            // Add as two-column spanning row
            m_detailsLayout->addRow(sectionLabel);
            // Add small spacing after section header
            m_detailsLayout->addItem(new QSpacerItem(0, 4, QSizePolicy::Minimum, QSizePolicy::Fixed));

            for (const auto &[label, value] : items) {
                // Create label widget with colon and top alignment
                QLabel *labelWidget = new QLabel(label + QLatin1Char(':'), this);
                labelWidget->setAlignment(Qt::AlignRight | Qt::AlignTop);
                labelWidget->setWordWrap(false);

                // Create value label widget for the field column
                QLabel *valueLabel = new QLabel(value, this);
                valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
                valueLabel->setWordWrap(true);
                valueLabel->setTextFormat(Qt::PlainText); // Prevent HTML interpretation
                valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
                valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

                // Add row with both widgets
                m_detailsLayout->addRow(labelWidget, valueLabel);

                // Fix height after layout calculates width to prevent spacing changes
                QTimer::singleShot(0, this, [valueLabel]() {
                    // Calculate proper height based on actual width
                    int properHeight = valueLabel->heightForWidth(valueLabel->width());
                    if (properHeight > 0) {
                        // Use Fixed policy to prevent any expansion/shrinking
                        valueLabel->setFixedHeight(properHeight);
                    }
                });
            }
        }
        m_stackedLayout->setCurrentIndex(1); // Show details form
    }
}