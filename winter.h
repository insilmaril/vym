#ifndef WINTER_H
#define WINTER_H

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QObject>
#include <QTimer>

class SnowFlake:public QGraphicsEllipseItem
{
public:
    SnowFlake(QGraphicsScene *scene);
};

class Winter:public QObject {
    Q_OBJECT
public:
    Winter(QGraphicsScene *scene);
    ~Winter();

public slots:
    void animate();
    void makeSnow();

private:
    QGraphicsScene *mapScene;
    QList <SnowFlake*> snowflakes;
    int maxFlakes;
    QTimer *animTimer;
    QTimer *snowTimer;
};

#endif
