/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtCore module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** There is aditional information in the LICENSE file of libqxt.
** If you did not receive a copy of the file try to download it or
** contact the libqxt Management
** 
** <http://libqxt.sourceforge.net>  <aep@exys.org>
**
****************************************************************************/
#include <QTimer>
#include <QxtCursesApplication.h>
#include <QxtCFrame.h>
#include "cursestest.h"

QxtCFrame *_f, *_g, *_h;

void Test::moveWindows() {
    static int step = 0;
    if(step<2)
        _f->move(_f->x()+1, _f->y());
    else if(step<4)
        _g->move(_g->x()+1, _g->y());
    else if(step<6)
        _h->resize(_h->width()+1, _h->height());
    else if(step<8)
        _f->move(_f->x(), _f->y()+1);
    else if(step<10)
        _g->move(_g->x(), _g->y()+1);
    else if(step<12)
        _h->resize(_h->width(), _h->height()+1);
    else if(step<14)
        _f->move(_f->x()-1, _f->y());
    else if(step<16)
        _g->move(_g->x()-1, _g->y());
    else if(step<18)
        _h->resize(_h->width()-1, _h->height());
    else if(step<20)
        _f->move(_f->x(), _f->y()-1);
    else if(step<22)
        _g->move(_g->x(), _g->y()-1);
    else
        _h->resize(_h->width(), _h->height()-1);

    step = (step+1)%24;
}

int main(int argc, char** argv) {
    QxtCursesApplication app(argc, argv);
    QxtCFrame f;
    f.move(20,10);
    f.resize(60,60);
    QxtCFrame* g = new QxtCFrame(&f);
    QxtCFrame* h = new QxtCFrame(g);
    g->move(10,10);
    g->resize(60,20);
    h->move(5,5);
    h->resize(5,5);
    f.update();
    g->update();
    h->update();
    _f = &f;
    _g = g;
    _h = h;
    QTimer t;
    Test test;
    QObject::connect(&t, SIGNAL(timeout()), &test, SLOT(moveWindows()));
    t.start(100);
    app.exec();
}
