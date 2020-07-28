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
    type    = UndefinedFlag;

    uuid = QUuid::createUuid();
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
    type    = other->type;
    path    = other->path;
    uuid    = other->uuid;  
}


void Flag::load (const QString &fn)
{
    if (!pixmap.load(fn))
	qDebug() << "Flag::load (" << fn << ") failed.";
    else
        path = fn;
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

const QString Flag::getPath()
{
    return path;
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

Flag::FlagType Flag::getType()
{
    return type;
}

void Flag::setType(Flag::FlagType t)
{
    type = t;
}

void Flag::setUuid(const QUuid &id)
{
    uuid = id;
}

QUuid Flag::getUuid() { return uuid; }

QString Flag::saveDef()
{
    if (type == Flag::UserFlag) 
    {
        QStringList attributes;
        attributes << attribut("name", name);
        attributes << attribut("path", path);
        attributes << attribut("uuid", uuid.toString());
        return singleElement("userflagdef", attributes);
    } else
        return QString();
}

bool Flag::saveDataToDir (const QString &tmpdir, const QString &prefix)
{
    QString fn = tmpdir + prefix + name + ".png";
    return pixmap.save (fn, "PNG");
}

QString Flag::saveState()
{
    if (type == Flag::UserFlag) 
        return singleElement ("userflag", attribut("name", name) + attribut("uuid", uuid.toString()));
    else
        return valueElement ("standardflag", name);
}


