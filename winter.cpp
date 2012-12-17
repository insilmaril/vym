#include "winter.h"

#include "misc.h"

#include <QDebug>
#include <QPen>

SnowFlake::SnowFlake(QGraphicsScene *scene)
{
    size=qrand()%10+10;
    setRect(-size/2, -size/2, size, size);
    setPen(QPen(Qt::white));
    scene->addItem(this);
    dv=QPointF(qrand()%10/10.0-0.5, qrand()%10/10.0 +1);
}

SnowFlake::~SnowFlake()
{
    //qDebug()<<"Destr. SnowFlake";
}

void SnowFlake::animate()
{
    moveBy(dv.x() + dblow.x(), dv.y() + dblow.y());
    dblow = dblow *0.9;
}

void SnowFlake::blow(const QPointF &v)
{
    dblow=v;
}

Winter::Winter(QGraphicsScene *scene)
{
    mapScene=scene;

    QRectF r=mapScene->sceneRect();
    viewRect=QRectF (r.x()+50, r.y()+50, r.width()-100, r.height()-100);
    qDebug()<<"       r="<<r;
    qDebug()<<"viewRect="<<viewRect;
    QGraphicsRectItem *ri=new QGraphicsRectItem (viewRect);
    mapScene->addItem (ri);

    animTimer = new QTimer;
    connect (animTimer, SIGNAL (timeout()), this, SLOT (animate() ));
    animTimer->start(50);

    maxFlakes=2000;
    makeSnow();

    snowTimer = new QTimer;
    connect (snowTimer, SIGNAL (timeout()), this, SLOT (makeSnow() ));
    snowTimer->start(1000);

}

Winter::~Winter()
{
    delete animTimer;
    delete snowTimer;
}

void Winter::setObstacles(QList <QRectF> obslist)
{
    obstacles=obslist;

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
            // Blow a bit up
            frozenSnow.at(i)->blow( QPointF(qrand()%10/10.0-0.5, qrand()%10/10.0 -5));
            fallingSnow.append(frozenSnow.at(i));
            frozenSnow.removeAt(i);
        } else
            i++;
    }
}


void Winter::animate()
{
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
    if (fallingSnow.count()<maxFlakes)
    {
        SnowFlake *snowflake;
        for (int i=0; i<30; i++)
        {
            snowflake=new SnowFlake(mapScene);
            snowflake->setPos( 
                    rand()%round_int(viewRect.width()) + viewRect.x(),
                    viewRect.y() -20
            );
            fallingSnow.append(snowflake);
        }
    }
}

