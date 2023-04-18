#include "mapdesign.h"

#include <QDebug>

#include "branchitem.h"

extern bool usingDarkTheme;

template <typename T> ConfigList <T> & ConfigList<T>::operator<<(const T &other) {
    qlist << other;
}

template <typename T> T & ConfigList<T>::operator[](int i) {
    if (i >= 0 && i < qlist.count())
        return qlist.at(i);
}

template <typename T> T ConfigList<T>::tryAt(int i) {
    if (i >= qlist.count())
        return qlist.last();
    else
        return qlist.at(i);
}

template <typename T> int ConfigList<T>::count() {
    return qlist.count();
}

/*
FIXME-3 currently not used template <typename T> void ConfigList<T>::setDefault(const T &d) {
    defaultValue = d;
}
*/

/////////////////////////////////////////////////////////////////
// MapDesign
/////////////////////////////////////////////////////////////////

MapDesign::MapDesign()
{
    //qDebug() << "Constr. MapDesign";
    init();
}

void MapDesign::init()
{
    int mapDesign = 0;  // FIXME-2 only for testing, later load/save

    if (mapDesign == 0) {
        // Default mapDesign
        // NewBranch: Layout of children branches 
        branchContainerLayouts << Container::FloatingBounded;
        branchContainerLayouts << Container::Vertical;

        // NewBranch: Layout of children images 
        imageContainerLayouts << Container::FloatingFree;

        // Heading colors
        headingColorHints << MapDesign::SpecificColor;         // Specific for MapCenter
        headingColorHints << MapDesign::InheritedColor;        // Use color of parent

        headingColors << QColor(Qt::white);
        headingColors << QColor(Qt::green);

        // Frames
        innerFrameTypes << FrameContainer::RoundedRectangle;
        innerFrameTypes << FrameContainer::Rectangle;
        innerFrameTypes << FrameContainer::Rectangle;
        innerFrameTypes << FrameContainer::NoFrame;
        innerFramePenWidths << 2;

        outerFrameTypes << FrameContainer::Rectangle;
        outerFrameTypes << FrameContainer::RoundedRectangle;
        outerFrameTypes << FrameContainer::Rectangle;
        outerFrameTypes << FrameContainer::Rectangle;
        outerFrameTypes << FrameContainer::NoFrame;
        if (usingDarkTheme) {
            innerFramePenColors << QColor(Qt::black);
            innerFrameBrushColors << QColor(85, 85, 127);
            outerFramePenColors << QColor(Qt::green);
            outerFramePenColors << QColor(Qt::red);
            outerFramePenColors << QColor(Qt::green);
            outerFramePenColors << QColor(Qt::red);
            outerFrameBrushColors << QColor(85, 85, 127);
            outerFrameBrushColors << QColor(25, 25, 117);
            outerFrameBrushColors << QColor(85, 85, 127);
            outerFrameBrushColors << QColor(25, 25, 117);
        } else {
            innerFramePenColors << QColor(Qt::black);
            innerFrameBrushColors << QColor(Qt::white);
            outerFramePenColors << QColor(Qt::green);
            outerFrameBrushColors << QColor(85, 85, 127);
        }

        outerFrameTypes << FrameContainer::NoFrame;
        /*
        outerFramePenColors;
        outerFrameBrushColors;
        outerFramePenWidths;
        */

        // Should links of branches use a default color or the color of heading?
        linkColorHintInt = LinkObj::DefaultColor;
        defaultLinkCol = Qt::blue;

        linkStyles << LinkObj::NoLink;
        linkStyles << LinkObj::Parabel;
    } else if (mapDesign == 1) {
        // Rainbow colors depending on depth mapDesign
        // NewBranch: Layout of children branches 
        branchContainerLayouts << Container::FloatingBounded;
        branchContainerLayouts << Container::Vertical;

        // NewBranch: Layout of children images 
        imageContainerLayouts << Container::FloatingFree;

        // Heading colors
        headingColorHints << MapDesign::SpecificColor;

        headingColors << QColor(Qt::red);
        headingColors << QColor(Qt::green);
        headingColors << QColor(Qt::blue);
        headingColors << QColor(Qt::white);
        headingColors << QColor(Qt::yellow);

        // Should links of branches use a default color or the color of heading?
        linkColorHintInt = LinkObj::DefaultColor;
        defaultLinkCol = Qt::blue;

        linkStyles << LinkObj::NoLink;
        linkStyles << LinkObj::Parabel;
        //linkStyles << LinkObj::PolyLine;
        //linkStyles << LinkObj::Line;
        //linkStyles << LinkObj::PolyParabel;
        //linkStyles << LinkObj::Parabel;
    }
}

void MapDesign::setName(const QString &s)
{
    name = s;
}

QString MapDesign::getName()
{
    return name;
}

Container::Layout MapDesign::branchesContainerLayout(int depth)
{
    return branchContainerLayouts.tryAt(depth);
}

Container::Layout MapDesign::imagesContainerLayout(int depth)
{
    return imageContainerLayouts.tryAt(depth);
}

LinkObj::ColorHint MapDesign::linkColorHint()
{
    return linkColorHintInt;
}

void MapDesign::setLinkColorHint(const LinkObj::ColorHint &lch)
{
    linkColorHintInt = lch;
}

QColor MapDesign::defaultLinkColor()
{
    return defaultLinkCol;
}

void MapDesign::setDefaultLinkColor(const QColor &col)
{
    defaultLinkCol = col;
}

LinkObj::Style MapDesign::linkStyle(int depth)
{
    // Special case for now:    // FIXME-3
    // For style PolyParabel or PolyLine in d == 1
    // return Parabel or Line for d > 1

    if (depth < 2)
        return linkStyles.tryAt(depth);

    if (linkStyles.tryAt(1) == LinkObj::PolyParabel)
        return LinkObj::Parabel;

    if (linkStyles.tryAt(1) == LinkObj::PolyLine)
        return LinkObj::Line;

    return linkStyles.tryAt(1);    // Return either Line or Parabel
}

bool MapDesign::setLinkStyle(const LinkObj::Style &style, int depth)
{
    // Special case for now: only set LinkStyle for first levels    // FIXME-2
    // Only set style for d=1 (Used to be map-wide setting before 3.x)

    if (linkStyles.count() < 2)

    return true;
}

void MapDesign::updateBranchHeadingColor(
        BranchItem *branchItem,
        int depth)
{
    if (branchItem) {
        //qDebug() << "MD::updateBranchHeadingColor " << " d=" << depth << branchItem->getHeadingPlain(); // FIXME-2
        HeadingColorHint colHint = headingColorHints.tryAt(depth);

        QColor col;
        switch (colHint) {
            case InheritedColor: {
                //qDebug() << " - InheritedColor"; // FIXME-2
                BranchItem *pi = branchItem->parentBranch();
                if (pi) {
                    col = pi->getHeadingColor();
                    break;
                }
                // If there is no parent branch, mapCenter should 
                // have a specific color, thus continue
            }
            case SpecificColor: {
                //qDebug() << " - SpecificColor"; // FIXME-2
                col = headingColors.tryAt(depth);
                branchItem->setHeadingColor(col);

                break;
            }
            case UnchangedColor:
                qDebug() << " - UnchangedColor";
                return;
            default:
                qWarning() << "MapDesign::updateBranchHeadingColor no branchHeadingColorHint defined";
        }
        branchItem->setHeadingColor(col);
    }
}

FrameContainer::FrameType MapDesign::frameType(bool useInnerFrame, int depth)
{
    if (useInnerFrame)
        return innerFrameTypes.tryAt(depth);
    else
        return outerFrameTypes.tryAt(depth);
}

void MapDesign::updateFrames(
    BranchContainer *branchContainer,
    const CreationMode &creationMode,
    const RelinkMode &relinkMode,
    int depth)
{
    if (branchContainer && creationMode != NotCreated) {
        // Inner frame
        branchContainer->setFrameType(true, frameType(true, depth));
        branchContainer->setFrameBrushColor(true, innerFrameBrushColors.tryAt(depth));
        branchContainer->setFramePenColor(true, innerFramePenColors.tryAt(depth));

        // Outer frame
        branchContainer->setFrameType(false, frameType(false, depth));
        branchContainer->setFrameBrushColor(false, outerFrameBrushColors.tryAt(depth));
        branchContainer->setFramePenColor(false, outerFramePenColors.tryAt(depth));
    }
}

QColor MapDesign::selectionColor()
{
    return selectionColorInt;
}

void MapDesign::setSelectionColor(const QColor &c)
{
    selectionColorInt = c;
}
