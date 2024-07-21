#include "foo-wrapper.h"

#include "xlink.h"

#include "vymmodel.h"

#include <QJSEngine>
extern QJSEngine *scriptEngine;

FooWrapper::FooWrapper()
{
    qDebug() << "Constr FooWrapper (ii)";
    xlinkInt = nullptr;
}

FooWrapper::~FooWrapper()
{
    qDebug() << "Destr FooWrapper";
}

VymModel* FooWrapper::model() {return xlinkInt->getModel();}

XLink* FooWrapper::xlink() {return xlinkInt;}

int FooWrapper::getWidth() {return 123;}

