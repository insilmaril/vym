#include <QDebug>

#include "heading-container.h"

#include "headingobj.h" //FIXME-1 probably move content from HO here to HC later
#include "mapobj.h"     // FIXME-1 needed?

HeadingContainer::HeadingContainer(QGraphicsItem *parent) : Container(parent) 

{
    //qDebug() << "* Const HeadingContainer begin this = " << this;
    init();
}

HeadingContainer::~HeadingContainer()
{
    //qDebug() << "* Destr HeadingContainer" << name << this;

    delete headingObj;
}

void HeadingContainer::init()
{
    type = Container::Heading;

    headingObj = new HeadingObj(this);
    setText("");
}

void HeadingContainer::setText(const QString &s)// FIXME-2 richtext has wrong position
{
    // Update heading in container
    if (s != headingObj->text()) headingObj->setText(s);

    QRectF r = rect();
    if (!s.isEmpty())  {
        r.setWidth(headingObj->getBBox().width());
        r.setHeight(headingObj->getBBox().height());
    }
    setRect(r);

    setName(QString("HC (%1)").arg(s));
}

QString HeadingContainer::getName() {
    return Container::getName() + " '" + headingObj->text() + "'";
}

void HeadingContainer::reposition()
{
    // qDebug() << "HC::reposition " + info();
    return;
}

