#ifndef FRAME_CONTAINER_H
#define FRAME_CONTAINER_H

#include "container.h"

#include "xmlobj.h"
#include <QPen>

/*! \brief This class adds a frame to a Container.
 */

class FrameContainer : public XMLObj, public Container {
  public:
    /*! \brief Supported frame types */
    enum FrameType { NoFrame, Rectangle, RoundedRectangle, Ellipse, Cloud };

    FrameContainer();
    ~FrameContainer();
    void init();
    void clear();

    void repaint();
    virtual void reposition();
    void setFrameZValue(double z);

    // Interfaces
    FrameType frameType();
    static FrameType frameTypeFromString(const QString &);
    QString frameTypeString();
    void setFrameType(const FrameType &);
    void setFrameType(const QString &);
    QRectF frameRect();
    void setFrameRect(const QRectF &); // set dimensions
    void setFramePos(const QPointF &p);
    int framePadding();
    void setFramePadding(const int &);
    qreal frameTotalPadding(); // padding +  pen width + xsize (e.g. cloud)
    qreal frameXPadding();
    int framePenWidth();
    void setFramePenWidth(const int &);
    QColor framePenColor();
    void setFramePenColor(const QColor&);
    QColor frameBrushColor();
    void setFrameBrushColor(const QColor &);
    bool frameIncludeChildren();
    void setFrameIncludeChildren(bool);
    void setFrameRotation(const qreal &);
    QString saveFrame();

  protected:
    FrameType frameTypeInt; //! Frame type
    int framePaddingInt;    //! Distance text - frame
    QPen framePen;
    QBrush frameBrush;
    bool frameIncludeChildrenInt;

  private:
    QGraphicsRectItem *rectFrame;
    QGraphicsEllipseItem *ellipseFrame;
    QGraphicsPathItem *pathFrame;
    qreal frameXSize; //! Extra size caused e.g. by cloud geometry
    QRectF frameRectInt;
    qreal angle;
};
#endif
