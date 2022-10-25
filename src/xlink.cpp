#include <QDebug>

#include "xlink.h"

#include "branchitem.h"
#include "misc.h"
#include "vymmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

class VymModel;

/////////////////////////////////////////////////////////////////
// Link
/////////////////////////////////////////////////////////////////

Link::Link(VymModel *m)
{
    // qDebug() << "Const Link () this="<<this;
    model = m;
    init();
}

Link::~Link()
{
    //    qDebug()<<"* Destr Link begin this="<<this<<"  bLI="<<beginLinkItem<<"
    //    eLI="<<endLinkItem;
    deactivate();
    //    qDebug()<<"* Destr Link end   this="<<this;
}

void Link::init()
{
    xlo = NULL;
    beginBranch = NULL;
    endBranch = NULL;
    beginLinkItem = NULL;
    endLinkItem = NULL;
    xLinkState = Link::undefinedXLink;

    type = Bezier;
    pen = model->getMapDefXLinkPen();
}

VymModel *Link::getModel() { return model; }

void Link::setBeginBranch(BranchItem *bi)
{
    if (bi) {
        xLinkState = initXLink;
        beginBranch = bi;
    }
}

BranchItem *Link::getBeginBranch() { return beginBranch; }

void Link::setEndBranch(BranchItem *bi)
{
    if (bi) {
        endBranch = bi;
        if (xlo)
            xlo->initC1();
    }
}

BranchItem *Link::getEndBranch() { return endBranch; }

void Link::setEndPoint(QPointF p)
{
    // Used only while creating the link, without endBranch
    if (xlo)
        xlo->setEnd(p);
}

void Link::setBeginLinkItem(XLinkItem *li)
{
    if (li) {
        xLinkState = initXLink;
        beginLinkItem = li;
    }
}

XLinkItem *Link::getBeginLinkItem() { return beginLinkItem; }

void Link::setEndLinkItem(XLinkItem *li)
{
    if (li) {
        xLinkState = initXLink;
        endLinkItem = li;
    }
}

XLinkItem *Link::getEndLinkItem() { return endLinkItem; }

XLinkItem *Link::getOtherEnd(XLinkItem *xli)
{
    if (xli == beginLinkItem)
        return endLinkItem;
    if (xli == endLinkItem)
        return beginLinkItem;
    return NULL;
}

void Link::setPen(const QPen &p)
{
    pen = p;
    if (xlo)
        xlo->updateXLink();
}

QPen Link::getPen() { return pen; }

void Link::setLinkType(const QString &s)
{
    if (s == "Linear")
        type = Linear;
    else if (s == "Bezier")
        type = Bezier;
    else
        qWarning() << "Link::setLinkType  Unknown type: " << s;
}

void Link::setStyleBegin(const QString &s)
{
    if (xlo) {
        xlo->setStyleBegin(s);
        xlo->updateXLink();
    }
}

QString Link::getStyleBeginString()
{
    if (xlo)
        return ArrowObj::styleToString(xlo->getStyleBegin());
    else
        return QString();
}

void Link::setStyleEnd(const QString &s)
{
    if (xlo) {
        xlo->setStyleEnd(s);
        xlo->updateXLink();
    }
}

QString Link::getStyleEndString()
{
    if (xlo)
        return ArrowObj::styleToString(xlo->getStyleEnd());
    else
        return QString();
}

bool Link::activate()
{
    if (beginBranch && endBranch) {
        if (beginBranch == endBranch)
            return false;
        xLinkState = activeXLink;
        model->updateActions();
        return true;
    }
    else
        return false;
}

void Link::deactivate()
{
    // Remove pointers from XLinkItem to Link and
    // delete XLinkObj

    //    qDebug()<<"Link::deactivate ******************************";
    xLinkState = deleteXLink;
    if (beginLinkItem)
        beginLinkItem->setLink(NULL);
    if (endLinkItem)
        endLinkItem->setLink(NULL);
    if (xlo) {
        delete (xlo);
        xlo = NULL;
    }
}

Link::XLinkState Link::getState() { return xLinkState; }

void Link::removeXLinkItem(XLinkItem *xli)
{
    // Only mark _one_ end for removal here!
    if (xli == beginLinkItem)
        beginLinkItem = NULL;
    if (xli == endLinkItem)
        endLinkItem = NULL;
    xLinkState = deleteXLink;
}

void Link::updateLink()
{
    if (xlo)
        xlo->updateXLink();
}

QString Link::saveToDir()
{
    //    qDebug()<<"Link::saveToDir  this="<<this<<"
    //    beginBranch="<<beginBranch<<"  endBranch="<<endBranch<<"
    //    state="<<xLinkState;
    QString s = "";
    if (beginBranch && endBranch && xLinkState == activeXLink) {
        if (beginBranch == endBranch)
            qWarning(
                "Link::saveToDir  ignored, because beginBranch==endBranch, ");
        else {
            QString colAttr = attribut("color", pen.color().name());
            QString widAttr =
                attribut("width", QString().setNum(pen.width(), 10));
            QString styAttr =
                attribut("penstyle", penStyleToString(pen.style()));
            QString ctrlAttr;
            QString typeAttr;
            switch (type) {
            case Linear:
                typeAttr = attribut("type", "Linear");
                break;
            case Bezier:
                typeAttr = attribut("type", "Bezier");
                if (xlo) {
                    ctrlAttr += attribut("c0", pointToString(xlo->getC0()));
                    ctrlAttr += attribut("c1", pointToString(xlo->getC1()));
                }
                break;
            }
            QString begSelAttr =
                attribut("beginID", model->getSelectString(beginBranch));
            QString endSelAttr =
                attribut("endID", model->getSelectString(endBranch));
            QString styleAttr;
            if (xlo) {
                styleAttr =
                    QString(" styleBegin=\"%1\"")
                        .arg(ArrowObj::styleToString(xlo->getStyleBegin()));
                styleAttr +=
                    QString(" styleEnd=\"%1\"")
                        .arg(ArrowObj::styleToString(xlo->getStyleEnd()));
            }
            s = singleElement("xlink", colAttr + widAttr + styAttr + typeAttr +
                                           ctrlAttr + begSelAttr + endSelAttr +
                                           styleAttr);
        }
    }
    return s;
}

XLinkObj *Link::getXLinkObj() { return xlo; }

XLinkObj *Link::createXLinkObj()
{
    if (!xlo)
        xlo = new XLinkObj(this);
    xlo->setVisibility();
    return xlo;
}
