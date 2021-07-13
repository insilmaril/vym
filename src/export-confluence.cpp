#include "export-confluence.h"

#include <QMessageBox>

#include "branchobj.h"
#include "confluence-agent.h"
#include "mainwindow.h"
#include "settings.h"
#include "warningdialog.h"
#include "xmlobj.h"

extern QString flagsPath;
extern Main *mainWindow;
extern QString vymName;
extern QString vymVersion;
extern QString vymHome;
extern Settings settings;

ExportConfluence::ExportConfluence() : ExportBase() { init(); }

ExportConfluence::ExportConfluence(VymModel *m) : ExportBase(m) { init(); }

void ExportConfluence::init()
{
    createNewPage = true;
    exportName = "ConfluenceNewPage";

    extension = ".html";
    frameURLs = true;

    url = "";
    pageTitle = "";
}

void ExportConfluence::setCreateNewPage(bool b) {createNewPage = b; }

void ExportConfluence::setURL(const QString &u) { url = u; }

void ExportConfluence::setPageTitle(const QString &t) { pageTitle = t;}

QString ExportConfluence::getBranchText(BranchItem *current)
{
    if (current) {
        QRectF hr;
        LinkableMapObj *lmo = current->getLMO();
        if (lmo) {
            hr = ((BranchObj *)lmo)->getBBoxHeading();
        }
        QString id = model->getSelectString(current);
        QString heading = quoteMeta(current->getHeadingPlain());

        if (dia.useTextColor) {
            QColor c = current->getHeadingColor();
            QString cs = QString("rgb(%1,%2,%3);")
                             .arg(c.red())
                             .arg(c.green())
                             .arg(c.blue());
            heading = QString("<span style='color: %1'>%2</span>")
                          .arg(cs)
                          .arg(heading);
        }

        QString s;
        QString url = current->getURL();

        // Task flags
        QString taskFlags;
        /*
        if (dia.useTaskFlags)
        {
            Task *task = current->getTask();
            if (task)
            {
                QString taskName = task->getIconString();
                taskFlags += QString("<img src=\"flags/flag-%1.png\"
        alt=\"%2\">") .arg(taskName) .arg(QObject::tr("Flag: %1","Alt tag in
        HTML export").arg(taskName));
            }
        }
        */

        // User flags
        QString userFlags;
        /*
        if (dia.useUserFlags)
        {
            foreach (QString flag, current->activeFlagNames())          //
        better don't use activeFlagNames, won't work for userflags userFlags +=
        QString("<img src=\"flags/flag-%1.png\" alt=\"%2\">") .arg(flag)
                    .arg(QObject::tr("Flag: %1","Alt tag in HTML
        export").arg(flag));
        }
        */

        // Numbering
        QString number;
        // if (dia.useNumbering) number = getSectionString(current) + " ";

        // URL
        if (!url.isEmpty()) {
            if (url.contains("ri:userkey"))
                s += url; 
            else {
                if (url.contains(settings.value("/confluence/url",
                                                   "---undefined---").toString()) && url.contains("&")) {

                // Fix ampersands in URL to Confluence itself
                url = quoteMeta(url);
            } 

            s += QString("<a href=\"%1\">%2</a>")
                     .arg(url)
                     .arg(number + taskFlags + heading + userFlags);
            }
        } else
            s += number + taskFlags + heading + userFlags;

        // Include images // FIXME-3 not implemented yet
        /*
        if (dia.includeImages)
        {
            int imageCount = current->imageCount();
            ImageItem *image;
            QString imagePath;
            for (int i=0; i< imageCount; i++)
            {
                image = current->getImageNum(i);
                imagePath =  "image-" + image->getUuid().toString() + ".png";
                image->save( dirPath + "/" + imagePath, "PNG");
                s += "<br /><img src=\"" + imagePath;
                s += "\" alt=\"" + QObject::tr("Image: %1","Alt tag in HTML
        export").arg(image->getOriginalFilename()); s += "\"><br />";
            }
        }
        */

        // Include note
        if (!current->isNoteEmpty()) {
            VymNote note = current->getNote();
            QString n;
            if (note.isRichText()) {
                n = note.getText();
                QRegExp re("<p.*>");
                re.setMinimal(true);
                re.setPattern("</?html>");
                n.replace(re, "");

                re.setPattern("</?head.*>");
                n.replace(re, "");

                re.setPattern("</?body.*>");
                n.replace(re, "");

                re.setPattern("</?meta.*>");
                n.replace(re, "");

                re.setPattern("<style.*>.*</style>");
                n.replace(re, "");

                re.setPattern("<!DOCTYPE.*>");
                n.replace(re,"");

                re.setPattern("&(?!\\w*;)");
                n.replace(re, "&amp;");
            }
            else {
                n = current->getNoteASCII();
                n.replace("&", "&amp;");
                n.replace("<", "&lt;");
                n.replace(">", "&gt;");
                if (current->getNote().getFontHint() == "fixed")
                    n = "<pre>" + n + "</pre>";
                else {
                    n = "<p>" + n + "</p>";
                    n.replace("\n", "</p><p>");
                }
            }

            s += "\n<table class=\"vym-note\"><tr><td>\n" + n +
                 "\n</td></tr></table>\n";
        }
        return s;
    }
    return QString();
}

QString ExportConfluence::buildList(BranchItem *current)
{
    QString r;

    uint i = 0;
    uint visChilds = 0;

    BranchItem *bi = current->getFirstBranch();

    QString ind = "\n" + indent(current->depth() + 1, false);

    QString sectionBegin;
    QString sectionEnd;
    QString itemBegin;
    QString itemEnd;

    switch (current->depth() + 1) {
    case 0:
        sectionBegin = "";
        sectionEnd = "";
        itemBegin = "<h1>";
        itemEnd = "</h1>";
        break;
    case 1:
        sectionBegin = "";
        sectionEnd = "";
        itemBegin = "<h3>";
        itemEnd = "</h3>";
        break;
    case 2:
        sectionBegin = "";
        sectionEnd = "";
        itemBegin = "<h4>";
        itemEnd = "</h4>";
        break;
    default:
        sectionBegin =
            "<ul " +
            QString("class=\"vym-list-ul-%1\"").arg(current->depth() + 1) + ">";
        sectionEnd = "</ul>";
        itemBegin = "  <li>";
        itemEnd = "  </li>";
        break;
    }

    if (bi && !bi->hasHiddenExportParent() && !bi->isHidden()) {
        r += ind + sectionBegin;
        while (bi) {
            if (!bi->hasHiddenExportParent() && !bi->isHidden()) {
                visChilds++;
                r += ind + itemBegin;
                r += getBranchText(bi);

                if (itemBegin.startsWith("<h"))
                    r += itemEnd + buildList(bi);
                else
                    r += buildList(bi) + itemEnd;
            }
            i++;
            bi = current->getBranchNum(i);
        }
        r += ind + sectionEnd;
    }

    return r;
}

QString ExportConfluence::createTOC()
{
    QString toc;
    QString number;
    toc += "<table class=\"vym-toc\">\n";
    toc += "<tr><td class=\"vym-toc-title\">\n";
    toc += QObject::tr("Contents:", "Used in HTML export");
    toc += "\n";
    toc += "</td></tr>\n";
    toc += "<tr><td>\n";
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    model->nextBranch(cur, prev);
    while (cur) {
        if (!cur->hasHiddenExportParent() && !cur->hasScrolledParent()) {
            if (dia.useNumbering)
                number = getSectionString(cur);
            toc +=
                QString("<div class=\"vym-toc-branch-%1\">").arg(cur->depth());
            toc += QString("<a href=\"#%1\"> %2 %3</a><br />\n")
                       .arg(model->getSelectString(cur))
                       .arg(number)
                       .arg(quoteMeta(cur->getHeadingPlain()));
            toc += "</div>";
        }
        model->nextBranch(cur, prev);
    }
    toc += "</td></tr>\n";
    toc += "</table>\n";
    return toc;
}

void ExportConfluence::doExport(bool useDialog)
{
    // Initialize tmp directory below tmp dir of map vym itself
    setupTmpDir();

    filePath = tmpDir.path() + "/export.html";

    // Setup dialog and read settings
    dia.setMapName(model->getMapName());
    dia.setFilePath(model->getFilePath());
    dia.setURL(url);
    dia.setPageTitle(pageTitle);
    dia.readSettings();

    if (useDialog) {
        if (dia.exec() != QDialog::Accepted)
            return;
        model->setChanged();
        url = dia.getURL();
        createNewPage = dia.getCreateNewPage();
        pageTitle = dia.getPageTitle();
    }

    // Open file for writing
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            0, QObject::tr("Critical Export Error"),
            QObject::tr("Trying to save HTML file:") + "\n\n" +
                QObject::tr("Could not write %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }
    QTextStream ts(&file);
    ts.setCodec("UTF-8");

    // Hide stuff during export
    model->setExportMode(true);

    // Include image
    // (be careful: this resets Export mode, so call before exporting branches)
    /*
    if (dia.includeMapImage)
    {
        QString mapName = getMapName();
        ts << "<center><img src=\"" << mapName << ".png\"";
        ts << "alt=\"" << QObject::tr("Image of map: %1.vym","Alt tag in HTML
    export").arg(mapName) << "\""; ts << " usemap='#imagemap'></center>\n";
        offset = model->exportImage (dirPath + "/" + mapName + ".png", false,
    "PNG");
    }
    */

    // Include table of contents
    // if (dia.useTOC) ts << createTOC();

    // Main loop over all mapcenters
    ts << buildList(model->getRootItem()) << "\n";

    ts << "<p style=\"text-align: center;\"> <sub> <em>Page created with ";
    ts << "<a href=\"https://sourceforge.net/projects/vym/\">" << vymName << " " << vymVersion<< "</a>";
    ts << "</em> </sub> </p>";

    file.close();

    // Create Confluence agent
    ConfluenceAgent *agent = new ConfluenceAgent();
    if (createNewPage)
        agent->setJobType(ConfluenceAgent::NewPage);
    else
        agent->setJobType(ConfluenceAgent::UpdatePage);
    agent->setPageURL(url);
    agent->setNewPageTitle(pageTitle);
    agent->setUploadFilePath(filePath);
    agent->setModelID(model->getModelID());
    agent->startJob();


    // Old stuff below, replace with native Confluence agent
    if (false)
    {
        // First check if page already exists
        ConfluenceAgent *ca_details = new ConfluenceAgent();
        ConfluenceAgent *ca_content = new ConfluenceAgent();

        mainWindow->statusMessage(
            QObject::tr("Trying to read Confluence page details...", "Confluence export"));
        qApp->processEvents();

        if (ca_details->getPageDetails(url)) {
            ca_details->waitForResult();

            if (ca_details->success()) {
                // Page with URL is existing already
                if (createNewPage) {
                    // URL exists and is parent page
                    if(debug) qDebug() << "Starting to create new page..."; 
                    ca_content->createPage(url, pageTitle,
                                           filePath);
                    ca_content->waitForResult();
                    if (ca_content->success()) {
                        if (debug) qDebug() << "Page created.";
                        success = true;
                    }
                    else {
                        if (debug) qDebug() << "Page not created.";
                        success = false;
                    }
                } else {
                    // URL exists and is update page
                    if (debug) qDebug() << "Starting to update existing page...";
                    mainWindow->statusMessage(
                        QObject::tr("Trying to update Confluence page...", "Confluence export"));
                    qApp->processEvents();

                    ca_content->updatePage(url, pageTitle,
                                           filePath);
                    ca_content->waitForResult();
                    if (ca_content->success()) {
                        if (debug) qDebug() << "Page updated.";
                        success = true;
                    }
                    else {
                        if (debug) {
                            qWarning() << "Page not updated:";
                            qWarning() << ca_content->getResult();
                        }
                        success = false;
                    }
                }
            }
            else {
                // Page with URL does not exist, abort
                // neither as parent of new page or to update existing page
                success = false;
                if (createNewPage)
                    qWarning() << "Parent page not existing: " << url;
                else
                    qWarning() << "Page not existing, cannot update it: "
                               << url;
            }
        }

        delete (ca_details);
        delete (ca_content);
    } // Old code

    displayedDestination = url;

    QStringList args;
    createNewPage ? exportName = "ConfluenceNewPage" : exportName = "ConfluenceUpdatePage";
    args <<  displayedDestination;
    args <<  pageTitle;
    completeExport(args);

    dia.saveSettings();
    model->setExportMode(false);
}
