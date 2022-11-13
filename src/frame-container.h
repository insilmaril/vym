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

    FrameContainer(QGraphicsItem *parent);
    ~FrameContainer();
    void init();
    void clear();
    void setFrameRect(const QRectF &); // set dimensions
    QRectF getFrameRect();
    void setFramePadding(const int &);
    int getFramePadding();
    qreal getFrameTotalPadding(); // padding +  pen width + xsize (e.g. cloud)
    qreal getFrameXPadding();
    void setFramePenWidth(const int &);
    int getFramePenWidth();
    FrameType getFrameType();
    static FrameType getFrameTypeFromString(const QString &);
    QString getFrameTypeName();
    void setFrameType(const FrameType &);
    void setFrameType(const QString &);
    void setFramePenColor(QColor);
    QColor getFramePenColor();
    void setFrameBrushColor(QColor);
    QColor getFrameBrushColor();
    void setFrameIncludeChildren(bool);
    bool getFrameIncludeChildren();
    void repaint();
    void setFrameZValue(double z);
    QString saveFrame();
    virtual void reposition();

  protected:
    FrameType frameType;    //! Frame type
    int framePadding;       //! Distance text - frame
    QPen framePen;
    QBrush frameBrush;
    bool frameIncludeChildren;

  private:
    QGraphicsRectItem *rectFrame;
    QGraphicsEllipseItem *ellipseFrame;
    QGraphicsPathItem *pathFrame;
    qreal frameXSize; //! Extra size caused e.g. by cloud geometry
    QRectF frameRect;
};
#endif
