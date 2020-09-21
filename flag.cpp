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

Flag::Flag (Flag* io)
{
    qDebug() << "Const Flag (Flag)";
    copy (io);
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

    uuid = QUuid::createUuid(); // FIXME-1 used? and: is path used?

}

void Flag::copy (Flag* other)   //FIXME-0 how to create deep copy of esp. SVG item???
{
    qDebug() << "Flag::copy   other=" << other->name; // FIXME-1 
    action  = other->action;
    name    = other->name;
    group   = other->group;
    tooltip = other->tooltip;
    state   = other->state;
    used    = other->used;
    image   = other->image;
    type    = other->type;
    path    = other->path;
    uuid    = other->uuid;  
}

#include <QRandomGenerator>         // FIXME-1 testing only
bool Flag::load (const QString &fn) // FIXME-0   svg??
{
    QPointF pos =QPointF( 
            QRandomGenerator::global()->generateDouble() * 200,
            QRandomGenerator::global()->generateDouble() * 200);
    qDebug() << "Flag::load fn=" << fn << "  pos=" << pos << " image= " << image;
    if (!image) image = new ImageObj();

    if (!image->load(fn)) return false;

    path = fn;
    image->setVisible (false);   // FIXME-00  usually false, testing
    image->setPos(pos);

    return true;
}

void Flag::load (const QPixmap &pm) // FIXME-0 still needed?
{
    qDebug() << "Flag::load pm";
    if (!image) image = new ImageObj();

    image->load(pm);
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

QString Flag::saveDef(const QString &dirPath) 
{
    if (type == Flag::UserFlag) 
    {
        // Create unique string for filename based on memory address
        ulong n = reinterpret_cast <ulong> (this);

        QString url = "flags/" + uuid.toString() + "-" + name + image->getExtension();
        QStringList attributes;
        attributes << attribut("name", name);
        //attributes << attribut("path", dirPath + uuid.toString() + "-" + name + image->getExtension() );    // FIXME-0 needed?
	attributes << attribut ("href", QString ("file:%1").arg(url));
        attributes << attribut("uuid", uuid.toString());
        return singleElement("userflagdef", attributes);
    } else
        return QString();
}

bool Flag::saveDataToDir (const QString &dirPath) // FIXME-1 save to "flags/standard/" or "flags/user/"? using prefix?
{
    qDebug() << "Flag::saveDataToDir  " << name << " to " << dirPath << "  image=" << image;
    if (image)
    {
        path = dirPath + "/" + uuid.toString() + "-" + name + image->getExtension();    // FIXME-1 check, if separator converted automagically on windows
        return image->save (path);
    }
    return true;    // Nothing to save here FIXME-1  really return true?
}

QString Flag::saveState()
{
    if (type == Flag::UserFlag) 
        return singleElement ("userflag", attribut("name", name) + attribut("uuid", uuid.toString()));
    else
        return valueElement ("standardflag", name);
}

bool Flag::vtest(bool v)  // FIXME-1 testing
{
    qDebug() << "Flag::vtest  v=" << v;
    return v;
}

