#ifndef WINTER_H
#define WINTER_H

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QObject>
#include <QTimer>
#include <QRectF>

class SnowFlake:public QGraphicsEllipseItem
{
public:
    SnowFlake(QGraphicsScene *scene);
    ~SnowFlake();
    void animate();
    void blow(const QPointF &v);
private:
    int size;
    QPointF dv;
    QPointF dblow;
};

class Winter:public QObject {
    Q_OBJECT
public:
    Winter(QGraphicsScene *scene);
    ~Winter();
    void setObstacles(QList <QRectF> obslist);

public slots:
    void animate();
    void makeSnow();

private:
    QGraphicsScene *mapScene;
    QList <SnowFlake*> fallingSnow;
    QList <SnowFlake*> frozenSnow;
    int maxFlakes;
    QTimer *animTimer;
    QTimer *snowTimer;
    QList <QRectF> obstacles;

    QRectF viewRect;
};

#endif
