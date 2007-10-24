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
#ifndef QXTDESIGNERPLUGIN_H
#define QXTDESIGNERPLUGIN_H

#include <QDesignerCustomWidgetInterface>

class QxtDesignerPlugin : public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    QxtDesignerPlugin(const QString& plugin);

    QString group() const;
    QIcon icon() const;
    QString includeFile () const;
    void initialize(QDesignerFormEditorInterface*);
    bool isContainer() const;
    bool isInitialized() const;
    QString name() const;
    QString toolTip() const;
    QString whatsThis() const;

private:
    bool init;
    QString plugin;
};

#endif // QXTDESIGNERPLUGIN_H
