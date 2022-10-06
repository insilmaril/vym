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
    void setRect(const QRectF &); // set dimensions
    void setPadding(const int &);
    int getPadding();
    qreal getTotalPadding(); // padding +  pen width + xsize (e.g. cloud)
    qreal getXPadding();
    void setPenWidth(const int &);
    int getPenWidth();
    FrameType getFrameType();
    static FrameType getFrameTypeFromString(const QString &);
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
    QString saveToDir();
    void reposition();

  private:
    FrameType frameType; //!< Frame type
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
