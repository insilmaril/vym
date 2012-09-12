#ifndef GEOMETRY
#define GEOMETRY

#include <QPolygonF>

namespace Geometry {
    qreal distance (const QPointF &p, const QPointF &q);
};

QRectF addBBox(QRectF r1, QRectF r2);
QSize addBBoxSize(QSize s1, QSize s2);
bool isInBox(const QPointF &p, const QRectF &box);

class Vector:public QPointF
{
public:
	Vector ();
	Vector (const QPointF &p);
	Vector (qreal x, qreal y);

	friend inline bool operator==(const Vector &v1, const Vector &v2 )
	{ return v1.x()==v2.x() && v1.y()==v2.y(); }

	bool isNull();
	virtual void normalize ();
	virtual qreal dotProduct (const QPointF &b);
	virtual void scale  (const qreal &f);
	virtual void invert ();
	virtual QPointF toQPointF();
};

class ConvexPolygon:public QPolygonF
{
public:
	ConvexPolygon ();
	ConvexPolygon (QPolygonF p);
	void calcCentroid() ;
	QPointF centroid() const;
	qreal weight() const;
	std::string toStdString ();
	Vector at (const int &i) const ; 
	virtual void translate ( const Vector &offset );
	virtual void translate ( qreal dx, qreal dy );
private:
	Vector _centroid;
	qreal _area;
};

class PolygonCollisionResult {
public:
    // Are the polygons going to intersect forward in time?
    bool willIntersect;

    // Are the polygons currently intersecting?
    bool intersect;

    // The translation to apply to the first polygon to push the polygons apart.
    QPointF minTranslation;
};


void projectPolygon(Vector axis, ConvexPolygon polygon, qreal &min, qreal &max) ;

qreal intervalDistance(qreal minA, qreal maxA, qreal minB, qreal maxB);
PolygonCollisionResult polygonCollision(ConvexPolygon polygonA, 
                              ConvexPolygon polygonB, Vector velocity);

#endif