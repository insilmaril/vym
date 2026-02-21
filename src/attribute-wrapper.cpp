#include "attribute-wrapper.h"

#include "attributeitem.h"

#include "mainwindow.h"
#include "vymmodel.h"

extern Main *mainWindow;

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
    bool r = attributeItemInt->heading().isRichText();
    mainWindow->setScriptResult(r);
    return r;
}

QString AttributeWrapper::headingText()
{
    QString r = attributeItemInt->headingPlain();
    mainWindow->setScriptResult(r);
    return r;
}

bool AttributeWrapper::selectParent()
{
    bool r = model()->selectParent(attributeItemInt);
    if (!r)
	mainWindow->abortScript(
                QJSValue::GenericError,
                "Couldn't select parent item");
    mainWindow->setScriptResult(r);
    return r;
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

