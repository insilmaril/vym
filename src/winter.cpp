#include "winter.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>

#include "mapobj.h"
#include "misc.h"

SnowFlake::SnowFlake(QGraphicsScene *scene, SnowType t)
{
    type = t;

    size = qrand() % 10 + 3;
    QPen p(Qt::white);
    dv = QPointF(qrand() % 10 / 10.0 - 0.5, qrand() % 10 / 10.0 + 1);

    switch (type) {
        case Smilla: {
            int s4 = size / 4;
            int s3 = size / 3;
            int s6 = size / 6;

            for (int a = 0; a < 6; a++) {
                lines.append(scene->addLine(0, -s6, 0, -size));
                lines.last()->setRotation(a * 60);

                lines.append(scene->addLine(-s4, -size + s6, 0, -size + s3));
                lines.last()->setRotation(a * 60);

                lines.append(scene->addLine(s4, -size + s6, 0, -size + s3));
                lines.last()->setRotation(a * 60);
            }

            foreach (QGraphicsLineItem *l, lines) {
                l->setZValue(1000);
                l->setPen(p);
                l->setParentItem(this);
                l->setZValue(Z_SNOW);
            }
            da = qrand() % 20 / 10.0 - 1;
        }
            setRotation(qrand() % 60);
            break;
        case Disc:
            disc = scene->addEllipse(0, 0, size, size, p);
            disc->setParentItem(this);
            disc->setBrush(Qt::white);
            disc->setZValue(Z_SNOW);
            break;
        case Egg:
            disc = scene->addEllipse(0, 0, size, size * 1.5, p);
            disc->setParentItem(this);
            disc->setBrush(QColor( 
                qrand() % 100 + 150, qrand() % 100 + 150, qrand() % 100 + 150, 255));
            disc->setZValue(Z_SNOW);
            break;
        default:
            break;
    }
}

SnowFlake::~SnowFlake()
{
    // qDebug()<<"Destr. SnowFlake";
    switch (type) {
        case (Smilla):
            while (lines.isEmpty())
                delete lines.takeFirst();
            break;
        case Egg:
            delete disc;
            break;
        case Disc:
            delete disc;
            break;
        default:
            break;
    }
}

QRectF SnowFlake::boundingRect() const
{
    return QRectF(-size, -size, size * 2, size * 2);
}

void SnowFlake::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

void SnowFlake::animate()
{
    moveBy(dv.x() + dblow.x(), dv.y() + dblow.y());

    if (type == SnowFlake::Smilla)
        setRotation(rotation() + da);

    dblow = dblow * 0.9;
}

void SnowFlake::blow(const QPointF &v) { dblow = v; }

Winter::Winter(QGraphicsView *v)
{
    view = v;

    updateView();
    /*
    test = view->scene()->addRect(
            QRectF(viewRect.topLeft(), viewRect.bottomRight()),
            QPen(Qt::blue) );
    */

    type = SnowFlake::Egg;

    switch (type) {
        case SnowFlake::Smilla:
            maxFlakes = 1500;
            maxFalling = 140;
            maxUnfreeze = 50;
            break;
        case SnowFlake::Egg:
            maxFlakes = 500;
            maxFalling = 150;
            maxUnfreeze = 50;
            break;
        default:
            maxFlakes = 6500;
            maxFalling = 850;
            maxUnfreeze = 50;
            break;
    }

    makeSnow();

    animTimer = new QTimer;
    connect(animTimer, SIGNAL(timeout()), this, SLOT(animate()));
    animTimer->start(50);

    snowTimer = new QTimer;
    connect(snowTimer, SIGNAL(timeout()), this, SLOT(makeSnow()));
    snowTimer->start(1000);
}

Winter::~Winter()
{
    delete animTimer;
    delete snowTimer;
    while (!fallingSnow.isEmpty())
        delete fallingSnow.takeFirst();
    while (!frozenSnow.isEmpty())
        delete frozenSnow.takeFirst();
}

void Winter::updateView()
{
    QPointF topLeft = view->mapToScene(0, 0);
    QPointF topRight = view->mapToScene(view->rect().width(), 0);
    QPointF botLeft = view->mapToScene(0, view->rect().height());
    QPointF botRight =
        view->mapToScene(view->rect().width(), view->rect().height());

    QPointF p0;
    QPointF p1;

    topLeft.y() < topRight.y() ? p0.setY(topLeft.y()) : p0.setY(topRight.y());
    topLeft.x() < topRight.x() ? p0.setX(topLeft.x()) : p0.setX(topRight.x());

    botLeft.y() > botRight.y() ? p1.setY(botLeft.y()) : p1.setY(botRight.y());
    botLeft.x() > botRight.x() ? p1.setX(botLeft.x()) : p1.setX(botRight.x());

    viewRect = QRectF(p0, p1);
}

void Winter::setObstacles(QList<QRectF> obslist)
{
    obstacles = obslist;

    QList<SnowFlake *> unfreeze;

    // Find frozen snowflakes, which are free again
    QPointF p;
    int i = 0;
    bool frozen;
    while (i < frozenSnow.count()) {
        p = frozenSnow.at(i)->pos();
        frozen = false;

        int j = 0;
        while (j < obstacles.count() && !frozen)

        {
            if (obstacles.at(j).contains(p))
                frozen = true;
            j++;
        }
        if (!frozen) {
            unfreeze.append(frozenSnow.at(i));
            frozenSnow.removeAt(i);
        }
        else
            i++;
    }

    // Remove some flakes, if too many
    while (fallingSnow.count() + unfreeze.count() > maxFalling + maxUnfreeze)
        delete unfreeze.takeFirst();

    while (!unfreeze.isEmpty()) {
        // Blow a bit up
        unfreeze.first()->blow(
            QPointF(qrand() % 10 / 10.0 - 0.5, qrand() % 10 / 10.0 - 5));
        fallingSnow.append(unfreeze.takeFirst());
    }
}

void Winter::animate()
{
    // test->setRect(QRectF(viewRect.topLeft(), viewRect.bottomRight()));

    QPointF p;
    int i = 0;
    bool cont;
    while (i < fallingSnow.count()) {
        p = fallingSnow.at(i)->pos();
        cont = true;

        int j = 0;
        while (j < obstacles.count() && cont) {
            if (obstacles.at(j).contains(p) &&
                qrand() % (obstacles.count() + 1) > obstacles.count() - 1) {
                // Freeze snowflake on obstacle
                // Probality is equale for obstacles or falling through
                frozenSnow.append(fallingSnow.at(i));
                fallingSnow.removeAt(i);
                cont = false;
            }
            j++;
        }
        if (cont && p.y() > viewRect.bottomRight().y() + 20)

        {
            delete fallingSnow.takeAt(i);
            cont = false;
        }
        // Let snowflake fall further
        if (cont)
            fallingSnow.at(i)->animate();
        i++;
    }
}

void Winter::makeSnow()
{
    // qDebug()<<"falling: "<<fallingSnow.count()<<"  frozen:
    // "<<frozenSnow.count();
    if (fallingSnow.count() + frozenSnow.count() < maxFlakes) {
        if (fallingSnow.count() < maxFalling) {
            // Create more snowflakes
            SnowFlake *snowflake;
            for (int i = 0; i < 10; i++) {
                snowflake = new SnowFlake(view->scene(), type);
                view->scene()->addItem(snowflake);
                snowflake->setPos(rand() % round_int(viewRect.width()) +
                                      viewRect.x(),
                                  viewRect.y() - 20);
                fallingSnow.append(snowflake);
            }
        }
    }
    else {
        // Remove some of the existing frozen flakes
        for (int i = 0; i < 10; i++) {
            if (frozenSnow.count() > 0) {
                int j = qrand() % frozenSnow.count();
                delete frozenSnow.takeAt(j);
            }
        }
    }
}
