#include <QDebug>

#include "xlink.h"

#include "branchitem.h"
#include "misc.h"
#include "scripting-xlink-wrapper.h"
#include "vymmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

class VymModel;

/////////////////////////////////////////////////////////////////
// Link
/////////////////////////////////////////////////////////////////

XLink::XLink(VymModel *m)
{
    // qDebug() << "Const Link () this="<<this;
    model = m;
    init();
}

XLink::~XLink()
{
    // qDebug() << "* Destr Link begin this=" << this;

    delete (xlo);

    if (xlinkWrapperInt) {
        delete xlinkWrapperInt;
        xlinkWrapperInt = nullptr;
    }
    // XLinkItems are deleted in VymModel::deleteXLinkInt()
}

void XLink::init()
{
    uuid = QUuid::createUuid();

    xlo = nullptr;
    beginBranch = nullptr;
    endBranch = nullptr;
    beginXLinkItemInt = nullptr;
    endXLinkItemInt = nullptr;
    xLinkState = XLink::undefinedXLink;

    type = Bezier;
    pen = model->mapDesign()->defXLinkPen();

    xlinkWrapperInt = nullptr;
}

void XLink::setUuid(const QString &id) { uuid = QUuid(id); }

QUuid XLink::getUuid() { return uuid; }

VymModel* XLink::getModel() { return model; }

XLinkWrapper* XLink::xlinkWrapper()
{
    if (!xlinkWrapperInt)
        xlinkWrapperInt = new XLinkWrapper(this);
    return xlinkWrapperInt;
}

void XLink::setBeginBranch(BranchItem *bi)
{
    if (bi) {
        xLinkState = initXLink;
        beginBranch = bi;
    }
}

BranchItem *XLink::getBeginBranch() { return beginBranch; }

void XLink::setEndBranch(BranchItem *bi)
{
    if (bi)
        endBranch = bi;
}

BranchItem *XLink::getEndBranch() { return endBranch; }

void XLink::setEndPoint(QPointF p)
{
    // Used only while creating the link, without endBranch
    if (xlo)
        xlo->setEnd(p);
}

void XLink::setBeginXLinkItem(XLinkItem *li)
{
    if (li) {
        xLinkState = initXLink;
        beginXLinkItemInt = li;
    }
}

XLinkItem *XLink::beginXLinkItem() { return beginXLinkItemInt;}

void XLink::setEndXLinkItem(XLinkItem *li)
{
    if (li) {
        xLinkState = initXLink;
        endXLinkItemInt = li;
    }
}

XLinkItem *XLink::endXLinkItem() { return endXLinkItemInt; }

void XLink::setPen(const QPen &p)
{
    pen = p;
    if (xlo)
        xlo->updateGeometry();
}

void XLink::unsetXLinkItem(XLinkItem *xli)
{
    if (xli == beginXLinkItemInt) {
        beginXLinkItemInt = nullptr;
        xLinkState = deleteXLink;
    } else if (xli == endXLinkItemInt) {
        endXLinkItemInt = nullptr;
        xLinkState = deleteXLink;
    }
}

QPen XLink::getPen() { return pen; }

void XLink::setLinkType(const QString &s)
{
    if (s == "Linear")
        type = Linear;
    else if (s == "Bezier")
        type = Bezier;
    else
        qWarning() << "XLink::setLinkType  Unknown type: " << s;
}

void XLink::setStyleBegin(const QString &s)
{
    if (xlo) {
        xlo->setStyleBegin(s);
        xlo->updateGeometry();
    }
}

QString XLink::getStyleBeginString()
{
    if (xlo)
        return ArrowObj::styleToString(xlo->getStyleBegin());
    else
        return QString();
}

void XLink::setStyleEnd(const QString &s)
{
    if (xlo) {
        xlo->setStyleEnd(s);
        xlo->updateGeometry();
    }
}

QString XLink::getStyleEndString()
{
    if (xlo)
        return ArrowObj::styleToString(xlo->getStyleEnd());
    else
        return QString();
}

bool XLink::activate()
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

XLink::XLinkState XLink::getState() { return xLinkState; }

void XLink::updateXLink()
{
    if (xlo)
        xlo->updateGeometry();
}

QString XLink::saveToDir()
{
    //    qDebug()<<"XLink::saveToDir  this="<<this<<"
    //    beginBranch="<<beginBranch<<"  endBranch="<<endBranch<<"
    //    state="<<xLinkState;
    QString s = "";
    if (beginBranch && endBranch && xLinkState == activeXLink) {
        if (beginBranch == endBranch)
            qWarning(
                "XLink::saveToDir  ignored, because beginBranch==endBranch, ");
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

XLinkObj *XLink::getXLinkObj() { return xlo; }

XLinkObj *XLink::createXLinkObj()
{
    if (!xlo)
        xlo = new XLinkObj(this);
    return xlo;
}
