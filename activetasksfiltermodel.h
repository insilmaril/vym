#ifndef ACTIVETASKFILTERMODEL_H
#define ACTIVETASKFILTERMODEL_H

#include <QSortFilterProxyModel>

class ActiveTasksFilterModel:public QSortFilterProxyModel
{
public:
    void setFilter (bool b);
protected:    
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
private:
    bool useFilter;
};

#endif
