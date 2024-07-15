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
    uuid = QUuid::createUuid();

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
            QStringList attrs;

            attrs << attribute("color", pen.color().name());
            attrs << attribute("width", QString().setNum(pen.width(), 10));
            attrs << attribute("penstyle", penStyleToString(pen.style()));
            switch (type) {
                case Linear:
                    attrs << attribute("type", "Linear");
                    break;
                case Bezier:
                    attrs << attribute("type", "Bezier");
                    if (xlo) {
                         attrs << attribute("c0", pointToString(xlo->getC0()));
                         attrs << attribute("c1", pointToString(xlo->getC1()));
                    }
                    break;
            }
            attrs << attribute("beginID", beginBranch->getUuid().toString());
            attrs << attribute("endID", endBranch->getUuid().toString());
            if (xlo) {
                attrs << attribute("styleBegin", ArrowObj::styleToString(xlo->getStyleBegin()));
                attrs << attribute("styleEnd", ArrowObj::styleToString(xlo->getStyleEnd()));
            }
            attrs << attribute("uuid", uuid.toString());

            s = singleElement("xlink", attrs.join(""));
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
