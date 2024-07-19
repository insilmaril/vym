#ifndef FLAGROW_H
#define FLAGROW_H

#include <QList>
#include <QStringList>
#include <QToolBar>

class FlagRowMaster;

class FlagRow
{
  public:
    FlagRow ();
    ~FlagRow ();
    const QStringList activeFlagNames();
    const QList<QUuid> activeFlagUids();
    bool isActive(const QString &name);
    bool isActive(const QUuid &uuid);
    bool hasFlag(const QString &name);

    /*! \brief Toggle a Flag
    To activate a flag its uid will be copied from masterRow to activeUids in
    current row.
    */
    bool toggle(const QString &, bool useGroups = true);
    bool toggle(const QUuid &, bool useGroups = true);
    bool activate(const QString &);
    bool activate(const QUuid &);
    bool deactivate(const QString &);
    bool deactivate(const QUuid &);
    bool deactivateGroup(const QString &);
    void deactivateAll();
    QString saveState();
    void setMasterRow(FlagRowMaster *row);

  private:
    FlagRowMaster *masterRow;
    QList<QUuid> activeUids; //! Used in treeitems: Lists all uids of currently
                             //! active flags
};
#endif
