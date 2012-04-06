#include "geometry.h"

#include <math.h>
#include "misc.h"

#include <QString>

#include <iostream>
using namespace std;


QRectF addBBox(QRectF r1, QRectF r2)
{   
    // Find smallest QRectF containing given rectangles

    QRectF n;
    // Set left border
    if (r1.left() <= r2.left() )
	n.setLeft(r1.left() );
    else
	n.setLeft(r2.left() );
	
    // Set top border	    
    if (r1.top() <= r2.top() )
	n.setTop(r1.top() );
    else
	n.setTop(r2.top() );
	
    // Set right border
    if (r1.right() <= r2.right() )
	n.setRight(r2.right() );
    else
	n.setRight(r1.right() );
	
    // Set bottom 
    if (r1.bottom() <= r2.bottom() )
	n.setBottom(r2.bottom() );
    else
	n.setBottom(r1.bottom() );
    return n;
}

QSize addBBoxSize (QSize s1, QSize s2)
{   
    // Find the minimimum smallest QSize which could include 2 given sizes

    QSize s=s1;
    if (s1.width() <= s2.width() )
	s.setWidth (s2.width() );
    if (s1.height() <= s2.height() )
	s.setHeight(s2.height() ); 
    return s;
}

bool isInBox(const QPointF &p, const QRectF &box)
{
    if (p.x() >= box.left() && p.x() <= box.right()  
    && p.y() <= box.bottom() && p.y() >= box.top() )
	return true;
    return false;   
}

qreal distance (const QPointF &p, const QPointF &q)
{
    return sqrt (p.x()*q.x() + p.y()*q.y());
}

Vector::Vector ():QPointF ()
{
}

Vector::Vector (const QPointF &p):QPointF (p)
{
}

Vector::Vector (qreal x, qreal y):QPointF (x,y)
{
}

//! Check if length is 0
bool Vector::isNull()
{
    if (x()==0 && y()==0)
	return true;
    return false;   
}

//! Normalize vector
void Vector::normalize ()
{
    if (isNull() ) return;
    qreal l=sqrt ( x()*x() + y()*y() );
    setX (x()/l);
    setY (y()/l);
}

//! Dot product of two vectors
qreal Vector::dotProduct (const QPointF &b)
{
    return x()*b.x() + y()*b.y();
}


void Vector::scale (const qreal &f)
{
    setX (x()*f);
    setY (y()*f);
}

void Vector::invert ()
{
    setX (-x());
    setY (-y());
}

QPointF Vector::toQPointF ()
{
    return QPointF (x(),y());
}

/*! Calculate the projection of a polygon on an axis
    and returns it as a [min, max] interval  */
ConvexPolygon::ConvexPolygon ()
{
}

ConvexPolygon::ConvexPolygon (QPolygonF p):QPolygonF (p)
{
}

void ConvexPolygon::calcCentroid() 
{
    // Calculate area and centroid
    // http://en.wikipedia.org/wiki/Centroid
    qreal cx,cy,p;
    cx=cy=0;
    _area=0;

    append (at(0));
    for (int i=0;i<size()-1;i++)
    {
	p=at(i).x() * at(i+1).y() - at(i+1).x() * at(i).y();
	_area+=p;
	cx+=(at(i).x()+at(i+1).x()) * p;
	cy+=(at(i).y()+at(i+1).y()) * p;
    }	
    pop_back();
    // area is negative if vertices ordered counterclockwise
    // (in mirrored graphicsview!)
    _area=_area/2;  
    p=_area*6;
    _centroid.setX (cx/p);
    _centroid.setY (cy/p);
}

QPointF ConvexPolygon::centroid() const
{
    return _centroid;
}

qreal ConvexPolygon::weight() const
{
    return _area;
}

std::string ConvexPolygon::toStdString()
{
    QString s ("(");
    for (int i=0;i<size();++i)
    {
	s+=QString("(%1,%2)").arg(at(i).x()).arg(at(i).y());
	if (i<size()-1) s+=",";
    }
    s+=")"; 
    return s.toStdString();
}

Vector ConvexPolygon::at(const int &i) const
{
    return Vector (QPolygonF::at(i).x(),QPolygonF::at(i).y());
}

void ConvexPolygon::translate ( const Vector & offset )
{ translate (offset.x(),offset.y());}

void ConvexPolygon::translate ( qreal dx, qreal dy )
{
    QPolygonF::translate (dx,dy);
    _centroid=_centroid+QPointF (dx,dy);
}

void projectPolygon(Vector axis, ConvexPolygon polygon, qreal &min, qreal &max) 
{
    // To project a point on an axis use the dot product

    //qDebug() << "Projecting on "<< axis;
    qreal d = axis.dotProduct(polygon.at(0));
    min = d;
    max = d;
    for (int i = 0; i < polygon.size(); i++) 
    {
        d= polygon.at(i).dotProduct (axis);
        if (d < min) 
            min = d;
        else 
            if (d> max) max = d;
    //	qDebug() << "p="<<polygon.at(i)<<"  d="<<d<<"  (min, max)=("<<min<<","<<max<<")";	
    }
}

// Calculate the signed distance between [minA, maxA] and [minB, maxB]
// The distance will be negative if the intervals overlap

qreal intervalDistance(qreal minA, qreal maxA, qreal minB, qreal maxB) {
    if (minA < minB) {
        return minB - maxA;
    } else {
        return minA - maxB;
    }
}

/*!
 Check if polygon A is going to collide with polygon B.
 The last parameter is the *relative* velocity 
 of the polygons (i.e. velocityA - velocityB)
*/

PolygonCollisionResult polygonCollision(ConvexPolygon polygonA, 
                              ConvexPolygon polygonB, Vector velocity) 
{
    PolygonCollisionResult result;
    result.intersect = true;
    result.willIntersect = true;

    int edgeCountA = polygonA.size();
    int edgeCountB = polygonB.size();
    qreal minIntervalDistance = 1000000000;
    QPointF translationAxis;
    QPointF edge;

/*
    qDebug() << "A: ";
    for (int k=0; k<edgeCountA;k++)
	qDebug() <<polygonA.at(k);
    qDebug() << "B: ";
    for (int k=0; k<edgeCountB;k++)
	qDebug() <<polygonB.at(k);
    qDebug() ;    
*/	
	
    // Loop through all the edges of both polygons
    for (int i=0;i<edgeCountA + edgeCountB;i++)
    {
        if (i< edgeCountA) 
	{
	    // Loop through polygon A
	    if (i<edgeCountA-1)
		edge = QPointF (
		    polygonA.at(i+1).x()-polygonA.at(i).x(), 
		    polygonA.at(i+1).y()-polygonA.at(i).y());
	    else	
		edge = QPointF (
		    polygonA.at(0).x()-polygonA.at(i).x(), 
		    polygonA.at(0).y()-polygonA.at(i).y());
        } else 
	{
	    // Loop through polygon B
	    if (i < edgeCountA +edgeCountB -1 )
		edge = QPointF (
		    polygonB.at(i-edgeCountA+1).x() - polygonB.at(i-edgeCountA).x(), 
		    polygonB.at(i-edgeCountA+1).y() - polygonB.at(i-edgeCountA).y());
	    else    
		edge = QPointF (
		    polygonB.at(0).x() - polygonB.at(i-edgeCountA).x(), 
		    polygonB.at(0).y() - polygonB.at(i-edgeCountA).y());
	}

        // ===== 1. Find if the polygons are currently intersecting =====

        // Find the axis perpendicular to the current edge

        Vector axis (-edge.y(), edge.x());
        axis.normalize();

        // Find the projection of the polygon on the current axis

        qreal minA = 0; qreal minB = 0; qreal maxA = 0; qreal maxB = 0;
        projectPolygon(axis, polygonA, minA, maxA);
        projectPolygon(axis, polygonB, minB, maxB);

        // Check if the polygon projections are currentlty intersecting

        qreal d = intervalDistance(minA, maxA, minB, maxB);
        if (d > 0) result.intersect = false;

       // ===== 2. Now find if the polygons *will* intersect =====


        // Project the velocity on the current axis

        qreal velocityProjection = axis.dotProduct(velocity);

        // Get the projection of polygon A during the movement

        if (velocityProjection < 0) 
            minA += velocityProjection;
        else 
            maxA += velocityProjection;

        // Do the same test as above for the new projection

        // d = intervalDistance(minA, maxA, minB, maxB);
        //if (d > 0) result.willIntersect = false;
	/*
	qDebug() <<"   ";
	qDebug() << "edge="<<edge<<"  ";
	qDebug() <<"axis="<<axis<<"  ";
	qDebug() <<"dA=("<<minA<<","<<maxA<<")  dB=("<<minB<<","<<maxB<<")";
	qDebug() <<"  d="<<d<<"   ";
	//qDebug() <<"minD="<<minIntervalDistance<<"  ";
	qDebug() <<"int="<<result.intersect<<"  ";
	//qDebug() <<"wint="<<result.willIntersect<<"  ";
	//qDebug() <<"velProj="<<velocityProjection<<"  ";
	qDebug() ;
	*/
    
        if (result.intersect )// || result.willIntersect) 
	{
	    // Check if the current interval distance is the minimum one. If so
	    // store the interval distance and the current distance.  This will
	    // be used to calculate the minimum translation vector

	    if (d<0) d=-d;
	    if (d < minIntervalDistance) {
		minIntervalDistance = d;
		//translationAxis = axis;
		//qDebug() << "tAxix="<<translationAxis;

		//QPointF t = polygonA.Center - polygonB.Center;
		//QPointF t = polygonA.at(0) - polygonB.at(0);
		//if (dotProduct(t,translationAxis) < 0)
		//  translationAxis = -translationAxis;
	    }
	}
    }

    // The minimum translation vector
    // can be used to push the polygons appart.

    if (result.willIntersect)
        result.minTranslation = 
               translationAxis * minIntervalDistance;
    
    return result;
}

/* The function can be used this way: 
   QPointF polygonATranslation = new QPointF();
*/   


/*
PolygonCollisionResult r = PolygonCollision(polygonA, polygonB, velocity);

if (r.WillIntersect) 
  // Move the polygon by its velocity, then move
  // the polygons appart using the Minimum Translation Vector
  polygonATranslation = velocity + r.minTranslation;
else 
  // Just move the polygon by its velocity
  polygonATranslation = velocity;

polygonA.Offset(polygonATranslation);

*/


