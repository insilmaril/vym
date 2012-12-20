#ifndef WINTER_H
#define WINTER_H

#include <QObject>
#include <QGraphicsItem>
#include <QTimer>
#include <QRectF>

class QGraphicsView;
class QGraphicsScene;

class SnowFlake:public QGraphicsItem
{
public:
    SnowFlake(QGraphicsScene *scene);
    ~SnowFlake();
    QRectF boundingRect() const;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    void animate();
    void blow(const QPointF &v);
private:
    int size;
    QList <QGraphicsLineItem*> lines;
    QPointF dv;
    QPointF dblow;
    qreal da;
};

class Winter:public QObject {
    Q_OBJECT
public:
    Winter(QGraphicsView *view);
    ~Winter();
    void updateView();
    void setObstacles(QList <QRectF> obslist);

public slots:
    void animate();
    void makeSnow();

private:
    QGraphicsView *view;
    QList <SnowFlake*> fallingSnow;
    QList <SnowFlake*> frozenSnow;
    int maxFlakes;
    QTimer *animTimer;
    QTimer *snowTimer;
    QList <QRectF> obstacles;

    int maxFalling;
    int maxUnfreeze;
    //QGraphicsLineItem *test;

    QRectF viewRect;
};

#endif
