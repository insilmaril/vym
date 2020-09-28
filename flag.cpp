#include "flag.h"

#include "file.h"

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
    //qDebug() << "Const Flag (fname)" << fname;
    init ();
    if (!load (fname))
        qWarning() << "Flag::Flag  Failed to load " << fname;
}

Flag::~Flag()
{
   //qDebug() << "Destr Flag  this="<<this <<"  " << qPrintable(name) << "  image=" << image;
   if (image) delete image;
}


void Flag::init ()
{
    action  = NULL;
    name    = "undefined";
    visible = true;
    unsetGroup();

    image = NULL;

    state   = false;
    used    = false;
    type    = UndefinedFlag;

    uuid = QUuid::createUuid();
}

bool Flag::load (const QString &fn) 
{
    if (!image) image = new ImageObj();

    if (!image->load(fn)) return false;

    path = fn;

    return true;
}

void Flag::setName(const QString &n)
{
    name = n;
    if (name.contains("/") )
        name = basename(name);

    name = name.section('.', 0, 0);
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

ImageObj* Flag::getImageObj()
{
    if (image) 
        return image;
    else
        return NULL;
}

void Flag::setAction (QAction *a)
{
    action = a;
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

QString Flag::getDefinition(const QString &prefix)
{
    if (type == Flag::UserFlag) 
    {
        QString url = "flags/" + prefix + uuid.toString() + "-" + name + image->getExtension();
        QStringList attributes;
        attributes << attribut("name", name);
	attributes << attribut ("href", QString ("file:%1").arg(url));
        attributes << attribut("uuid", uuid.toString());
        return singleElement("userflagdef", attributes);
    } else
        return QString();
}

void Flag::saveDataToDir (const QString &dirPath) 
{
    //qDebug() << "Flag::saveDataToDir  " << name << " to " << dirPath << "  image=" << image;
    if (image)
    {
        path = dirPath + "/" + uuid.toString() + "-" + name + image->getExtension();    // FIXME-1 check, if separator converted automagically on windows
        image->save (path);
    }
}

QString Flag::saveState()
{
    if (type == Flag::UserFlag) 
        return singleElement ("userflag", attribut("name", name) + attribut("uuid", uuid.toString()));
    else
        return valueElement ("standardflag", name);
}

