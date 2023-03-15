#include "animpoint.h"

#include <math.h>

AnimPoint::AnimPoint() { init(); }

void AnimPoint::operator=(const AnimPoint &other) { copy(other); }

void AnimPoint::operator=(const QPointF &other)
{
    init();
    setX(other.x());
    setY(other.x());
}

bool AnimPoint::operator==(const QPointF &other)
{
    QPointF p(x(), y());
    return p == other;
}

bool AnimPoint::operator==(AnimPoint other)
{
    if (rx() != other.rx())
        return false;
    if (ry() != other.ry())
        return false;
    if (startPos != other.startPos)
        return false;
    if (destPos != other.destPos)
        return false;
    if (animated != other.animated)
        return false;

    return true;
}

void AnimPoint::init()
{
    animated = false;
    n = 0;
    startPos = QPointF(0, 0);
    destPos = QPointF(0, 0);
    vector = QPointF(0, 0);
    animTicks = 10;
}

void AnimPoint::copy(AnimPoint other)
{
    setX(other.x());
    setY(other.x());
    startPos = other.startPos;
    destPos = other.destPos;
    vector = other.vector;
    animated = other.animated;
    n = other.n;
    animTicks = other.animTicks;
}

void AnimPoint::setStart(const QPointF &p)
{
    startPos = p;
    setX(startPos.x());
    setY(startPos.y());
    initVector();
}

QPointF AnimPoint::getStart() { return startPos; }

void AnimPoint::setDest(const QPointF &p)
{
    destPos = p;
    initVector();
}

QPointF AnimPoint::getDest() { return destPos; }

void AnimPoint::setTicks(const uint &t) { animTicks = t; }

uint AnimPoint::getTicks() { return (uint)animTicks; }

void AnimPoint::setAnimated(bool b)
{
    animated = b;
    if (b)
        n = 0;
}

bool AnimPoint::isAnimated() { return animated; }

bool AnimPoint::animate()
{
    if (!animated)
        return false;
    // Some math to slow down the movement in the end
    qreal f = 1 - n / (qreal)animTicks;
    qreal ff = 1 - f * f * f;
    setX(startPos.x() + vector.x() * ff);
    setY(startPos.y() + vector.y() * ff);

    n++;
    if (n > animTicks) {
        vector = QPointF(0, 0);
        animated = false;
        setX(destPos.x());
        setY(destPos.y());
        return false;
    }

    return animated;
}

void AnimPoint::stop()
{
    animated = false;
    setX(destPos.x());
    setY(destPos.y());
}

void AnimPoint::initVector()
{
    vector.setX(destPos.x() - startPos.x());
    vector.setY(destPos.y() - startPos.y());
}
