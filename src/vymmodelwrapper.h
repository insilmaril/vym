#ifndef VYMMODELWRAPPER_H
#define VYMMODELWRAPPER_H

#include "scripting.h"

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class BranchWrapper;
class ImageWrapper;
class VymModel;

class VymModelWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymModelWrapper(VymModel *m);

  public slots:
    void addMapCenterAtPos(qreal x, qreal y);
    void addMapInsert(QString filename, int pos, int contentFilter);
    void addMapInsert(const QString &filename, int pos);
    void addMapInsert(const QString &filename);
    void addMapReplace(QString filename);
    void addSlide();
    void addXLink(const QString &begin, const QString &end, int width,// FIXME-3 move to BranchWrapper
                  const QString &color, const QString &penstyle);
    int centerCount();
    void centerOnID(const QString &id); // tested: ok
    void copy();        // FIXME-3 copy to BranchWrapper (use without selecting in VymModel!)
    void cut();         // FIXME-3 copy to BranchWrapper  (use without selecting in VymModel!)
    void cycleTask();   // FIXME-3 move to BranchWrapper
    int depth();        // FIXME-3 move to BranchWrapper
    void detach();      // FIXME-3 move to BranchWrapper
    bool exportMap(QJSValueList args);
    BranchWrapper* findBranchByAttribute(const QString &key, const QString &value);
    BranchWrapper* findBranchById(const QString &);
    ImageWrapper* findImageById(const QString &);
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getHeadingXML();    // FIXME-3 move to BranchWrapper
    QString getAuthor();
    QString getComment();
    QString getTitle();
    QString getNotePlainText(); // FIXME-3 move to BranchWrapper
    QString getNoteXML();       // FIXME-3 move to BranchWrapper
    qreal getPosX();            // FIXME-3 move to BranchWrapper, copy for image
    qreal getPosY();            // FIXME-3 move to BranchWrapper, copy for image
    qreal getScenePosX();       // FIXME-3 move to BranchWrapper, copy for image
    qreal getScenePosY();       // FIXME-3 move to BranchWrapper, copy for image
    int getRotationHeading();   // FIXME-3 move to BranchWrapper
    int getRotationSubtree();   // FIXME-3 move to BranchWrapper
    QString getSelectionString();//FIXME-3 copy to BranchWrapper?
    int getTaskPriorityDelta(); // FIXME-3 move to BranchWrapper
    QString getTaskSleep();     // FIXME-3 move to BranchWrapper
    int getTaskSleepDays();     // FIXME-3 move to BranchWrapper
    QString getXLinkColor();    // FIXME-3 move to XLinkWrapper
    int getXLinkWidth();        // FIXME-3 move to XLinkWrapper
    QString getXLinkPenStyle(); // FIXME-3 move to XLinkWrapper
    QString getXLinkStyleBegin();// FIXME-3 move to XLinkWrapper
    QString getXLinkStyleEnd();// FIXME-3 move to XLinkhWrapper
    bool hasActiveFlag(const QString &flag);// FIXME-3 move to BranchWrapper
    bool hasNote();             // FIXME-3 move to BranchWrapper
    bool hasRichTextNote();     // FIXME-3 move to BranchWrapper
    bool hasTask();             // FIXME-3 move to BranchWrapper
    void importDir(const QString &path);
    void loadImage(const QString &filename);// FIXME-3 move to BranchWrapper
    void loadNote(const QString &filename); // FIXME-3 move to BranchWrapper
    void moveSlideDown(int n);
    void moveSlideDown();
    void moveSlideUp(int n);
    void moveSlideUp();
    void newBranchIterator(const QString &itname, bool deepLevelsFirst = false);
    BranchWrapper* nextBranch(const QString &itname);
    void note2URLs();       // FIXME-3 move to BranchWrapper
    void paste();           // FIXME copy to BranchWrappe (use without selecting in VymModel!)
    void redo();
    void remove();          // FIXME-3 copy to ImageWrapper, leave for VymModel (already in BranchWrapper)
    void removeChildren();      // FIXME-3 move to BranchWrapper
    void removeChildBranches(); // FIXME-3 move to BranchWrapper
    void removeKeepChildren();  // FIXME-3 move to BranchWrapper
    void removeSlide(int n);
    QVariant repeatLastCommand();
    void saveImage(const QString &filename);// FIXME-3 move to ImageWrapper
    void saveNote(const QString &filename); // FIXME-3 move to BranchWrapper
    void saveSelection(const QString &filename);
    bool select(const QString &s);
    BranchWrapper* selectedBranch();
    bool selectID(const QString &s);
    bool selectFirstChildBranch();  // FIXME-3 move to BranchWrapper
    bool selectLastChildBranch();   // FIXME-3 move to BranchWrapper
    bool selectLastImage();         // FIXME-3 move to BranchWrapper
    bool selectLatestAdded();
    bool selectToggle(const QString &selectString); // FIXME-3 move to BranchWrapper and ImageWrapper
    bool selectXLink(int n);        // FIXME-3 move to BranchWrapper
    bool selectXLinkOtherEnd(int n);// FIXME-3 move to XLinkWrapper
    void setDefaultLinkColor(const QString &color); // FIXME-3-4 maybe also rename other setMap* methods?
    void setHeadingConfluencePageName();// FIXME-3 move to BranchWrapper
    void setHideExport(bool b);         // FIXME-3 move to BranchWrapper
    void setHideLinkUnselected(bool b); // FIXME-3 move to BranchWrapper and ImageWrapper
    void setMapAnimCurve(int n);        // FIXME-3 rename
    void setMapAnimDuration(int n);     // FIXME-3 rename
    void setAuthor(const QString &s);
    void setMapBackgroundColor(const QString &color);// FIXME-3 rename 
    void setComment(const QString &s);
    void setMapLinkStyle(const QString &style); // FIXME-3 rename 
    void setMapRotation(float a);               // FIXME-3 rename 
    void setTitle(const QString &s);
    void setMapZoom(float z);                   // FIXME-3 rename 
    void setNotePlainText(const QString &s);    // FIXME-3 OBSOLETE moved to BranchWrapper
    void setRotationHeading(const int &i);      // FIXME-3 move to BranchWrapper
    void setRotationSubtree(const int &i);      // FIXME-3 move to BranchWrapper
    void setRotationsAutoDesign(const bool b);  // FIXME-3 move to BranchWrapper
    void setScale(qreal f);                     // FIXME-3 move to BranchWrapper and ImageWrapper
    void setScaleSubtree(qreal f);              // FIXME-3 move to BranchWrapper
    void setScalingAutoDesign(const bool b);    // FIXME-3 move to BranchWrapper and ImageWrapper
    void setSelectionBrushColor(const QString &color);
    void setSelectionPenColor(const QString &color);
    void setSelectionPenWidth(const qreal &);
    void setTaskPriorityDelta(const int &n);    // FIXME-3 move to BranchWrapper
    bool setTaskSleep(const QString &s);        // FIXME-3 move to BranchWrapper
    void setXLinkColor(const QString &color);   // FIXME-3 move to XLinkWrapper
    void setXLinkStyle(const QString &style);   // FIXME-3 move to XLinkWrapper
    void setXLinkStyleBegin(const QString &style);// FIXME-3 move to XLinkWrapper
    void setXLinkStyleEnd(const QString &style);// FIXME-3 move to XLinkWrapper
    void setXLinkWidth(int w);// FIXME-3 move to XLinkWrapper
    void sleep(int n);
    int slideCount();
    void sortChildren(bool b);  // FIXME-3 move to BranchWrapper
    void sortChildren();        // FIXME-3 move to BranchWrapper
    void toggleTarget();        // FIXME-3 move to BranchWrapper
    void toggleTask();          // FIXME-3 move to BranchWrapper
    void undo();
    void unscrollChildren();    // FIXME-3 move to BranchWrapper
    void unselectAll();
    int xlinkCount();           // FIXME-3 move to BranchWrapper

  private:
    VymModel *model;
};

#endif
