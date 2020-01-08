#include "export-confluence.h"

#include <QMessageBox>

#include "confluence-agent.h"
#include "branchobj.h"
#include "mainwindow.h"
#include "settings.h"
#include "warningdialog.h"

extern QString flagsPath;
extern Main *mainWindow;
extern QString vymVersion;
extern QString vymHome;
extern Settings settings;

ExportConfluence::ExportConfluence():ExportBase()
{
    init();
}

ExportConfluence::ExportConfluence(VymModel *m):ExportBase(m)
{
    init();
}

void ExportConfluence::init()
{
    exportName = "Confluence";
    extension  = ".html";
    frameURLs  = true;

    pageURL    = "";
    pageTitle  = "";

}

void ExportConfluence::setPageURL(const QString &u)
{
    pageURL = u;
}
void ExportConfluence::setPageTitle(const QString &t)
{
    pageTitle = t;
}

QString ExportConfluence::getBranchText(BranchItem *current)
{
    if (current)
    {
        bool vis = false;
        QRectF hr;
        LinkableMapObj *lmo = current->getLMO();
        if (lmo)
        {
            hr = ((BranchObj*)lmo)->getBBoxHeading();
            vis = lmo->isVisibleObj();
        }
        QString col;
        QString id = model->getSelectString(current);
        if (dia.useTextColor)
            col = QString("style='color:%1'").arg(current->getHeadingColor().name());
        QString s;
        QString url = current->getURL();
        QString heading = quotemeta(current->getHeadingPlain());

        // Task flags
        QString taskFlags;
        if (dia.useTaskFlags)
        {
            Task *task = current->getTask();
            if (task)
            {
                QString taskName = task->getIconString();
                taskFlags += QString("<img src=\"flags/flag-%1.png\" alt=\"%2\">")
                    .arg(taskName)
                    .arg(QObject::tr("Flag: %1","Alt tag in HTML export").arg(taskName));
            }
        }

        // User flags
        QString userFlags;
        if (dia.useUserFlags)
        {
            foreach (QString flag, current->activeStandardFlagNames())
                userFlags += QString("<img src=\"flags/flag-%1.png\" alt=\"%2\">")
                    .arg(flag)
                    .arg(QObject::tr("Flag: %1","Alt tag in HTML export").arg(flag));
        }

        // Numbering
        QString number;
        //if (dia.useNumbering) number = getSectionString(current) + " ";
        
        // URL
        if (!url.isEmpty())
        {
            s += QString ("<a href=\"%1\">%2<img src=\"flags/flag-url.png\" alt=\"%3\"></a>")
                    .arg(url)
                    .arg(number + taskFlags + heading + userFlags)
                    .arg(QObject::tr("Flag: url","Alt tag in HTML export"));

            QRectF fbox = current->getBBoxURLFlag ();
            if (vis)
                imageMap += QString("  <area shape='rect' coords='%1,%2,%3,%4' href='%5' alt='%6'>\n")
                        .arg(fbox.left()   - offset.x())
                        .arg(fbox.top()    - offset.y())
                        .arg(fbox.right()  - offset.x())
                        .arg(fbox.bottom() - offset.y())
                        .arg(url)
                        .arg(QObject::tr("External link: %1","Alt tag in HTML export").arg(heading));
        } else
            s += number + taskFlags + heading + userFlags;

        // Create imagemap
        if (vis && dia.includeMapImage)
            imageMap += QString("  <area shape='rect' coords='%1,%2,%3,%4' href='#%5' alt='%6'>\n")
                    .arg(hr.left()   - offset.x())
                    .arg(hr.top()    - offset.y())
                    .arg(hr.right()  - offset.x())
                    .arg(hr.bottom() - offset.y())
                    .arg(id)
                    .arg(heading);

        // Include images experimental
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
                s += "</br><img src=\"" + imagePath;
                s += "\" alt=\"" + QObject::tr("Image: %1","Alt tag in HTML export").arg(image->getOriginalFilename());
                s += "\"></br>";
            }
        }

        // Include note
        if (!current->isNoteEmpty())
        {
            VymNote  note = current->getNote();
            QString n;
            if (note.isRichText())
            {
                n = note.getText();
                QRegExp re("<p.*>");
                re.setMinimal (true);
                if (current->getNote().getFontHint() == "fixed")
                    n.replace(re,"<p class=\"vym-fixed-note-paragraph\">");
                else
                    n.replace(re,"<p class=\"vym-note-paragraph\">");

                re.setPattern("</?html>");
                n.replace(re,"");

                re.setPattern("</?head.*>");
                n.replace(re,"");

                re.setPattern("</?body.*>");
                n.replace(re,"");

                re.setPattern("</?meta.*>");
                n.replace(re,"");

                re.setPattern("<style.*>.*</style>");
                n.replace(re,"");

                //re.setPattern("<!DOCTYPE.*>");
                //n.replace(re,"");
            }
            else
            {
                n = current->getNoteASCII().replace ("<","&lt;").replace (">","&gt;");
                n.replace("\n","<br/>");
                if (current->getNote().getFontHint()=="fixed")
                    n = "<pre>" + n + "</pre>";
            } 
            s += "\n<table class=\"vym-note\"><tr><td class=\"vym-note-flag\">\n<td>\n" + n + "\n</td></tr></table>\n";
        }
        return s;
    }
    return QString();
}

QString ExportConfluence::buildList (BranchItem *current)
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

    switch (current->depth() + 1)
    {
    case 0:
        sectionBegin = "";
        sectionEnd   = "";
        itemBegin    = "<h1>";
        itemEnd      = "</h1>";
        break;
    case 1:
        sectionBegin = "";
        sectionEnd   = "";
        itemBegin    = "<h3>";
        itemEnd      = "</h3>";
        break;
    default:
        sectionBegin = "<ul " + QString("class=\"vym-list-ul-%1\"").arg(current->depth() + 1)  +">";
        sectionEnd   = "</ul>";
        itemBegin    = "  <li>";
        itemEnd      = "  </li>";
        break;
    }
    
    if (bi && !bi->hasHiddenExportParent() && !bi->isHidden() )
    {
        r += ind + sectionBegin;
        while (bi)
        {
            if (!bi->hasHiddenExportParent() && !bi->isHidden())
            {
                visChilds++;
                r += ind + itemBegin;
                r += getBranchText (bi);

                if (itemBegin.startsWith("<h") )
                    r += itemEnd + buildList (bi);
                else
                    r += buildList (bi) + itemEnd;
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
    toc += QObject::tr("Contents:","Used in HTML export");
    toc += "\n";
    toc += "</td></tr>\n";
    toc += "<tr><td>\n";
    BranchItem *cur  = NULL;
    BranchItem *prev = NULL;
    model->nextBranch(cur, prev);
    while (cur)
    {
        if (!cur->hasHiddenExportParent() && !cur->hasScrolledParent() )
        {
            if (dia.useNumbering) number = getSectionString(cur);
            toc += QString("<div class=\"vym-toc-branch-%1\">").arg(cur->depth());
            toc += QString("<a href=\"#%1\"> %2 %3</a></br>\n")
                    .arg(model->getSelectString(cur))
                    .arg(number)
                    .arg(quotemeta( cur->getHeadingPlain() ));
            toc += "</div>";
        }
        model->nextBranch(cur,prev);
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
    dia.setMapName (model->getMapName());
    dia.setFilePath (model->getFilePath()); 
    dia.setPageURL( pageURL );
    dia.setPageTitle( pageTitle );
    dia.readSettings();

    if (useDialog)
    {
        if (dia.exec() != QDialog::Accepted) return;
        model->setChanged();
    }

    // Open file for writing
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical (0,
                               QObject::tr("Critical Export Error"),
                               QObject::tr("Trying to save HTML file:") + "\n\n"+
                               QObject::tr("Could not write %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }
    QTextStream ts( &file );
    ts.setCodec("UTF-8");

    // Hide stuff during export
    model->setExportMode (true);

    // Include image
    // (be careful: this resets Export mode, so call before exporting branches)
    /*
    if (dia.includeMapImage)
    {
        QString mapName = getMapName();
        ts << "<center><img src=\"" << mapName << ".png\"";
        ts << "alt=\"" << QObject::tr("Image of map: %1.vym","Alt tag in HTML export").arg(mapName) << "\"";
        ts << " usemap='#imagemap'></center>\n";
        offset = model->exportImage (dirPath + "/" + mapName + ".png", false, "PNG");
    }
    */

    // Include table of contents
    //if (dia.useTOC) ts << createTOC();

    // Main loop over all mapcenters
    ts << buildList(model->getRootItem()) << "\n";

    // Imagemap
    //ts << "<map name='imagemap'>\n" + imageMap + "</map>\n";

    file.close();

    // First check if page already exists
    ConfluenceAgent *ca_details = new ConfluenceAgent (model);
    ConfluenceAgent *ca_content = new ConfluenceAgent (model);

    mainWindow->statusMessage(QObject::tr("Trying to read Confluence page...","Confluence export"));
    if (ca_details->getPageDetails( dia.getPageURL() ) )
    {
        ca_details->waitForResult();

        if (ca_details->success() ) 
        {
            // Page is existing already
            if (dia.createNewPage() )
            {
                qDebug() << "Wanted to create new page, but page already exists. Aborted";
            } else
            {
                qDebug() << "Starting to update existing page...";
                ca_content->updatePage( dia.getPageURL(), dia.getPageTitle(), filePath);
                ca_content->waitForResult();
                if (ca_content->success() ) 
                {
                    qDebug() << "Page updated.";
                    success = true;
                } else
                {
                    qDebug() << "Page not updated.";
                }
            } 
        } else
        {
            // Page not existing yet
            if (dia.createNewPage() )
            {
                qDebug() << "Starting to create new page...";
                ca_content->createPage( dia.getPageURL(), dia.getPageTitle(), filePath);
                ca_content->waitForResult();
                if (ca_content->success() ) 
                {
                    qDebug() << "Page created.";
                    success = true;
                } else
                {
                    qDebug() << "Page not created.";
                }
            }
        }
    }

    delete (ca_details);
    delete (ca_content);

    destination = dia.getPageURL();

    QMap <QString, QString> args;
    args["pageURL"]   = destination;
    args["pageTitle"] = dia.getPageTitle();
    completeExport( args );

    dia.saveSettings();
    model->setExportMode (false);
}

