#include "flag.h"

#include <QDebug>

/////////////////////////////////////////////////////////////////
// Flag
/////////////////////////////////////////////////////////////////
Flag::Flag()
{
    //qDebug() << "Const Flag ()";
    init ();
}

Flag::Flag(const QString &fname)
{
    init ();
    load (fname);
}

Flag::Flag (Flag* io)
{
    //qDebug() << "Const Flag (Flag);
    copy (io);
}

Flag::~Flag()
{
   //qDebug() << "Destr Flag  this="<<this <<"  " << qPrintable(name);
}


void Flag::init ()
{
    action  = NULL;
    name    = "undefined";
    visible = true;
    unsetGroup();

    state   = false;
    used    = false;
}

void Flag::copy (Flag* other)
{
    action  = other->action;
    name    = other->name;
    group   = other->group;
    tooltip = other->tooltip;
    state   = other->state;
    used    = other->used;
    pixmap  = other->pixmap;
}


void Flag::load (const QString &fn)
{
    if (!pixmap.load(fn))
	qDebug() << "Flag::load (" << fn << ") failed.";
}

void Flag::load (const QPixmap &pm)
{
    pixmap = pm;
}

void Flag::setName(const QString &n)
{
    name = n;
}

const QString Flag::getName()
{
    return name;
}

void Flag::setVisible (bool b)
{
    visible = b;
}

bool Flag::isVisible ()
{
    return visible;
}

void Flag::setGroup (const QString &n)
{
    group = n;
}

const QString Flag::getGroup()
{
    return group;
}

void Flag::unsetGroup()
{
    group.clear();
}

void Flag::setToolTip(const QString &n)
{
    tooltip = n;
}

const QString Flag::getToolTip()
{
    return tooltip;
}

QPixmap Flag::getPixmap()
{
    return pixmap;
}

void Flag::setAction (QAction *a)
{
    action=a;
}

QAction* Flag::getAction ()
{
    return action;
}

void Flag::setUsed (bool b)
{
    used = b;
}

bool Flag::isUsed()
{
    return used;
}

void Flag::saveToDir (const QString &tmpdir, const QString &prefix)
{
    QString fn = tmpdir + prefix + name + ".png";
    pixmap.save (fn, "PNG");
}


