#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QColor>
#include <QObject>
#include <QScriptContext>
#include <QScriptable>
#include <QScriptValue>

class BranchItem;
class VymModel;


void logError(QScriptContext *context, QScriptContext::Error error, const QString &text);

class VymModelWrapper : public QObject, protected QScriptable
{
    Q_OBJECT
public:
    VymModelWrapper (VymModel* m);
private:
    BranchItem* getSelectedBranch();
    QVariant getParameter( bool &ok, const QString &key, const QStringList &parameters);

public slots:
    void addBranch();
    void addBranchBefore();
    void addMapCenter( qreal x, qreal y);
    void addMapInsert( QString filename, int pos, int contentFilter);
    void addMapInsert( const QString &filename, int pos);
    void addMapInsert( const QString &filename);
    void addMapReplace( QString filename);
    void addSlide();
    void addXLink( const QString &begin, const QString &end, int width, const QString &color, const QString &penstyle);
    int branchCount();
    int centerCount();
    void centerOnID( const QString &id);        // tested: ok
    void clearFlags();
    void colorBranch( const QString &color);
    void colorSubtree( const QString &color);
    void copy();
    void cut();
    void cycleTask();
    bool exportMap( const QString &format, const QStringList &parameters);
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getFrameType();
    QString getHeadingPlainText();
    QString getHeadingXML();
    QString getMapAuthor();
    QString getMapComment();
    QString getMapTitle();
    QString getNotePlainText();
    QString getNoteXML();
    QString getSelectString();
    void moveDown();
    void moveUp();
    void nop();
    void paste();
    void redo();
    void remove();
    void removeChildren();
    void removeKeepChildren();
    void removeSlide(int n);
    void scroll();
    bool select( const QString &s );
    bool selectID( const QString &s );
    bool selectFirstBranch();
    bool selectLastBranch();
    bool selectLastImage();
    bool selectParent();
    bool selectLatestAdded();
    void setFlag( const QString &s);
    void setHeadingPlainText( const QString &s );
    void setMapAuthor( const QString &s);
    void setMapComment( const QString &s);
    void setMapRotation( float a);              // tested: ok
    void setMapTitle( const QString &s);
    void setMapZoom ( float z);                 // tested: o
    void setNotePlainText( const QString &s );
    void setURL( const QString &s);
    void setVymLink( const QString &s );
    void sleep( int n);
    void sortChildren( bool b);
    void sortChildren();
    void toggleFlag( const QString &s );
    void toggleFrameIncludeChildren();
    void toggleScroll();
    void toggleTarget();
    void toggleTask();
    void undo();
    bool unscroll();
    void unscrollChildren();
    void unselectAll();
    void unsetFlag( const QString &s);

private:
    VymModel *model;
};

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public QObject, protected QScriptable
{
    Q_OBJECT
public:
    VymWrapper ();

public slots:
    void toggleTreeEditor();
    QObject* getCurrentMap();
    void selectMap (uint n);
};


#endif
