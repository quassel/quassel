#include "coresessionwidget.h"
#include <QtCore/QDateTime>
#include <client.h>


CoreSessionWidget::CoreSessionWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    connect(ui.disconnectButton, SIGNAL(released()), this, SLOT(disconnectClicked()));
}

void CoreSessionWidget::setData(QMap<QString, QVariant> map)
{
    QLabel *iconSecure = ui.iconSecure;

    ui.labelRemoteAddress->setText(map["remoteAddress"].toString());
    ui.labelLocation->setText(map["location"].toString());
    ui.labelClient->setText(map["clientVersion"].toString());
    ui.labelVersionDate->setText(map["clientVersionDate"].toString());
    ui.labelUptime
        ->setText(map["connectedSince"].toDateTime().toLocalTime().toString(Qt::DateFormat::SystemLocaleShortDate));
    if (map["location"].toString().isEmpty()) {
        ui.labelLocation->hide();
        ui.labelLocationTitle->hide();
    }

    bool success = false;
    _peerId = map["id"].toInt(&success);
    if (!success) _peerId = -1;
}

void CoreSessionWidget::disconnectClicked()
{
    emit disconnectClicked(_peerId);
}
