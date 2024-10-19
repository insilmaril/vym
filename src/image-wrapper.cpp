#include "image-wrapper.h"

#include "branch-wrapper.h"
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
    bool r = imageItemInt->heading().isRichText();
    mainWindow->setScriptResult(r);
    return r;
}

qreal ImageWrapper::getPosX()
{
    qreal r = imageItemInt->getImageContainer()->pos().x();
    mainWindow->setScriptResult(r);
    return r;
}

qreal ImageWrapper::getPosY()
{
    qreal r = imageItemInt->getImageContainer()->pos().y();
    mainWindow->setScriptResult(r);
    return r;
}

qreal ImageWrapper::getScenePosX()
{
    qreal r = imageItemInt->getImageContainer()->scenePos().x();
    mainWindow->setScriptResult(r);
    return r;
}

qreal ImageWrapper::getScenePosY()
{
    qreal r = imageItemInt->getImageContainer()->scenePos().y();
    mainWindow->setScriptResult(r);
    return r;
}

QString ImageWrapper::headingText()
{
    QString r = imageItemInt->headingPlain();
    mainWindow->setScriptResult(r);
    return r;
}

bool ImageWrapper::relinkToBranch(BranchWrapper *dst)
{
    bool r = model()->relinkImage(imageItemInt, (TreeItem*)(dst->branchItem()));
    mainWindow->setScriptResult(r);
    return r;
}

bool ImageWrapper::relinkToBranchAt(BranchWrapper *dst, int pos)
{
    bool r = model()->relinkImage(imageItemInt, (TreeItem*)(dst->branchItem()), pos);
    mainWindow->setScriptResult(r);
    return r;
}

bool ImageWrapper::selectParent()
{
    bool r = model()->selectParent(imageItemInt);
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Couldn't select parent item");
    mainWindow->setScriptResult(r);
    return r;
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

void ImageWrapper::setHideLinkUnselected(bool b)
{
    model()->setHideLinkUnselected(b, imageItemInt);
}

void ImageWrapper::setPos(qreal x, qreal y)
{
    model()->setPos(QPointF(x, y), imageItemInt);
}

