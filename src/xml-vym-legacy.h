#ifndef XML_H
#define XML_H

#include "xml-base-legacy.h"

#include "vymnote.h"

class BranchItem;
class ImageItem;
class MapItem;
class SlideItem;
class Task;

/*! \brief Parsing VYM maps from XML documents */

enum Content {
    TreeContent = 0x0001,
    SlideContent = 0x0002,
    XLinkContent = 0x0004
};

class parseVYMHandler : public parseBaseHandler {
  public:
    parseVYMHandler();
    void setContentFilter(const int &);

  private:
    int contentFilter;

  public:
    bool startDocument();
    bool startElement(const QString &, const QString &, const QString &eName,
                      const QXmlAttributes &atts);
    bool endElement(const QString &, const QString &, const QString &);
    bool characters(const QString &);
    QString errorString();
    bool readMapAttr(const QXmlAttributes &);
    bool readBranchAttr(const QXmlAttributes &);
    bool readFrameAttr(const QXmlAttributes &);
    bool readOOAttr(const QXmlAttributes &);
    bool readNoteAttr(const QXmlAttributes &);
    bool readImageAttr(const QXmlAttributes &);
    bool readXLinkAttr(const QXmlAttributes &);
    bool readLinkNewAttr(const QXmlAttributes &);
    bool readSettingAttr(const QXmlAttributes &);
    bool readSlideAttr(const QXmlAttributes &);
    bool readTaskAttr(const QXmlAttributes &);
    bool readUserFlagDefAttr(const QXmlAttributes &);
    bool readUserFlagAttr(const QXmlAttributes &);

  private:
    enum State {
        StateInit,
        StateMap,
        StateMapSelect,
        StateMapSetting,
        StateMapSlide,
        StateMapCenter,
        StateBranch,
        StateBranchXLink, // Obsolete
        StateVymNote,
        StateHtmlNote, // Obsolete >= 1.13.6
        StateHtml,
        StateFrame,
        StateStandardFlag, // New in 2.7.509
        StateUserFlagDef,  // New in 2.7.509
        StateUserFlag,
        StateNote, // Obsolete >= 1.4.6
        StateImage,
        StateHeading,
        StateLink,
        StateAttribute,
        StateTask
    };

    int branchesCounter;
    int branchesTotal;

    State state;
    QList<State> stateStack;
    VymText vymtext;

    BranchItem *lastBranch;
    ImageItem *lastImage;
    MapItem *lastMI;
    SlideItem *lastSlide;
    Task *lastTask;
    QString lastSetting;

    bool useProgress;
};
#endif
