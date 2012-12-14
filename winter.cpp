#include "winter.h"

#include <QDebug>
#include <QPen>

SnowFlake::SnowFlake(QGraphicsScene *scene)
{
    setRect(qrand()%1000-500,0,20,20);
    setPen(QPen(Qt::white));
    scene->addItem(this);
}

Winter::Winter(QGraphicsScene *scene)
{
    qDebug()<<"Constr. Winter";
    mapScene=scene;
    maxFlakes=3000;
    makeSnow();
    animTimer = new QTimer;
    connect (animTimer, SIGNAL (timeout()), this, SLOT (animate() ));
    animTimer->start(100);

    snowTimer = new QTimer;
    connect (snowTimer, SIGNAL (timeout()), this, SLOT (makeSnow() ));
    snowTimer->start(1000);
}

Winter::~Winter()
{
    qDebug()<<"Destr. Winter";
    delete animTimer;
    delete snowTimer;
}

void Winter::animate()
{
    QPointF p;
    foreach (SnowFlake *flake, snowflakes)
    {
        p=flake->pos();
        p.setY(p.y()+3);
        flake->setPos(p);
    }
}

void Winter::makeSnow()
{
    if (snowflakes.count()<maxFlakes)
    {
        SnowFlake *snowflake;
        for (int i=0; i<30; i++)
        {
            snowflake=new SnowFlake(mapScene);
            snowflakes.append(snowflake);
        }
    }
}

