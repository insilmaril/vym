#ifndef FLAGROWMASTER_H
#define FLAGROWMASTER_H

#include <QList>
#include <QStringList>
#include <QToolBar>

#include "flag.h"
#include "xmlobj.h"

/*! \brief A set of flags (Flag).

   A toolbar can be created from the flags in this row.
   The data needed for represention in a vym map
   is stored in FlagRowObj.
 */

class FlagRowMaster : public XMLObj {
  public:
    enum WriteMode { NoFlags, UsedFlags, AllFlags };
    FlagRowMaster();
    ~FlagRowMaster();
    Flag *createFlag(const QString &path);
    void createConfigureAction();
    void addActionToToolbar(QAction *a);
    Flag *findFlagByUid(const QUuid &uid);
    Flag *findFlagByName(const QString &name);
    void resetUsedCounter();
    QString saveDef(WriteMode mode);
    void saveDataToDir(const QString &, WriteMode mode);
    void setName(const QString &); // prefix for exporting flags to dir
    QString getName();             // Used for debugging only
    void setPrefix(const QString &prefix);
    void setToolBar(QToolBar *tb);
    void setEnabled(bool);
    void updateToolBar(QList<QUuid> activeUids);

  private:
    QToolBar *toolbar;
    QList<Flag *> flags;      //! Used in to define flags
    QString rowName;          //! Name of this collection of flags
    QString prefix;           //! Prefix for saving data: user/ or standard/
    QAction *configureAction; //! Action to trigger loading new user flags
};
#endif
