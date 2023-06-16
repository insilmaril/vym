#include "misc.h"

#include "geometry.h"

#include <math.h>
#include <stdlib.h>

#include <QDebug>
#include <QDialog>
#include <QString>

extern QString vymVersion;

QString boolToString(const bool &b)
{
    return b ? "true" : "false";
}

QString richTextToPlain(QString r, const QString &indent, const int &width)
{
    Q_UNUSED(width);

    // Avoid failing assert with mingw
    if (r.isEmpty())
        return r;

    QRegExp rx;
    rx.setMinimal(true);

    // Remove all <style...> ...</style>
    rx.setPattern("<style.*>.*</style>");
    r.replace(rx, "");

    // convert all "<br*>" to "\n"
    rx.setPattern("<br.*>");
    r.replace(rx, "\n");

    // convert all "</p>" to "\n"
    rx.setPattern("</p>");
    r.replace(rx, "\n");

    // remove all remaining tags
    rx.setPattern("<.*>");
    r.replace(rx, "");

    // If string starts with \n now, remove it.
    // It would be wrong in an OOo export for example
    while (r.length() > 0 && r.at(0) == '\n')
        r.remove(0, 1);

    // convert "&", "<" and ">"
    rx.setPattern("&gt;");
    r.replace(rx, ">");
    rx.setPattern("&lt;");
    r.replace(rx, "<");
    rx.setPattern("&amp;");
    r.replace(rx, "&");
    rx.setPattern("&quot;");
    r.replace(rx, "\"");

    // Indent everything
    rx.setPattern("^\n");
    r.replace(rx, indent);
    r = indent + r; // Don't forget first line

    return r;
}

QString qpointToString(const QPoint &p)
{
    return QString("(%1, %2)").arg(p.x()).arg(p.y());
}

QString qpointFToString(const QPointF &p, int d)
{
    return QString("(%1, %2)")
        .arg(QString::number(p.x(),'f', d))
        .arg(QString::number(p.y(),'f', d));
}

QString qrectFToString(const QRectF &r, int d)
{
    return QString("(%1, %2  %3x%4)")
        .arg(QString::number(r.x(),'f', d))
        .arg(QString::number(r.y(),'f', d))
        .arg(QString::number(r.width(),'f', d))
        .arg(QString::number(r.height(),'f', d));
}

QString VectorToString(const Vector &p)
{
    return QString("(%1, %2)").arg(p.x()).arg(p.y());
}

ostream &operator<<(ostream &stream, QPoint const &p)
{
    stream << "(" << p.x() << ", " << p.y() << ")";
    return stream;
}

ostream &operator<<(ostream &stream, QPointF const &p)
{
    stream << "(" << p.x() << ", " << p.y() << ")";
    return stream;
}

ostream &operator<<(ostream &stream, QRectF const &r)
{
    stream << "tL=" << r.topLeft() << " - (w,h)=" << r.width() << ","
           << r.height() << "  bR=" << r.bottomRight();
    return stream;
}

ostream &operator<<(ostream &stream, Vector const &p)
{
    stream << "(" << p.x() << "," << p.y() << ")";
    return stream;
}

qreal min(qreal a, qreal b)
{
    if (a < b)
        return a;
    return b;
}

qreal max(qreal a, qreal b)
{
    if (a > b)
        return a;
    return b;
}

qreal roof(qreal x)
{
    if (x <= 0.5)
        return x;
    else
        return 1 - x;
}

int round_int(qreal x) { return (x > 0.0) ? (x + 0.5) : (x - 0.5); }

Qt::PenStyle penStyle(const QString &s, bool &ok)
{
    ok = true;
    Qt::PenStyle p(Qt::NoPen);
    if (s == "Qt::NoPen")
        p = Qt::SolidLine;
    if (s == "Qt::SolidLine")
        p = Qt::SolidLine;
    else if (s == "Qt::DashLine")
        p = Qt::DashLine;
    else if (s == "Qt::DotLine")
        p = Qt::DotLine;
    else if (s == "Qt::DashDotLine")
        p = Qt::DashDotLine;
    else if (s == "Qt::DashDotDotLine")
        p = Qt::DashDotDotLine;
    else {
        qWarning() << "misc.cpp penStyle - Unknown style s=" << s;
        ok = false;
    }
    return p;
}

QString penStyleToString(Qt::PenStyle p)
{
    switch (p) {
    case Qt::NoPen:
        return "Qt::NoPen";
    case Qt::SolidLine:
        return "Qt::SolidLine";
    case Qt::DashLine:
        return "Qt::DashLine";
    case Qt::DotLine:
        return "Qt::DotLine";
    case Qt::DashDotLine:
        return "Qt::DashDotLine";
    case Qt::DashDotDotLine:
        return "Qt::DashDotDotLine";
    default:
        return "";
    }
}

QPointF point(const QString &s, bool &ok)
{
    ok = true;
    bool okx, oky;
    qreal x = s.section(',', 0, 0).toFloat(&okx);
    qreal y = s.section(',', 1, 1).toFloat(&oky);
    if (okx && oky)
        return QPointF(x, y);
    else
        qWarning() << "misc.cpp Couldn't create QPointF from " << s
                   << "  ok=" << okx << "," << oky;
    ok = false;
    return QPointF();
}

QString pointToString(const QPointF &p)
{
    return QString("%1,%2").arg(p.x()).arg(p.y());
}

void centerDialog(QDialog *dia)
{
    dia->move(QCursor::pos() -
              0.5 * QPoint(dia->rect().width(), dia->rect().height()));
}

// #include "version.h"

// #include <QDebug>
// #include <QRegExp>

bool versionLowerThanVym(const QString &v)
{
    // returns true, if Version v <  VYM_VERSION
    if (v == vymVersion)
        return false;
    else
        return versionLowerOrEqualThanVym(v);
}

bool versionLowerOrEqualThanVym(const QString &v)
{
    // returns true, if Version v <=  VYM_VERSION
    return versionLowerOrEqual(v, vymVersion);
}

bool versionLowerOrEqual(const QString &v, const QString &vstatic)
{
    // returns true, if version v <= vstatic
    bool ok = false;
    int v1 = 0;
    int v2 = 0;
    int v3 = 0;
    int vs1 = 0;
    int vs2 = 0;
    int vs3 = 0;

    QRegExp rx("(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})");
    int pos = rx.indexIn(v);
    if (pos > -1) {
        v1 = rx.cap(1).toInt(&ok);
        if (ok)
            v2 = rx.cap(2).toInt(&ok);
        if (ok)
            v3 = rx.cap(3).toInt(&ok);
    }

    pos = rx.indexIn(vstatic);
    if (ok && pos > -1) {
        vs1 = rx.cap(1).toInt(&ok);
        if (ok)
            vs2 = rx.cap(2).toInt(&ok);
        if (ok)
            vs3 = rx.cap(3).toInt(&ok);
    }

    if (!ok) {
        qWarning() << QString(
                          "Warning: Checking version failed: v=%1  vstatic=%2")
                          .arg(v)
                          .arg(vstatic);
        return false;
    }

    if (vs1 > v1)
        return true;
    if (vs1 < v1)
        return false;
    if (vs2 > v2)
        return true;
    if (vs2 < v2)
        return false;
    if (vs3 > v3)
        return true;
    if (vs3 < v3)
        return false;
    return true;
}
