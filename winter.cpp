#include "winter.h"

#include "misc.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>

SnowFlake::SnowFlake(QGraphicsScene *scene)
{
    size=qrand()%10+3;

    int s4=size/4;
    int s3=size/3;
    int s6=size/6;

    QGraphicsLineItem *l;
    for (int a=0; a<6; a++)
    {
        lines.append(scene->addLine(0, -s6, 0, -size));
        lines.last()->setRotation(a*60);

        lines.append(scene->addLine(-s4, -size + s6, 0, -size + s3));
        lines.last()->setRotation(a*60);

        lines.append(scene->addLine( s4, -size + s6, 0, -size + s3));
        lines.last()->setRotation(a*60);
    }

    QPen p(Qt::white);
    foreach (QGraphicsLineItem *l, lines)
    {
        l->setZValue(1000);
        l->setPen(p);
        l->setParentItem(this);
    }
    dv=QPointF(qrand()%10/10.0-0.5, qrand()%10/10.0 +1);
    da=qrand()%20 / 10.0 - 1;
}

SnowFlake::~SnowFlake()
{
    //qDebug()<<"Destr. SnowFlake";
    while(lines.isEmpty())
        delete lines.takeFirst();
}

QRectF SnowFlake::boundingRect() const 
{
    return QRectF (-size, -size, size*2, size*2);
}

void SnowFlake::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
}

void SnowFlake::animate()
{
    moveBy(dv.x() + dblow.x(), dv.y() + dblow.y());
    setRotation(rotation() + da);
    dblow = dblow *0.9;
}

void SnowFlake::blow(const QPointF &v)
{
    dblow=v;
}

Winter::Winter(QGraphicsView *v)
{
    view=v;

    updateView();
    /*
    test=view->scene()->addLine(
            QLineF(viewRect.topLeft(), viewRect.bottomRight()), 
            QPen(Qt::blue) );
    */

    maxFlakes=1500;
    maxFalling=140;
    maxUnfreeze=30;

    makeSnow();

    animTimer = new QTimer;
    connect (animTimer, SIGNAL (timeout()), this, SLOT (animate() ));
    animTimer->start(50);

    snowTimer = new QTimer;
    connect (snowTimer, SIGNAL (timeout()), this, SLOT (makeSnow() ));
    snowTimer->start(1000);
}

Winter::~Winter()
{
    delete animTimer;
    delete snowTimer;
}

void Winter::updateView()
{
    QPointF p0=view->mapToScene( QPoint(0,0));
    QPointF p1=view->mapToScene( view->rect().width(), view->rect().height() );
    viewRect=QRectF(p0,p1);
}

void Winter::setObstacles(QList <QRectF> obslist)
{
    obstacles=obslist;

    QList <SnowFlake*> unfreeze;

    // Find frozen snowflakes, which are free again
    QPointF p;
    int i=0;
    bool frozen;
    while (i < frozenSnow.count())
    {
        p=frozenSnow.at(i)->pos();
        frozen=false;
        
        int j=0;
        while (j<obstacles.count() && !frozen)

        {
            if (obstacles.at(j).contains(p) )
                frozen=true;
            j++;
        }
        if (!frozen)
        {
            unfreeze.append(frozenSnow.at(i));
            frozenSnow.removeAt(i);
        } else
            i++;
    }
    
    // Remove some flakes, if too many
    while (fallingSnow.count() + unfreeze.count() > maxFalling + maxUnfreeze)
        delete unfreeze.takeFirst();

    while (!unfreeze.isEmpty())
    {
        // Blow a bit up
        unfreeze.first()->blow( QPointF(qrand()%10/10.0-0.5, qrand()%10/10.0 -5));
        fallingSnow.append(unfreeze.takeFirst());
    }
}


void Winter::animate()
{
    updateView();
    //test->setLine(QLineF(viewRect.topLeft(), viewRect.bottomRight())); 

    QPointF p;
    int i=0;
    bool cont;
    while (i<fallingSnow.count())
    {
        p=fallingSnow.at(i)->pos();
        cont=true;
        
        int j=0;
        while (j<obstacles.count() && cont)
        {
            if (obstacles.at(j).contains(p) && qrand()%(obstacles.count()+1) > obstacles.count()-1)
            {
                // Freeze snowflake on obstacle
                // Probality is equale for obstacles or falling through
                frozenSnow.append(fallingSnow.at(i));
                fallingSnow.removeAt(i);
                cont=false;
            } 
            j++;
        }
        if (cont && p.y() > viewRect.y() + viewRect.height() + 20)
        {
            delete fallingSnow.takeAt(i);
            cont=false;
        }
        // Let snowflake fall further
        if (cont) fallingSnow.at(i)->animate();
        i++;
    }
}

void Winter::makeSnow()
{
    //qDebug()<<"falling: "<<fallingSnow.count()<<"  frozen: "<<frozenSnow.count();
    if (fallingSnow.count() + frozenSnow.count() <maxFlakes)
    {
        if (fallingSnow.count() < maxFalling)
        {
            // Create more snowflakes
            SnowFlake *snowflake;
            for (int i=0; i<10; i++)
            {
                snowflake=new SnowFlake(view->scene());
                snowflake->setPos( 0,0);
                snowflake->setRotation(qrand()%60);
                view->scene()->addItem(snowflake);
                snowflake->setPos( 
                        rand()%round_int(viewRect.width()) + viewRect.x(),
                        viewRect.y() -20
                );
                fallingSnow.append(snowflake);
            }
        }
    } else
    {
        // Remove some of the existing frozen flakes
        for (int i=0; i<10; i++)
        {
            if (frozenSnow.count()>0)
            {
                int j=qrand()%frozenSnow.count();
                delete frozenSnow.takeAt(j);
            }
        }
    }
    
}

