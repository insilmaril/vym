#ifndef WINTER_H
#define WINTER_H

#include <QGraphicsItem>
#include <QObject>
#include <QRectF>
#include <QTimer>

class QGraphicsView;
class QGraphicsScene;

class SnowFlake : public QGraphicsItem {
  public:
    enum SnowType { Smilla, Disc };

    SnowFlake(QGraphicsScene *scene, SnowType type);
    ~SnowFlake();
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    void animate();
    void blow(const QPointF &v);

  private:
    SnowType type;
    int size;
    QList<QGraphicsLineItem *> lines;
    QGraphicsEllipseItem *disc;
    QPointF dv;
    QPointF dblow;
    qreal da;
};

class Winter : public QObject {
    Q_OBJECT
  public:
    Winter(QGraphicsView *view);
    ~Winter();
    void updateView();
    void setObstacles(QList<QRectF> obslist);

  public slots:
    void animate();
    void makeSnow();

  private:
    QGraphicsView *view;
    QList<SnowFlake *> fallingSnow;
    QList<SnowFlake *> frozenSnow;
    int maxFlakes;
    QTimer *animTimer;
    QTimer *snowTimer;
    QList<QRectF> obstacles;

    SnowFlake::SnowType type;
    int maxFalling;
    int maxUnfreeze;
    // QGraphicsRectItem *test;

    QRectF viewRect;
};

#endif
