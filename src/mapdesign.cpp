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
    return qlist.last();
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

        // Frames   // FIXME-0 settings not complete yet (also consider dark theme)
        innerFrameTypes << FrameContainer::RoundedRectangle;
        innerFramePenColors;
        innerFrameBrushColors;
        innerFramePenWidths;
        /*
        outerFrameTypes;
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
        qDebug() << "MD::updateBranchHeadingColor " << " d=" << depth << branchItem->getHeadingPlain();
        HeadingColorHint colHint = headingColorHints.tryAt(depth);

        QColor col;
        switch (colHint) {
            case InheritedColor: {
                qDebug() << " - InheritedColor";
                BranchItem *pi = branchItem->parentBranch();
                if (pi) {
                    col = pi->getHeadingColor();
                    break;
                }
                // If there is no parent branch, mapCenter should 
                // have a specific color, thus continue
            }
            case SpecificColor: {
                qDebug() << " - SpecificColor";
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
    // FIXME-00 Hardcoded settings for frames for now, use lists soon:
    if (depth == 0)
        return FrameContainer::RoundedRectangle;
    else
        return FrameContainer::NoFrame;
}

void MapDesign::updateFrames(
    BranchContainer *branchContainer,
    const UpdateMode &mode,
    int depth)
{
    if (branchContainer && mode == NewItem) {
        qDebug() << "MD::updateFrames mode=" << mode << " d=" << depth << branchContainer->getBranchItem()->getHeadingPlain();
        // FIXME-00 Hardcoded settings for frames for now, use lists soon:

        // Inner frame
        FrameContainer::FrameType ftype = frameType(true, depth);
        if (mode == NewItem && depth == 0) {
            branchContainer->setFrameType(true, ftype);
            if (usingDarkTheme) {
                branchContainer->setFramePenColor(true, QColor(Qt::white));
                branchContainer->setFrameBrushColor(true, QColor(85, 85, 127));
            } else {
                    branchContainer->setFramePenColor(true, QColor(Qt::black));
                    branchContainer->setFrameBrushColor(true, QColor(Qt::white));
            }
        }
        // Outer frame
        branchContainer->setFrameType(false, FrameContainer::NoFrame);
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
