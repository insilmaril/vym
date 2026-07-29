#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include <QUuid>

#include "codeeditor.h"
#include "highlighter.h"
#include "ui_scripteditor.h"

class BranchItem;
class VymModel;

class ScriptEditor : public QWidget {
    Q_OBJECT

  public:
    enum Mode { Branch, Slide, File };

  private:
    Mode mode;
    uint vymModelID;
    uint slideID;

  public:
    ScriptEditor(QWidget *parent = 0);
    void setFocus();
    QString getScriptFile();
    void setSlideScript(uint vymModelID, uint slideID, const QString &);

    //! Show script of currently selected branch, if exactly one is selected
    void updateBranchScript(VymModel *model);

    /*! \brief Reload script shown in branch tab

    Called, when the script of a branch has been changed elsewhere, e.g. by
    undo/redo or a script. Ignored, if the branch is not shown currently.
    */
    void reloadBranchScript(BranchItem *bi);

    //! Show branch tab and set focus there
    void setFocusBranchScript();

  protected:
    bool eventFilter(QObject *obj, QEvent *ev);

  public slots:
    void closeWindow();
    void runMacro();
    void runSlide();
    void runScript();
    void runBranchScript();
    void deleteBranchScript();
    void saveBranchScript();
    void saveSlide();
    void reloadMacros();
    void saveMacros();
    bool loadScript(QString fn = "");
    bool reloadScript();
    void saveScript();
    void saveScriptAs();

  private slots:
    void branchScriptChanged();

  signals:
    void runScript(QString);

  private:
    BranchItem *branchScriptItem();

    Ui::ScriptEditor ui;
    CodeEditor *branchEditor;
    CodeEditor *slideEditor;
    CodeEditor *macroEditor;
    CodeEditor *codeEditor;
    QString filename;
    Highlighter *highlighterBranch;
    Highlighter *highlighterMacro;
    Highlighter *highlighterSlide;
    Highlighter *highlighterFile;

    uint branchModelID;         //! Model of branch shown in branch tab
    QUuid branchUuid;           //! Uuid of branch shown in branch tab
    QString branchScriptOriginal; //! Script of branch before editing started
    bool branchScriptModified;  //! True, if branch script has been edited
    bool blockBranchScriptSignal; //! Set while loading a script into editor
};

#endif
