#ifndef TASKFILTERMODEL_H
#define TASKFILTERMODEL_H

#include <QSortFilterProxyModel>

class Task;

class TaskFilterModel : public QSortFilterProxyModel {
  public:
    void setFilter(bool b);
    void setFilterNew(bool b);
    void setFilterBlocker(bool b);
    void setMapFilter(const QString &s);
    void setFilterFlags1(bool b);
    void setFilterFlags2(bool b);
    void setFilterFlags3(bool b);
    bool taskVisible(Task*) const;

  protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

  private:
    bool useFilter;
    QString mapFilter;
    bool filterNew;
    bool filterBlocker;
    bool filterFlags1;
    bool filterFlags2;
    bool filterFlags3;
};

#endif
