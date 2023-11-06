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
    enum FrameType { NoFrame, Rectangle, RoundedRectangle, Ellipse, Circle, Cloud };

    /*! When saving, save also the usage */
    enum FrameUsage {Undefined, InnerFrame, OuterFrame};

    FrameContainer();
    ~FrameContainer();
    void init();
    void clear();
    void repaint();
    virtual void reposition();

    // Interfaces
    FrameType frameType();
    static FrameType frameTypeFromString(const QString &);
    QString frameTypeString();
    void setFrameDesignAuto(const bool &);
    void setFrameType(const FrameType &);
    void setFrameType(const QString &);

  private:
    void updateGeometry(const QRectF &);

  public:  
    int framePadding();
    void setFramePadding(const int &);
    int framePenWidth();
    void setFramePenWidth(const int &);
    QColor framePenColor();
    void setFramePenColor(const QColor&);
    QColor frameBrushColor();
    void setFrameBrushColor(const QColor &);
    void setUsage(FrameUsage u);
    QString saveFrame();

  protected:
    FrameType frameTypeInt; //! Frame type
    int framePaddingInt;    //! Distance text - frame
    QPen framePen;
    QBrush frameBrush;

  private:
    QGraphicsRectItem *rectFrame;
    QGraphicsEllipseItem *ellipseFrame;
    QGraphicsPathItem *pathFrame;
    QRectF contentRect;
    FrameUsage usage;
};
#endif
