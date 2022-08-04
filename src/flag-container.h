#ifndef FLAG_CONTAINER_H
#define FLAG_CONTAINER_H

#include <QAction>
#include <QPixmap>
#include <QUuid>

#include "flag.h"

#include "image-container.h"

/*! \brief One flag which is visible in the map.

    Flags are usually aligned in a row.
*/

/////////////////////////////////////////////////////////////////////////////
class FlagContainer : public ImageContainer {
  public:
    FlagContainer();
    ~FlagContainer();
    virtual void init();
    void loadImage(ImageContainer *ic);
    void setUuid(const QUuid &uid);
    const QUuid getUuid();
    QPixmap getPixmap();
    void setAction(QAction *);
    void setAlwaysVisible(bool b);
    bool isAlwaysVisible();

  protected:
    QUuid uid;
    bool avis;
};

#endif
