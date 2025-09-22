#ifndef TREEEDITOR_H
#define TREEEDITOR_H

#include <QTreeView>

class VymModel;

/*! \brief TreeView widget in vym to display and edit a map, based on
 * QTreeView */

class TreeEditor : public QTreeView {
    Q_OBJECT

  public:
    TreeEditor(VymModel *m = nullptr);
    ~TreeEditor();
    void init();
    QModelIndex getSelectedIndex();

  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);
    virtual void closeEvent(QCloseEvent *event);

  public slots:
    void closeWindow();

  private slots:
    void cursorUp();
    void cursorDown();
    void startEdit();

  private:
    VymModel *model;
};

#endif
