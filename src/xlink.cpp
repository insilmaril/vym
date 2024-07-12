#include <QDebug>

#include "xlink.h"

#include "branchitem.h"
#include "misc.h"
#include "vymmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"
#include "xlink-wrapper.h"

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
    //qDebug() << "* Destr Link begin this=" << this;

    delete (xlo);

    if (xlinkWrapperInt) {
        delete xlinkWrapperInt;
        xlinkWrapperInt = nullptr;
    }
    
    // XLinkItems are deleted in VymModel::deleteXLink()
}

void Link::init()
{
    xlo = nullptr;
    beginBranch = nullptr;
    endBranch = nullptr;
    beginXLinkItemInt = nullptr;
    endXLinkItemInt = nullptr;
    xLinkState = Link::undefinedXLink;

    type = Bezier;
    pen = model->mapDesign()->defXLinkPen();

    xlinkWrapperInt = nullptr;
}

void Link::setUuid(const QString &id) { uuid = QUuid(id); }

QUuid Link::getUuid() { return uuid; }

VymModel* Link::getModel() { return model; }

XLinkWrapper* Link::xlinkWrapper()
{
    if (!xlinkWrapperInt)
        xlinkWrapperInt = new XLinkWrapper(this);
    return xlinkWrapperInt;
}

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
    if (bi)
        endBranch = bi;
}

BranchItem *Link::getEndBranch() { return endBranch; }

void Link::setEndPoint(QPointF p)
{
    // Used only while creating the link, without endBranch
    if (xlo)
        xlo->setEnd(p);
}

void Link::setBeginXLinkItem(XLinkItem *li)
{
    if (li) {
        xLinkState = initXLink;
        beginXLinkItemInt = li;
    }
}

XLinkItem *Link::beginXLinkItem() { return beginXLinkItemInt;}

void Link::setEndXLinkItem(XLinkItem *li)
{
    if (li) {
        xLinkState = initXLink;
        endXLinkItemInt = li;
    }
}

XLinkItem *Link::endXLinkItem() { return endXLinkItemInt; }

XLinkItem *Link::getOtherEnd(XLinkItem *xli)
{
    if (xli == beginXLinkItemInt)
        return endXLinkItemInt;
    if (xli == endXLinkItemInt)
        return beginXLinkItemInt;
    return nullptr;
}

void Link::setPen(const QPen &p)
{
    pen = p;
    if (xlo)
        xlo->updateGeometry();
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
        xlo->updateGeometry();
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
        xlo->updateGeometry();
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
        if (xlo) xlo->initC1();
        model->updateActions();
        return true;
    }
    else
        return false;
}

Link::XLinkState Link::getState() { return xLinkState; }

void Link::updateLink()
{
    if (xlo)
        xlo->updateGeometry();
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
            QString colAttr = attribute("color", pen.color().name());
            QString widAttr =
                attribute("width", QString().setNum(pen.width(), 10));
            QString styAttr =
                attribute("penstyle", penStyleToString(pen.style()));
            QString ctrlAttr;
            QString typeAttr;
            switch (type) {
            case Linear:
                typeAttr = attribute("type", "Linear");
                break;
            case Bezier:
                typeAttr = attribute("type", "Bezier");
                if (xlo) {
                    ctrlAttr += attribute("c0", pointToString(xlo->getC0()));
                    ctrlAttr += attribute("c1", pointToString(xlo->getC1()));
                }
                break;
            }
            QString begSelAttr =
                attribute("beginID", beginBranch->getUuid().toString());
            QString endSelAttr =
                attribute("endID", endBranch->getUuid().toString());
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
    return xlo;
}
