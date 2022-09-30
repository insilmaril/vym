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

    FrameContainer(Container *parent);
    ~FrameContainer();
    void init();
    void clear();
    void setPos(double x, double y);   // move to absolute Position
    void setRect(const QRectF &); // set dimensions
    void setPadding(const int &);
    int getPadding();
    qreal getTotalPadding(); // padding + borderwidth + xsize (e.g. cloud)
    qreal getXPadding();
    void setBorderWidth(const int &);
    int getBorderWidth();
    FrameType getFrameType();
    FrameType getFrameTypeFromString(const QString &);
    QString getFrameTypeName();
    void setFrameType(const FrameType &);
    void setFrameType(const QString &);
    void setPenColor(QColor);
    QColor getPenColor();
    void setBrushColor(QColor);
    QColor getBrushColor();
    void setIncludeChildren(bool);
    bool getIncludeChildren();
    void repaint();
    void setZValue(double z);
    void setVisibility(bool);
    QString saveToDir();
    virtual void reposition();

  private:
    FrameType type; //!< Frame type
    QRectF frameSize;
    QGraphicsRectItem *rectFrame;
    QGraphicsEllipseItem *ellipseFrame;
    QGraphicsPathItem *pathFrame;
    int padding; // distance text - frame
    qreal xsize; //! Extra size caused e.g. by cloud geometry
    QPen pen;
    QBrush brush;
    bool includeChildren;
};
#endif
