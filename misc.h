#ifndef MISC_H
#define MISC_H

#include <Qt>
#include <iostream>
using namespace std;

class QString;
class QPoint;
class QPointF;
class QRectF;
class Vector;

/////////////////////////////////////////////////////////////////////////////
QString qpointToString (const QPoint &p);
QString qpointFToString (const QPointF &p);
QString VectorToString (const Vector &p);


extern ostream &operator<< (ostream &stream, QPoint const &p);
extern ostream &operator<< (ostream &stream, QPointF const &p);
extern ostream &operator<< (ostream &stream, QRectF const &r);
extern ostream &operator<< (ostream &stream, Vector const &p);
qreal getAngle(const QPointF &);
qreal min (qreal,qreal);
qreal max (qreal,qreal);
qreal roof (qreal x);    

Qt::PenStyle penStyle (const QString &, bool &ok);
QString penStyleToString (Qt::PenStyle);
QPointF point (const QString &s, bool &ok);
QString pointToString (const QPointF &p);

#endif
