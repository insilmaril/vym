#ifndef ANIMPOINT_H
#define ANIMPOINT_H

#include <QPointF>

class AnimPoint: public QPointF
{
public:
    AnimPoint();
    void operator= ( const AnimPoint & );
    void operator= ( const QPointF & );
    bool operator== ( const QPointF & );
    bool operator== ( AnimPoint  );
    void init();
    void copy(AnimPoint other);
    void setStart (const QPointF &);
    QPointF getStart();
    void setDest (const QPointF &);
    QPointF getDest();
    void setTicks (const uint &t);
    uint getTicks();
    void setAnimated(bool);
    bool isAnimated ();
    bool animate();
    void stop();

private:
    void initVector();

    QPointF startPos;
    QPointF destPos;
    QPointF vector;
    qreal n;
    uint animTicks;
    bool animated;

};

#endif
