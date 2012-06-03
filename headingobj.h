#ifndef HEADINGOBJ_H
#define HEADINGOBJ_H

#include "mapobj.h"

/*! \brief The heading of an OrnamentedObj */

class HeadingObj:public MapObj {
public:
    HeadingObj(QGraphicsItem*);
    virtual ~HeadingObj();
    virtual void init();
    virtual void copy(HeadingObj*);
    virtual void move (double x,double y);      // move to absolute Position
    virtual void moveBy (double x,double y);    // move to relative Position
    virtual void positionBBox();
	virtual void calcBBoxSize();
private:
//    QGraphicsSimpleTextItem* newLine(QString);		// generate new textline
    QGraphicsTextItem* newLine(QString);		// generate new textline
public:    
    virtual void setTransformOriginPoint (const QPointF &);
    virtual void setRotation (qreal const &a);
    virtual qreal getRotation();
private:
    qreal angle;    

public:    
    virtual void setText(QString);
    virtual QString text();
    virtual void setFont(QFont);
    virtual QFont getFont();
    virtual void setColor(QColor);
    virtual QColor getColor();
    virtual void setZValue (double z);
    virtual void setVisibility(bool);
    virtual qreal getHeight();
    virtual qreal getWidth();

protected:
    QString heading;
    int textwidth;								// width for formatting text
    QList <QGraphicsTextItem*> textline;
    QColor color;
    QFont font;
};
#endif
