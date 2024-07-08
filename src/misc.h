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
class QDialog;

/////////////////////////////////////////////////////////////////////////////
QString richTextToPlain(QString r);
QString toS(const bool &);
QString toS(const qreal &r, int d = 1);
QString toS(const QPoint &p);
QString toS(const QPointF &p, int d = 1);
QString toS(const QRectF &r, int d = 1);
QString toS(const Vector &p);

QString pluralize(const QString &, qsizetype count);

extern ostream &operator<<(ostream &stream, QPoint const &p);
extern ostream &operator<<(ostream &stream, QPointF const &p);
extern ostream &operator<<(ostream &stream, QRectF const &r);
extern ostream &operator<<(ostream &stream, Vector const &p);

qreal min(qreal, qreal);
qreal max(qreal, qreal);
qreal roof(qreal x);
int round_int(qreal x);

Qt::PenStyle penStyle(const QString &, bool &ok);
QString penStyleToString(Qt::PenStyle);
QPointF point(const QString &s, bool &ok);
QString pointToString(const QPointF &p);

void centerDialog(QDialog *dia);

bool versionLowerThanVym(const QString &);
bool versionLowerOrEqualThanVym(const QString &);
bool versionLowerOrEqual(const QString &, const QString &);
#endif
