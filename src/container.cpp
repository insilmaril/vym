#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "mapobj.h"

Container::Container(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    //qDebug() << "* Const Container begin this = " << this << ";
    init();
}

Container::~Container()
{
    qDebug() << "* Destr Container" << name << this;
}

void Container::init()
{
    type = Undefined;
    contentType = UndefinedContent;
    contentObj = nullptr;

    layout = Horizontal;

    // Testing only:
    QPointF p; // = QPointF(qrand() % 100, qrand() % 100);
    qreal w = qrand() % 200 + 20;
    qreal h = qrand() % 200 + 20;
    setRect(QRectF(p.x(), p.y(), w, h));
    show();
}

void Container::setContentType(const ContentType &ctype)
{
    contentType = ctype;
}

Container::ContentType Container::getContentType()
{
    return contentType;
}

Container::ContainerType Container::containerType()
{
    return type;
}

void Container::setContent(MapObj *mapObj)
{
    contentObj = mapObj;
    contentType = MapObject;
}

void Container::setLayoutType(const LayoutType &ltype)
{
    layout = ltype;
}

void Container::addContainer(Container *c)
{
    qDebug() << "Adding container " << c->getName() << c << " to " << name << this;
    c->setParentItem(this);
}

void Container::reposition()
{
    qDebug() << QString("Container::reposition of %1 container  (contentType: %2, containerType: %3").arg(name).arg(contentType).arg(type);

    QRectF r = rect();

    // Repositioning is done recursively. First the size sizes of subcontainers are calculated
    // Then the subcontainers are positioned.
    //
    // a) calc sizes of subcontainers based on their layouts
    if (type == Heading) {
        // size is already updated when heading itself changes, no need to recalc here.
        return;
    } else {
        //qDebug() << " * ContainerType: not heading;
        if (childItems().count() == 0) {
            // qDebug() << " * Setting r to minimal size";
            r.setWidth(0);
            r.setHeight(0);
            setRect(r);
        }
    }

    // FIXME-1 testing only, rotate children
    /*
    Container *pc = (Container*)parentItem();
    if (pc) {
        TreeItem *ti = pc->getTreeItem();
        if (ti && ti->getHeadingPlain() =="rot")
        {
            qDebug() << " * Setting rot";
            setRotation(20);
        }
    }
    */

    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        // FIXME-0 don't call, if c has no children, otherwise sizes become 0
        c = (Container*) child; // FIXME-0    why the cast here????
        //qDebug() << " * Repositioning childItem " << child << " of type " << c->contentType;
        if (c->contentType == Containers || c->contentType == MapObject) {
            qDebug() << " * Repositioning with type " << contentType;
            c->reposition();
        } else
            qDebug() << " * skipping Repositioning childItem due to type " << contentType;

    }

    // b) Align my own containers

    // The children Container is empty, if there are no children branches
    // No repositioning of children required then, of course.
    if (childItems().count() == 0) return;

    switch (layout) {
        case Horizontal: {
                qreal h_max = 0;
                qreal w_total = 0;
                qreal h;
                // Calc max height and total width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    h = c->rect().height();
                    h_max = (h_max < h) ? h : h_max;
                    w_total += c->rect().width();
                }

                qreal x = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    c->setPos (x, (h_max - c->rect().height() ) / 2);
                    x += c->rect().width();
                }
                r.setWidth(w_total);
                r.setHeight(h_max);
            }
            setRect(r);
            //qDebug() << " * Horizontal layout - Setting rect of " << name << this << " to " << r;
            break;
        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;
                // Calc total height and max width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    w = c->rect().width();
                    w_max = (w_max < w) ? w : w_max;
                    h_total += c->rect().height();
                }

                qreal y = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // align centered: 
                    // c->setPos ( (w_max - c->rect().width() ) / 2, y);

                    // Align to left
                    c->setPos (0, y);
                    y += c->rect().height();
                }
                r.setWidth(w_max);
                r.setHeight(h_total);
            }
            setRect(r);
            //qDebug() << " * Vertical layout - Setting rect of " << name << this << " to " << r;
            break;
        default:
            break;
    }
}

void Container::setName(const QString &n) {
    // qDebug() << "Container::setName = " <<n;    // FIXME-1 testing
    name = n;
}

QString Container::getName() {
    // qDebug() << "Container::getName = " << name;// FIXME-1 testing
    return name;
}
