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
#include "qxtdesignerplugin.h"

QxtDesignerPlugin::QxtDesignerPlugin(const QString& plugin) : init(false), plugin(plugin)
{}

QString QxtDesignerPlugin::group() const
{
    return "QxtGui Widgets";
}

QIcon QxtDesignerPlugin::icon() const
{
    return QIcon(":/logo.png");
}

QString QxtDesignerPlugin::includeFile() const
{
    return plugin;
}

void QxtDesignerPlugin::initialize(QDesignerFormEditorInterface*)
{
    if (init) return;
    init = true;
}

bool QxtDesignerPlugin::isContainer() const
{
    return false;
}

bool QxtDesignerPlugin::isInitialized() const
{
    return init;
}

QString QxtDesignerPlugin::name() const
{
    return plugin;
}

QString QxtDesignerPlugin::toolTip() const
{
    return plugin;
}

QString QxtDesignerPlugin::whatsThis() const
{
    return plugin;
}
