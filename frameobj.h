#ifndef FRAMEOBJ_H
#define FRAMEOBJ_H

#include "mapobj.h"


/*! \brief This class adds a frame to a MapObj. 
*/

class FrameObj:public MapObj {
public:

    /*! \brief Supported frame types */
    enum FrameType {NoFrame,Rectangle,Ellipse,Cloud};

    FrameObj();
    FrameObj(QGraphicsScene*);
    ~FrameObj();
    void init();
    void clear();
    void move (double x,double y);  // move to absolute Position
    void moveBy (double x,double y);// move to relative Position
    void positionBBox();	     
    void calcBBoxSize();	    
    float roof (float x);	    // "roof" function
    void setRect (const QRectF &);   // set dimensions		
    void setPadding(const int &);
    int getPadding();
    void setBorderWidth (const int &);
    int getBorderWidth ();
    FrameType getFrameType ();
    FrameType getFrameType (const QString &);
    QString getFrameTypeName ();
    void setFrameType (const FrameType &);
    void setFrameType (const QString &);
    void setPenColor (QColor);
    QColor getPenColor ();
    void setBrushColor (QColor);
    QColor getBrushColor ();
    void setFrameIncludeChildren(bool);
    bool getFrameIncludeChildren();
    void repaint();
    void setZValue (double z);
    void setVisibility(bool);
    QString saveToDir ();

private:
    FrameType type;	    //!< Frame type
    QGraphicsRectItem * rectFrame;
    QGraphicsEllipseItem * ellipseFrame;
    QGraphicsPathItem *pathFrame;
    int padding;	    // distance text - frame
    int borderWidth;
    QColor penColor;
    QColor brushColor;
    bool includeChildren;
};
#endif

