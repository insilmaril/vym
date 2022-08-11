#include <QDebug>

#include "flag.h"
#include "flag-container.h"

FlagContainer::FlagContainer()
{
    // qDebug() << "Const FlagContainer  this=" << info();
    init();
}

FlagContainer::~FlagContainer()
{
    // qDebug() << "Destr FlagContainer  this=" << info() << "  " << name;
}

void FlagContainer::init()
{
    avis = true;
    type = FlagCont;
}

void FlagContainer::setUuid(const QUuid &id) { uid = id; }

const QUuid FlagContainer::getUuid() { return uid; }

void FlagContainer::setAlwaysVisible(bool b) { avis = b; }

bool FlagContainer::isAlwaysVisible() { return avis; }

