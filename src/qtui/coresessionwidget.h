//
// Created by kuschku on 27.08.17.
//

#ifndef CORESESSIONWIDGET_H
#define CORESESSIONWIDGET_H

#include <QtWidgets/QWidget>
#include <ui_coresessionwidget.h>
#include <QtCore/QMap>

class CoreSessionWidget: public QWidget
{
Q_OBJECT

public:
    explicit CoreSessionWidget(QWidget *);

    void setData(QMap<QString, QVariant>);

private:
    Ui::CoreSessionWidget ui;
};

#endif //CORESESSIONWIDGET_H
