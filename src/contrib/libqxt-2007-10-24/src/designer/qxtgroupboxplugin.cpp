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
#include "qxtgroupbox.h"
#include "qxtgroupboxplugin.h"
#include <QtPlugin>

QxtGroupBoxPlugin::QxtGroupBoxPlugin(QObject* parent)
        : QObject(parent), QxtDesignerPlugin("QxtGroupBox")
{}

QWidget* QxtGroupBoxPlugin::createWidget(QWidget* parent)
{
    return new QxtGroupBox(parent);
}

QString QxtGroupBoxPlugin::domXml() const
{
    return "<widget class=\"QxtGroupBox\" name=\"qxtGroupBox\">\n"
           " <property name=\"geometry\">\n"
           "  <rect>\n"
           "   <x>0</x>\n"
           "   <y>0</y>\n"
           "   <width>120</width>\n"
           "   <height>80</height>\n"
           "  </rect>\n"
           " </property>\n"
           "</widget>\n";
}
