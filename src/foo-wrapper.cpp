#include "foo-wrapper.h"

#include "xlink.h"

#include "vymmodel.h"

#include <QJSEngine>
extern QJSEngine *scriptEngine;

XLinkWrapper::XLinkWrapper()
{
    qDebug() << "Constr XLinkWrapper (ii)";
    xlinkInt = nullptr;
}

XLinkWrapper::~XLinkWrapper()
{
    qDebug() << "Destr XLinkWrapper";
}

VymModel* XLinkWrapper::model() {return xlinkInt->getModel();}

XLink* XLinkWrapper::xlink() {return xlinkInt;}

int XLinkWrapper::getWidth() {return 123;}

