/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtDesigner module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/
#include "qxtpushbutton.h"
#include "qxtpushbuttonplugin.h"
#include <QtPlugin>

QxtPushButtonPlugin::QxtPushButtonPlugin(QObject* parent)
        : QObject(parent), QxtDesignerPlugin("QxtPushButton")
{}
QWidget* QxtPushButtonPlugin::createWidget(QWidget* parent)
{
    return new QxtPushButton(parent);
}

QString QxtPushButtonPlugin::domXml() const
{
    return "<widget class=\"QxtPushButton\" name=\"qxtPushButton\">\n"
           " <property name=\"geometry\">\n"
           "  <rect>\n"
           "   <x>0</x>\n"
           "   <y>0</y>\n"
           "   <width>105</width>\n"
           "   <height>27</height>\n"
           "  </rect>\n"
           " </property>\n"
           " <property name=\"text\" >\n"
           "  <string>QxtPushButton</string>\n"
           " </property>\n"
           "</widget>\n";
}
