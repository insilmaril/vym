#include "attribute-wrapper.h"

#include "attributeitem.h"

#include "vymmodel.h"

#include <QJSEngine>
extern QJSEngine *scriptEngine;

AttributeWrapper::AttributeWrapper(AttributeItem *ai)
{
    //qDebug() << "Constr AttributeWrapper (ii)";
    attributeItemInt = ai;
}

AttributeWrapper::~AttributeWrapper()
{
    //qDebug() << "Destr AttributeWrapper";
}

VymModel* AttributeWrapper::model() {return attributeItemInt->getModel();}

AttributeItem* AttributeWrapper::attributeItem() {return attributeItemInt;}

bool AttributeWrapper::hasRichTextHeading()
{
    return setResult(attributeItemInt->heading().isRichText());
}

QString AttributeWrapper::headingText()
{
    return setResult(attributeItemInt->headingPlain());
}

bool AttributeWrapper::selectParent()
{
    /*
    bool r = model()->selectParent(attributeItemInt);
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't select parent item");
    return setResult(r);
    */
    return false;
}

void AttributeWrapper::setHeadingRichText(const QString &text)
{
    /*
    VymText vt;
    vt.setRichText(text);
    model()->setHeading(vt, attributeItemInt);
    */
}

void AttributeWrapper::setHeadingText(const QString &text)
{
    model()->setHeadingPlainText(text, attributeItemInt);
}

