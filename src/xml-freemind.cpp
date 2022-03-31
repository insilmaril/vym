#include "xml-freemind.h"

#include <QDebug>
#include <QMessageBox>

#include "branchitem.h"
#include "misc.h"
#include "vymmodel.h"

extern Settings settings;
extern QString vymVersion;

bool parseFreemindHandler::nodeLikeState() 
{
    if (state == StateCenter || state == StateMainNode || state == StateNode)
        return true;
    else
        return false;
}

bool parseFreemindHandler::startDocument() // FIXME-3 import not complete yet
{
    errorProt = "";
    state = StateInit;
    stateStack.clear();
    stateStack.append(StateInit);
    isVymPart = false;
    return true;
}

QString parseFreemindHandler::parseHREF(QString href)
{
    QString type = href.section(":", 0, 0);
    QString path = href.section(":", 1, 1);
    if (!tmpDir.endsWith("/"))
        return tmpDir + "/" + path;
    else
        return tmpDir + path;
}

bool parseFreemindHandler::startElement(const QString &, const QString &,
                                        const QString &eName,
                                        const QXmlAttributes &atts)
{
    QColor col;
    /* Testing
    if (eName.contains("node"))
    qDebug() << "startElement: <" << eName
             << ">     state=" << state
             << "  laststate=" << stateStack.last();
             //<< "   loadMode=" << loadMode
             //<<"       line=" << QXmlDefaultHandler::lineNumber();
    */

    stateStack.append(state);
    if (state == StateInit && (eName == "map")) {
        state = StateMap;
        if (!atts.value("version").isEmpty()) {
            QString v = "0.9.0";
            if (!versionLowerOrEqual(atts.value("version"), v))
                QMessageBox::warning(
                    0, "Warning: Version Problem",
                    "<h3>Freemind map is newer than version " + v +
                        " </h3>"
                        "<p>The map you are just trying to load was "
                        "saved using freemind " +
                        atts.value("version") +
                        ". "
                        "Your version of vym can parse freemind " +
                        v + ".");
        }
        // Create mapcenter
        model->clear();
        mapCenter = model->createMapCenter();
        lastBranch = mapCenter;

        // Create two hidden branches, because Freemind has no relative
        // positioning for mainbranches
        mainBranchLeft = model->createBranch(lastBranch);
        mainBranchRight = model->createBranch(lastBranch);

        mainBranchLeft->setPos(QPointF(-200, 0));
        mainBranchLeft->setHeadingPlainText(" ");
        model->emitDataChanged(mainBranchLeft);

        mainBranchRight->setPos(QPointF(200, 0));
        mainBranchRight->setHeadingPlainText(" ");
        model->emitDataChanged(mainBranchRight);
    }
    else if (eName == "attribute_registry" && state == StateMap) {
        state = StateAttributeRegistry;
    }
    else if (eName == "attribute_name" && state == StateAttributeRegistry) {
        state = StateAttributeName;
    }
    else if (eName == "attribute_value" && state == StateAttributeName) {
        state = StateAttributeValue;
    }
    else if (eName == "node" && state == StateMap) {
        state = StateCenter;
        lastBranch = mapCenter;
        readNodeAttr(atts);
    }
    else if (eName == "node" && state == StateCenter) {
        state = StateMainNode;
        lastBranch = model->createBranch(lastBranch);
        readNodeAttr(atts);
    }
    else if (eName == "node" && (state == StateNode || state == StateMainNode)) {
        state = StateNode;
        lastBranch = model->createBranch(lastBranch);
        readNodeAttr(atts);
    }
    else if (eName == "properties") {
    }
    else if (eName == "map_styles") {
    }
    else if (eName == "font" && nodeLikeState())
    { 
        // FIXME-3 not implemented
        state = StateFont;
    }
    else if (eName == "edge" && nodeLikeState())
    {
         // FIXME-3 xlink not implemented
        state = StateEdge;
    }
    else if (eName == "hook" && nodeLikeState()) {
        state = StateHook;
    }
    else if (eName == "icon" && nodeLikeState()) {
        state = StateIcon;
        if (!atts.value("BUILTIN").isEmpty()) {
            QString f = atts.value("BUILTIN");
            QString v;
            if (f == "help")
                v = "questionmark";
            else if (f == "messagebox_warning")
                v = "freemind-warning";
            else if (f == "idea")
                v = "lamp";
            else if (f == "button_ok")
                v = "hook-green";
            else if (f == "button_cancel")
                v = "cross-red";
            else if (f.contains("full-"))
                v = f.replace("full-", "freemind-priority-");
            else if (f == "back")
                v = "freemind-back";
            else if (f == "forward")
                v = "freemind-forward";
            else if (f == "attach")
                v = "freemind-attach";
            else if (f == "ksmiletris")
                v = "smiley-good"; //
            else if (f == "clanbomber")
                v = "freemind-clanbomber";
            else if (f == "desktop_new")
                v = "freemind-desktopnew";
            else if (f == "flag")
                v = "freemind-flag";
            else if (f == "gohome")
                v = "freemind-gohome";
            else if (f == "kaddressbook")
                v = "freemind-kaddressbook";
            else if (f == "knotify")
                v = "freemind-knotify";
            else if (f == "korn")
                v = "freemind-korn";
            else if (f == "Mail")
                v = "freemind-mail";
            else if (f == "password")
                v = "freemind-password";
            else if (f == "pencil")
                v = "freemind-pencil";
            else if (f == "stop")
                v = "freemind-stop";
            else if (f == "wizard")
                v = "freemind-wizard";
            else if (f == "xmag")
                v = "freemind-xmag";
            else if (f == "bell")
                v = "freemind-bell";
            else if (f == "bookmark")
                v = "freemind-bookmark";
            else if (f == "penguin")
                v = "freemind-penguin";
            else if (f == "licq")
                v = "freemind-licq";
            else
                qWarning() << "parseFreemindHandler: Unknown icon found: " << f;

            lastBranch->activateStandardFlagByName(v);
        }
    }
    else if (eName == "arrowlink" && nodeLikeState()) {
        state = StateArrowLink;
    }
    else if (eName == "cloud" && nodeLikeState()) {
        state = StateCloud;
    }
    else if (eName == "richcontent" && nodeLikeState()) {
        state = StateRichContent;
        return readRichContentAttr(atts);
    }
    else if (eName == "html" && state == StateRichContent) {
        state = StateHtml;
        htmldata = "<" + eName;
        readHtmlAttr(atts);
        htmldata += ">";
    }
    else if (eName == "text" && state == StateHook) {
        state = StateText;
    }
    else if (state == StateHtml) {
        // accept all while in html mode,
        htmldata += "<" + eName;
        readHtmlAttr(atts);
        htmldata += ">";
    }
    else {
        // Usually we would return an error here
        // In order to not break with every new element in FreeMind or
        // FreePlane, better ignore the new element for now
        state = StateUnknown;
        qWarning()
            << "parseFreemindHandler: Unknown element found and ignoring:: "
            << eName;
    }
    return true;
}

bool parseFreemindHandler::endElement(const QString &, const QString &,
                                      const QString &eName)
{
    /* Testing
    QString h;
    lastBranch ? h = lastBranch->getHeadingPlain() : h = "";
    if (eName.contains("node"))
    qDebug() << "endElement </" << eName << ">  state=" << state << " lastBranch=" << h;
    */

    switch (state) {
        case StateMap:
            // Freemind does not have the two "extra" mainbranches used here,
            // so we have to update mapcenter
            model->emitDataChanged(mapCenter);

            // Remove helper branches, if not needed
            if (mainBranchLeft->childCount() == 0)
                model->deleteItem(mainBranchLeft);
            if (mainBranchRight->childCount() == 0)
                model->deleteItem(mainBranchRight);
            break;
        case StateCenter:
        case StateMainNode:
        case StateNode:
            model->emitDataChanged(lastBranch);
            lastBranch = (BranchItem *)lastBranch->parent();
            if (lastBranch) 
                lastBranch->setLastSelectedBranch(0);
            else
                qWarning() << "xml-vym:  lastBranch == nullptr";
            break;
        case StateRichContent:
            if (!htmldata.isEmpty()) {
                vymtext.setAutoText(htmldata);
                if (htmlPurpose == Node)
                    lastBranch->setHeading(vymtext);
                else if (htmlPurpose == Note)
                    lastBranch->setNote(vymtext);
            }
            break;
        case StateHtml:
            htmldata += "</" + eName + ">";
            if (eName == "html") {
                htmldata = htmldata.replace("<br></br>", "<br />");
                htmldata = htmldata.replace("\\n</p>", "</p>");
            }
            break;
        default:
            break;
    }
    state = stateStack.takeLast();
    return true;
}

bool parseFreemindHandler::characters(const QString &ch)
{
    //qDebug() << "characters \"" << qPrintable(ch) << "\"  state=" << state;

    QString ch_org = quoteMeta(ch);
    QString ch_simplified = ch.simplified();
    if (ch_simplified.isEmpty())
        return true;

    switch (state) {
    case StateInit:
        break;
    case StateMap:
        break;
    case StateNode:
        break;
    case StateCloud:
        break;
    case StateEdge:
        break;
    case StateIcon:
        break;
    case StateArrowLink:
        break;
    case StateFont:
        break;
    case StateHook:
        break;
    case StateText:
        lastBranch->setNote(ch_simplified);
        break;
    case StateHtml:
        htmldata += ch_org;
        break;
    case StateUnknown:
        break;
    default:
        return false;
    }
    return true;
}

QString parseFreemindHandler::errorString()
{
    return "the document is not in the Freemind file format";
}

bool parseFreemindHandler::readNodeAttr(const QXmlAttributes &a)
{
    if (state == StateMainNode)
    {
        // Freemind has a different concept for mainbranches
        // Move either to left or right side of mapcenter
        if (!a.value("POSITION").isEmpty()) {
            if (a.value("POSITION") == "left")
                model->relinkBranch(lastBranch, mainBranchLeft);
            else if (a.value("POSITION") == "right")
                model->relinkBranch(lastBranch, mainBranchRight);
        }
    }

    if (a.value("FOLDED") == "true")
        lastBranch->toggleScroll();

    if (!a.value("TEXT").isEmpty()) {
        lastBranch->setHeadingPlainText(
            a.value("TEXT")); // FIXME-3 what about RT?
        // model->setHeading (a.value ("TEXT"), lastBranch);
    }

    if (!a.value("COLOR").isEmpty())
        lastBranch->setHeadingColor(QColor(a.value("COLOR")));

    if (!a.value("LINK").isEmpty())
        lastBranch->setURL(a.value("LINK"));
    return true;
}

bool parseFreemindHandler::readRichContentAttr(const QXmlAttributes &a)
{
    if (a.value("TYPE") == "NODE")
        htmlPurpose = Node;
    else if (a.value("TYPE") == "NOTE")
        htmlPurpose = Note;
    else {
        htmlPurpose = Unknown;
        qWarning()
            << "parseFreemindHandler: Unknown purpose of richContent found: "
            << a.value("TYPE");
        ;
        // FIXME-3 Usually we would stop here, ignore for now
        // return false;
    }
    return true;
}
