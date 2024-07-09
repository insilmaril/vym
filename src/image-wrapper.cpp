#include "image-wrapper.h"

#include "imageitem.h"

#include "vymmodel.h"

#include <QJSEngine>
extern QJSEngine *scriptEngine;

ImageWrapper::ImageWrapper(ImageItem *ii)
{
    //qDebug() << "Constr ImageWrapper (ii)";
    imageItemInt = ii;
}

ImageWrapper::~ImageWrapper()
{
    //qDebug() << "Destr ImageWrapper";
}

VymModel* ImageWrapper::model() {return imageItemInt->getModel();}

ImageItem* ImageWrapper::imageItem() {return imageItemInt;}

bool ImageWrapper::hasRichTextHeading()
{
    return setResult(imageItemInt->heading().isRichText());
}

QString ImageWrapper::headingText()
{
    return setResult(imageItemInt->headingPlain());
}

bool ImageWrapper::selectParent()
{
    bool r = model()->selectParent(imageItemInt);
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't select parent item");
    return setResult(r);
}

void ImageWrapper::setHeadingRichText(const QString &text)
{
    VymText vt;
    vt.setRichText(text);
    model()->setHeading(vt, imageItemInt);
}

void ImageWrapper::setHeadingText(const QString &text)
{
    model()->setHeadingPlainText(text, imageItemInt);
}

