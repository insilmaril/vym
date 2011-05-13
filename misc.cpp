#include "misc.h"

#include "geometry.h"
#include <math.h>
//#include <qregexp.h>
#include <stdlib.h>

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

