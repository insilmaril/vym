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
    //qDebug() << "* Destr Container" << name << this;
}

void Container::init()
{
    type = Undefined;

    layout = Horizontal;

    show();
}

Container::ContainerType Container::containerType()
{
    return type;
}

void Container::setType(const Container::ContainerType &t)
{
    type = t;
}

void Container::setName(const QString &n)   // FIXME-3 debugging only
{
    name = n;
}

QString Container::getName()    // FIXME-3 debugging only
{
    QString t;
    switch (type) {
        case Undefined:
            t = "Undefined";
            break;
        case TmpParent:
            t = "TmpParent";
            break;
        case FloatingContent:
            t = "FloatingContent";
            break;
        case InnerContent:
            t = "InnerContent";
            break;
        case Children:
            t = "Children";
            break;
        case Branch:
            t = "Branch";
            break;
        case Heading:
            t = "Heading";
            break;
        default:
            t = "Unknown";
            break;
    }
    return QString("[%1]").arg(t);
}

QString Container::info (const QString &prefix)
{
    return prefix +
        getName() +
        QString(" scenePos: (%1, %2)").arg(scenePos().x()).arg(scenePos().y()) + 
        QString(" pos: (%1, %2)").arg(pos().x()).arg(pos().y()) +
        QString(" Layout: %1").arg(layout);
}

void Container::setLayoutType(const LayoutType &ltype)
{
    layout = ltype;
}

Container::LayoutType Container::getLayoutType()
{
    return layout;
}

void Container::setHorizontalDirection(const HorizontalDirection &hdir)
{
    horizontalDirection = hdir;
}

Container::HorizontalDirection Container::getHorizontalDirection()
{
    return horizontalDirection;
}

void Container::setVerticalAlignment(const VerticalAlignment &a)
{
    verticalAlignment = a;
}

void Container::addContainer(Container *c)
{
    c->setParentItem(this);
}

QVariant Container::itemChange(GraphicsItemChange change, const QVariant &value)
{
    //qDebug() << "Container::itemChange of " << this << ": " << change << value;
    /*
    if (change == ItemPositionChange && scene()) {
        // value is the new position.
        QPointF newPos = value.toPointF();
        QRectF rect = scene()->sceneRect();
        if (!rect.contains(newPos)) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            return newPos;
        }
    }
    */
    return  QGraphicsItem::itemChange(change, value);
}

void Container::setOrgPos()
{
    originalPos = pos();
}

QPointF Container::orgPos()
{
    return originalPos;
}

Container* Container::parentContainer() 
{
    return (Container*)parentItem();
}
void Container::reposition()
{
    qDebug() << QString("Container::reposition of %1 container").arg(info()) << this;

    QRectF r;

    // Repositioning is done recursively: 
    // First the size sizes of subcontainers are calculated, 
    // Container::reposition is overloaded, so for example HeadingContainer 
    // will return correct size of heading!
    //
    // Then the subcontainers are positioned.
    //
    // a) calc sizes of subcontainers based on their layouts 
    //    (overloaded, not there e.g. for HeadingContainer!)

    if (childItems().count() == 0 && layout != BFloat) {   // FIXME-0 check!
        qDebug() << " * Setting r to minimal size. r =" << rect();
        r.setWidth(0);
        r.setHeight(0);
        setRect(r);
        //return;
    }

    // FIXME-2 testing only, rotate children
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
    ct = QPointF (0, 0);
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
//        if (c->type != Undefined && c->type != Heading) {
            c->reposition();

    /*
            // If child container has floating layout, consider it's translation 
            // vector for my own children
            if (c->getLayoutType() == BFloat)
            {
                qDebug() << " # Adding floating ct= " << c->ct << " of " << c->getName() << " to " << getName();
                ct += c->ct;
            }
    */
//        }
    }

    // b) Align my own containers

    // The children Container is empty, if there are no children branches
    // No repositioning of children required then, of course.
    if (childItems().count() == 0) 
    {
        qDebug() << " * no children, could return?";
        //return;  // FIXME-0 still required to continue, if children are BoundedFloat
    }

    switch (layout) {
        case Floating: 
                qDebug() << " * Layout Floating begin of " << info() << "Children.count= " << childItems().count() << this ;

                {
                    /////////////////////////////////////////////////// fix from here
                    // Calc bbox of all children to prepare calculating rect()

                    qreal x_min, y_min, x_max, y_max;

                    foreach (QGraphicsItem *child, childItems()) {
                        c = (Container*) child;

                        x_min = c->pos().x();
                        y_min = c->pos().y();

                        x_max = x_min + c->rect().width();
                        y_max = y_min + c->rect().height();

                        if (c->pos().x() < x_min ) x_min = c->pos().x();  
                        if (c->pos().y() < y_min ) y_min = c->pos().y();  

                        if (c->pos().x() + c->rect().width() > x_max)
                            x_max = c->pos().x() + c->rect().width();
                        if (c->pos().y() + c->rect().height() > y_max)
                            y_max = c->pos().y() + c->rect().height();

                        qDebug() << "   - c: " << c->info() << 
                            "  c->r=" << c->rect() << 
                            " ( " << x_min <<", " << y_min << ") " <<
                            " ( " << x_max <<", " << y_max << ") " ;
                    }
                    
                /*
                QPointF topLeft = c->pos();
                QPointF bottomRight = c->pos();
                
                if (childItems().count() > 0) {
                    int n = 0;

                    c = (Container*) (childItems().at(n));
                    QPointF(
                        c->pos().x() + c->rect().width(),
                        c->pos().y() + c->rect().height() );

                    qDebug() << "   - c: " << c->getName() << 
                        "  pos=" << c->pos() << "  r=" << c->rect() << " n=" << n;

                    while (n < childItems().count() - 1) {
                        n++;
                        qDebug() << "   - c: " << c->getName() << 
                            "  pos=" << c->pos() << "  r=" << c->rect() << " n=" << n;

                        if (c->pos().x() < topLeft.x())  topLeft.setX(c->pos().x());
                        if (c->pos().y() < topLeft.y())  topLeft.setY(c->pos().y());

                        // FIXME-0 now expand bottomRight corner as needed
                    }
                }
                */
                /////////////////////////////////////////////////// fix to here

                /*
                qreal x_min, y_min = 0;
                qreal x_max, y_max = 0;
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    qDebug() << "   - c: " << c->getName() << "  pos=" << c->pos() << "  r=" << c->rect();

                    if (c->pos().x() < x_min ) x_min = c->pos().x();  
                    if (c->pos().y() < y_min ) y_min = c->pos().y();  

                    if (c->pos().x() + c->rect().width() > x_max)
                        x_max = c->pos().x() + c->rect().width();
                    if (c->pos().y() + c->rect().height() > y_max)
                        y_max = c->pos().y() + c->rect().height();
                }

                if (x_min < 0 ) ct.setX( -x_min);
                if (y_min < 0 ) ct.setY( -y_min);

                r.setRect(0, 0, x_max - x_min, y_max - y_min);
                */

                r.setTopLeft(QPointF(x_min, y_min));
                r.setBottomRight(QPointF(x_max, y_max));

                setRect(r);
                //if (r.topLeft().x() < 0) ct.setX( -r.topLeft().x());
                //if (r.topLeft().y() < 0) ct.setY( -r.topLeft().y());
                qDebug() << " * Layout Floating end of " << info() << " r=" << r << " ct=" << ct;
            }
            break;
        case Horizontal: {
                //qDebug() << " * Layout Horizontal begin of " << info() << " r=" << r << " ct=" << ct << " horDir=" << horizontalDirection;
                qreal h_max = 0;
                qreal w_total = 0;
                qreal h;
                // Calc max height and total width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    h = c->rect().height();
                    h_max = (h_max < h) ? h : h_max;
                    w_total += c->rect().width();
                    //qDebug() << "    - h_max=" << h_max << "h=" << h << " c:" << c->info();;
                }

                qreal x;
                horizontalDirection == LeftToRight ? x = 0 : x = w_total;

                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    // Order from left to right
                    if (horizontalDirection == LeftToRight)
                    {
                        c->setPos (x, (h_max - c->rect().height() ) / 2);
                        x += c->rect().width();
                    } else
                    {
                        c->setPos (x - c->rect().width(), (h_max - c->rect().height() ) / 2);
                        x -= c->rect().width();
                    }
                    /*
                    if (c->getLayoutType () != BFloat) {
                        qDebug() << "    - Translating by ct=" << ct;
                        c->moveBy(ct.x(), ct.y());
                    }
                    qDebug() << "    - setting position:  " << c->getName() << "  pos=" << c->pos() << "  ct=" << ct;
                    */
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

                // child is aligned further down, just go with sum and max
                // Calc total height and max width for stacked children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // child is aligned further down, just go with sum and max
                    w = c->rect().width();
                    w_max = (w_max < w) ? w : w_max;    // FIXME-2 use max function
                    h_total += c->rect().height();
                }

                qreal y = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    switch (verticalAlignment) {
                        case Left:
                            c->setPos (0, y);
                            break;
                        case Right:
                            c->setPos (w_max - c->rect().width(), y);
                            break;
                        case Centered:
                            c->setPos ( (w_max - c->rect().width() ) / 2, y);
                            break;
                    }

                    y += c->rect().height();
                }

                r.setWidth(w_max);
                r.setHeight(h_total);
            }
            setRect(r);
            break;
        case BFloat: {
                qDebug() << " * Layout BFloat begin of " << info() << "Children.count= " << childItems().count() << this ;


                /////////////////////////////////////////////////// fix from here
                // Calc bbox of all children to prepare calculating rect()

                qreal x_min, y_min, x_max, y_max;

                if (childItems().count() > 0) {
                    int n = 0;

                    c = (Container*) (childItems().at(n));

                    x_min = c->pos().x();
                    y_min = c->pos().y();

                    x_max = x_min + c->rect().width();
                    y_max = y_min + c->rect().height();

                    qDebug() << "   - c: " << c->info() << 
                        "  c->r=" << c->rect() << " n=" << n <<
                        " ( " << x_min <<", " << y_min << ") " <<
                        " ( " << x_max <<", " << y_max << ") " ;
                }
                    
                /*
                QPointF topLeft = c->pos();
                QPointF bottomRight = c->pos();
                
                if (childItems().count() > 0) {
                    int n = 0;

                    c = (Container*) (childItems().at(n));
                    QPointF(
                        c->pos().x() + c->rect().width(),
                        c->pos().y() + c->rect().height() );

                    qDebug() << "   - c: " << c->getName() << 
                        "  pos=" << c->pos() << "  r=" << c->rect() << " n=" << n;

                    while (n < childItems().count() - 1) {
                        n++;
                        qDebug() << "   - c: " << c->getName() << 
                            "  pos=" << c->pos() << "  r=" << c->rect() << " n=" << n;

                        if (c->pos().x() < topLeft.x())  topLeft.setX(c->pos().x());
                        if (c->pos().y() < topLeft.y())  topLeft.setY(c->pos().y());

                        // FIXME-0 now expand bottomRight corner as needed
                    }
                }
                */
                /////////////////////////////////////////////////// fix to here

                /*
                qreal x_min, y_min = 0;
                qreal x_max, y_max = 0;
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    qDebug() << "   - c: " << c->getName() << "  pos=" << c->pos() << "  r=" << c->rect();

                    if (c->pos().x() < x_min ) x_min = c->pos().x();  
                    if (c->pos().y() < y_min ) y_min = c->pos().y();  

                    if (c->pos().x() + c->rect().width() > x_max)
                        x_max = c->pos().x() + c->rect().width();
                    if (c->pos().y() + c->rect().height() > y_max)
                        y_max = c->pos().y() + c->rect().height();
                }

                if (x_min < 0 ) ct.setX( -x_min);
                if (y_min < 0 ) ct.setY( -y_min);

                r.setRect(0, 0, x_max - x_min, y_max - y_min);
                */

                r.setTopLeft(QPointF(x_min, y_min));
                r.setBottomRight(QPointF(x_max, y_max));

                setRect(r);
                if (r.topLeft().x() < 0) ct.setX( -r.topLeft().x());
                if (r.topLeft().y() < 0) ct.setY( -r.topLeft().y());
                qDebug() << " * Layout BFloat end of " << info() << " r=" << r << " ct=" << ct;
            }
            break;
        default:
            qWarning() << "Container::reposition  unknown layout type!";
            break;
    }

    if (layout != BFloat)
    {
        //qDebug() << " # layout != BFloat for " << info() << " ct=" << ct;
        //setPos (ct);
    }
}
