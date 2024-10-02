#include "image-wrapper.h"

#include "imageitem.h"
#include "image-container.h"

#include "vymmodel.h"
#include "mainwindow.h"

extern Main *mainWindow;

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

qreal ImageWrapper::getPosX()
{
    return setResult(imageItemInt->getImageContainer()->pos().x());
}

qreal ImageWrapper::getPosY()
{
    return setResult(imageItemInt->getImageContainer()->pos().y());
}

qreal ImageWrapper::getScenePosX()
{
    return setResult(imageItemInt->getImageContainer()->scenePos().x());
}

qreal ImageWrapper::getScenePosY()
{
    return setResult(imageItemInt->getImageContainer()->scenePos().y());
}

QString ImageWrapper::headingText()
{
    return setResult(imageItemInt->headingPlain());
}

bool ImageWrapper::selectParent()
{
    bool r = model()->selectParent(imageItemInt);
    if (!r)
        mainWindow->abortScript(
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

