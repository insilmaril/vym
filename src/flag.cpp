#include "flag.h"

#include "file.h"
#include "image-container.h"

#include <QDebug>

/////////////////////////////////////////////////////////////////
// Flag
/////////////////////////////////////////////////////////////////
Flag::Flag()
{
    // qDebug() << "Const Flag ()";
    init();
}

Flag::Flag(const QString &fname)
{
    qDebug() << "Const Flag (fname)" << fname;
    init();
    if (!load(fname))
        qWarning() << "Flag::Flag  Failed to load " << fname;
}

Flag::~Flag()
{
    // qDebug() << "Destr Flag  this="<<this <<"  " << qPrintable(name) << "
    // imageContainer=" << imageContainer;
    if (imageContainer)
        delete imageContainer;
}

void Flag::init()
{
    action = nullptr;
    name = "undefined";
    visible = true;
    unsetGroup();

    imageContainer = nullptr;

    state = false;
    used = false;
    type = UndefinedFlag;

    uuid = QUuid::createUuid();
}

bool Flag::load(const QString &fn)
{
    if (!imageContainer)
        imageContainer = new ImageContainer();

    if (!imageContainer->load(fn))
        return false;

    if (fn.contains("svg")) {
        imageContainer->setWidth(32); // FIXME-3 scale svg of flags
    }

    path = fn;

    return true;
}

void Flag::setName(const QString &n)
{
    name = n;
    if (name.contains("/"))
        name = basename(name);

    name = name.section('.', 0, 0);
}

const QString Flag::getName() { return name; }

const QString Flag::getPath() { return path; }

void Flag::setVisible(bool b) { visible = b; }

bool Flag::isVisible() { return visible; }

void Flag::setGroup(const QString &n) { group = n; }

const QString Flag::getGroup() { return group; }

void Flag::unsetGroup() { group.clear(); }

void Flag::setToolTip(const QString &n) { tooltip = n; }

const QString Flag::getToolTip() { return tooltip; }

ImageContainer *Flag::getImageContainer()
{
    if (imageContainer)
        return imageContainer;
    else
        return nullptr;
}

void Flag::setAction(QAction *a) { action = a; }

QAction *Flag::getAction() { return action; }

void Flag::setUsed(bool b) { used = b; }

bool Flag::isUsed() { return used; }

Flag::FlagType Flag::getType() { return type; }

void Flag::setType(Flag::FlagType t) { type = t; }

void Flag::setUuid(const QUuid &id) { uuid = id; }

QUuid Flag::getUuid() { return uuid; }

QString Flag::getDefinition(const QString &prefix)
{
    if (type == Flag::UserFlag) {
        QString url = "flags/" + prefix + uuid.toString() + "-" + name +
                      imageContainer->getExtension();
        QStringList attributes;
        attributes << attribut("name", name);
        attributes << attribut("href", QString("file:%1").arg(url));
        attributes << attribut("uuid", uuid.toString());
        return singleElement("userflagdef", attributes);
    }
    else
        return QString();
}

void Flag::saveDataToDir(const QString &dirPath)
{
    if (imageContainer) {
        path = dirPath + "/" + uuid.toString() + "-" + name +
               imageContainer->getExtension();
        imageContainer->save(path);
    }
}

QString Flag::saveState()
{
    if (type == Flag::UserFlag)
        return singleElement("userflag", attribut("name", name) +
                                             attribut("uuid", uuid.toString()));
    else
        return valueElement("standardflag", name);
}
