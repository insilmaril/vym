#ifndef MAPDESIGN_H
#define MAPDESIGN_H

#include <QFont>
#include <QString>

//#include "branch-container.h"
//#include "branchitem.h"
#include "container.h"
#include "frame-container.h"
#include "linkobj.h"

/////////////////////////////////////////////////////////////////////////////

// Extended QList, which allows to access element at position n or, if n does not exist,
// the last existing element before n
template <typename T> class ConfigList {
  public:
    QList <T> qlist;
    ConfigList <T> & operator<<(const T &other);
    T & operator[](int i);
    T tryAt(int);
    void setAt(int i, const T &val);
    int count();
    void clear();
    QString save(const QString &attrName, QString (&f)(int));
    QString saveBool(const QString &attrName);
    QString saveColor(const QString &attrName);
    QString saveInt(const QString &attrName);

  protected:
    T defaultValue;
};

/////////////////////////////////////////////////////////////////////////////
/*! \brief A MapDesign defines the visual appearance of a map, e.g. how branches, frames,     links look. Settings depend on
      - depth
      - Mode: NewBranch, RelinkBranch
    Settings may be overriden locallay and locally also depend on other settings, 
    e.g. links may depend on frames
*/

class BranchContainer;
class BranchItem;

class MapDesign {
  public:
    enum HeadingColorHint {
        SpecificColor,
        InheritedColor,
        UnchangedColor,
        UndefinedColor};

    enum UpdateMode : unsigned int {
        Undefined           = 0x0000,
        LoadingMap          = 0x0001,
        //MapImport         = 0x0002
        CreatedByUser       = 0x0004,
        RelinkedByUser      = 0x0008,
        LayoutChanged       = 0x0010,
        LinkStyleChanged    = 0x0020,
        StyleChanged        = 0x0040,   // e.g. heading color, which could change link color, too
        AutoDesign          = 0x0080
    };

    /*
    constexpr RelinkMode operator|(RelinkMode X, RelinkMode Y) {
        return static_cast<RelinkMode>(
            static_cast<unsigned int>(X) | static_cast<unsigned int>(Y));
    }

    RelinkMode& operator|=(RelinkMode& X, RelinkMode Y) {
        X = X | Y; return X;
    }
    */

    MapDesign();
    void init();

// Basic data of MapDesign
  public:
    void setName(const QString &);
    QString getName();
    static QString updateModeString(const UpdateMode &mode);

  private:
    QString name;

// Parse MapDesign when read from map file or undo/redo command
  public:
    bool setElement(const QString &k, const QString &v, const QString &d);

// Container layouts
  public:  
    Container::Layout branchesContainerLayout(int depth);
    Container::VerticalAlignment branchesContainerVerticalAlignment(int depth);

    Container::Layout imagesContainerLayout(int depth);
    bool branchesContainerAndOrnamentsVertical(int depth);

  private:
    ConfigList <Container::Layout> branchesContainerLayouts;
    ConfigList <Container::Layout> imagesContainerLayouts;
    ConfigList <bool> branchesContainerAndOrnamentsVerticalInt;
    ConfigList <Container::VerticalAlignment> branchesContainerVerticalAlignmentsInt;

// Links
  public:
    LinkObj::ColorHint linkColorHint();
    void setLinkColorHint(const LinkObj::ColorHint &lch);
    QColor defaultLinkColor();
    void setDefaultLinkColor(const QColor &col);

    LinkObj::Style linkStyle(int depth);
    void setLinkStyle(LinkObj::Style style, int depth);

    qreal linkWidth();

  private:
    LinkObj::ColorHint linkColorHintInt;
    QColor defaultLinkCol;
    ConfigList <LinkObj::Style> linkStyles;

// XLinks
  public:
    void setDefXLinkPen(const QPen &p);
    QPen defXLinkPen();
    void setDefXLinkStyleBegin(const QString &s);
    QString defXLinkStyleBegin();
    void setDefXLinkStyleEnd(const QString &s);
    QString defXLinkStyleEnd();

  private:
    QPen defXLinkPenInt;           // default pen for xlinks
    QString defXLinkStyleBeginInt; // default style begin
    QString defXLinkStyleEndInt;

// Background
  public:
    void setBackgroundColor(const QColor &);
    QColor backgroundColor();

    bool setBackgroundImage(const QString &fileName);
    void setBackgroundImageName(const QString &);
    void unsetBackgroundImage();
    bool hasBackgroundImage();
    QString backgroundImageName();
    QBrush backgroundImageBrush();

  private:
    QColor backgroundColorInt;
    bool usesBackgroundImage;
    QString backgroundImageNameInt;
    QImage backgroundImage;
    QBrush backgroundImageBrushInt;

// Heading & Fonts
  public:
    QFont font();
    void setFont(const QFont &f);
    int headingColumnWidth(const int &depth);

    QColor headingColor(
            const MapDesign::UpdateMode &updateMode,
            BranchItem *branchItem,
            bool &updateRequired);

  private:
    QFont fontInt;

    ConfigList <int> headingColumnWidths;
    ConfigList <MapDesign::HeadingColorHint> headingColorHints;
    ConfigList <QColor> headingColors;
    ConfigList <bool> headingColorUpdateWhenRelinking;

// Frames
  public:
    FrameContainer::FrameType frameType(bool useInnerFrame, const int &depth);
    QColor frameBrushColor(bool useInnerFrame, const int &depth);
    QColor framePenColor(bool useInnerFrame, const int &depth);
    int framePenWidth(bool useInnerFrame, const int &depth);
    bool updateFrameWhenRelinking(bool useInnerFrame, const int &depth);

  private:
    ConfigList <FrameContainer::FrameType> innerFrameTypes;
    ConfigList <QColor> innerFramePenColors;
    ConfigList <QColor> innerFrameBrushColors;
    ConfigList <int> innerFramePenWidths;
    ConfigList <bool> innerFrameUpdateWhenRelinking;

    ConfigList <FrameContainer::FrameType> outerFrameTypes;
    ConfigList <QColor> outerFramePenColors;
    ConfigList <QColor> outerFrameBrushColors;
    ConfigList <int> outerFramePenWidths;
    ConfigList <bool> outerFrameUpdateWhenRelinking;

// Selections
  public:  
    QPen selectionPen();
    void setSelectionPen(const QPen &);
    QBrush selectionBrush();
    void setSelectionBrush(const QBrush &);

  private:
    QPen selectionPenInt;
    QBrush selectionBrushInt;

// Transformations
  public:  
    int rotationHeading(const int &depth);
    int rotationSubtree(const int &depth);
    qreal scaleHeading(const int &depth);
    qreal scaleSubtree(const int &depth);

  private:
    ConfigList <int> rotationHeadingInt;
    ConfigList <int> rotationSubtreeInt;
    ConfigList <double> scaleHeadingInt;
    ConfigList <double> scaleSubtreeInt;

  public:
    QString saveToDir(const QString &tmpdir, const QString &prefix);

};

/////////////////////////////////////////////////////////////////////////////
inline MapDesign::UpdateMode operator|(MapDesign::UpdateMode a, MapDesign::UpdateMode b)
{
    return static_cast<MapDesign::UpdateMode>(static_cast<int>(a) | static_cast<int>(b));
}

inline MapDesign::UpdateMode operator|=(MapDesign::UpdateMode &a, MapDesign::UpdateMode b)
{
    a = a | b; return a;
}

#endif
