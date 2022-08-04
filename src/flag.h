#ifndef FLAG_H
#define FLAG_H

#include <QAction>
#include <QUuid>

#include "xmlobj.h"

class ImageContainer;

/*! \brief One flag belonging to a FlagRow.

    Each TreeItem in a VymModel has a set of standard flags and system
    flags.
*/

/////////////////////////////////////////////////////////////////////////////
class Flag : public XMLObj {
  public:
    enum FlagType {
        SystemFlag,
        StandardFlag,
        UserFlag,
        FreemindFlag,
        UndefinedFlag
    };

    Flag();
    Flag(const QString &fname);
    ~Flag();
    virtual void init();
    bool load(const QString &);
    void setName(const QString &);
    const QString getName();
    const QString getPath();
    void setVisible(bool b);
    bool isVisible();
    void setGroup(const QString &);
    const QString getGroup();
    void unsetGroup();
    void setToolTip(const QString &);
    const QString getToolTip();
    ImageContainer *getImageContainer();
    void setAction(QAction *a);
    QAction *getAction();
    void setUsed(bool);
    bool isUsed();
    FlagType getType();
    void setType(FlagType t);
    void setUuid(const QUuid &id);
    QUuid getUuid();
    QString getDefinition(const QString &prefix);
    void saveDataToDir(const QString &);
    QString saveState();

  protected:
    QString name;
    bool visible;
    QString group;
    QString tooltip;
    QAction *action;
    bool state;
    bool used;
    FlagType type;
    QUuid uuid;

  private:
    ImageContainer *imageContainer;
    QString path;
};

#endif
