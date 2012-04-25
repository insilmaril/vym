#include "misc.h"

#include "geometry.h"
#include <math.h>
//#include <qregexp.h>
#include <stdlib.h>

#include <QDebug>
#include <QString>

QString qpointToString (const QPoint &p)
{ return "(" + QString("%1").arg(p.x()) +","+ QString ("%1").arg (p.y()) +")"; }

QString qpointFToString (const QPointF &p)
{ return "(" + QString("%1").arg(p.x()) +","+ QString ("%1").arg (p.y()) +")"; }

QString VectorToString (const Vector &p)
{ return "(" + QString("%1").arg(p.x()) +","+ QString ("%1").arg (p.y()) +")"; }

ostream &operator<< (ostream &stream, QPoint const &p)
{ 
    stream << "("<<p.x()<<","<<p.y()<<")";
    return stream;
}

ostream &operator<< (ostream &stream, QPointF const &p)
{ 
    stream << "("<<p.x()<<","<<p.y()<<")";
    return stream;
}

ostream &operator<< (ostream &stream, QRectF const &r)
{ 
    stream << "tL="<<r.topLeft()<<" - (w,h)="<<r.width()<<","<<r.height()<<"  bR="<<r.bottomRight();
    return stream;
}

ostream &operator<< (ostream &stream, Vector const &p)
{ 
    stream << "("<<p.x()<<","<<p.y()<<")";
    return stream;
}

qreal getAngle(const QPointF &p)
{   
    // Calculate angle of vector to x-axis
    if (p.x()==0)
    {
	if (p.y()>=0)
	    return M_PI_2;
	else
	    return 3* M_PI_2;
    } else
    {
	if (p.x()>0) 
	{
	    if (p.y()<0)
		return (qreal)( - atan ( (qreal)(p.y()) / (qreal)(p.x()) ) );
	    else	
		return (qreal)( 2*M_PI - atan ( (qreal)(p.y()) / (qreal)(p.x()) ) );
	}    
	else	
	    return (qreal)(M_PI -atan ( (qreal)(p.y()) / (qreal)(p.x()) ) );
    }	
    /*
    // Calculate angle of vector to y-axis
    if (p.y()==0)
    {
	if (p.x()>=0)
	    return M_PI_2;
	else
	    return 3* M_PI_2;
    } else
    {
	if (p.y()>0) 
	    return (qreal)(M_PI  - atan ( (qreal)(p.x()) / (qreal)(p.y()) ) );
	else	
	    if (p.x()<0)
		return (qreal)( 2*M_PI - atan ( (qreal)(p.x()) / (qreal)(p.y()) ) );
	    else    
		return (qreal)( - atan ( (qreal)(p.x()) / (qreal)(p.y()) ) );
    }	
    */
}

qreal min(qreal a, qreal b)
{
    if (a<b)
	return a;
    return b;
}

qreal max(qreal a, qreal b)
{
    if (a>b)
	return a;
    return b;
}

qreal roof (qreal x)
{
    if (x<=0.5)
	return  x;
    else
	return 1-x;
}

Qt::PenStyle penStyle (const QString &s, bool &ok)
{
    ok=true;
    Qt::PenStyle p(Qt::NoPen);
    if (s=="Qt::NoPen")
	p=Qt::SolidLine;
    if (s=="Qt::SolidLine")
	p=Qt::SolidLine;
    else if (s=="Qt::DashLine")
	p=Qt::DashLine;
    else if (s=="Qt::DotLine")
	p=Qt::DotLine;
    else if (s=="Qt::DashDotLine")
	p=Qt::DashDotLine;
    else if (s=="Qt::DashDotDotLine")
	p=Qt::DashDotDotLine;
    else
    {
	qWarning()<<"misc.cpp penStyle - Unknown style s="<<s;
	ok=false;
    }
    return p;
}

QString penStyleToString (Qt::PenStyle p) 
{
    switch (p)
    {
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

QPointF point (const QString &s, bool &ok)
{
    ok=true;
    bool okx, oky;
    qreal x=s.section (',',0,0).toFloat (&okx);
    qreal y=s.section (',',1,1).toFloat (&oky);
    if (okx && oky) 
	return QPointF (x,y);
    else	
	qWarning()<<"misc.cpp Couldn't create QPointF from "<<s<<"  ok="<<okx<<","<<oky;
	ok=false;
	return QPointF();
}
   
QString pointToString (const QPointF &p)
{
    return QString("%1,%2").arg(p.x()).arg(p.y());
}

