#include "export-html.h"

#include <QMessageBox>

#include "branch-container.h"
#include "branchitem.h"
#include "heading-container.h"
#include "image-container.h"
#include "mainwindow.h"
#include "misc.h"
#include "task.h"
#include "vymmodel.h"
#include "warningdialog.h"

extern QString flagsPath;
extern Main *mainWindow;
extern QString vymVersion;
extern QString vymHome;

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;

ExportHTML::ExportHTML() : ExportBase() { init(); }

ExportHTML::ExportHTML(VymModel *m) : ExportBase(m) { init(); }

void ExportHTML::init()
{
    exportName = "HTML";
    extension = ".html";
    frameURLs = true;

    flagWidthInt = " width=\"32\" ";   // FIXME-3 use CSS instead
}

QString ExportHTML::getBranchText(BranchItem *current)
{
    if (current) {
        bool vis = false;
        QRectF hr;
        BranchContainer *bc = current->getBranchContainer();
        HeadingContainer *hc = bc->getHeadingContainer();
        hr = hc->mapRectToScene(hc->rect());
        vis = hc->isVisible();

        QString col;
        QString id = model->getSelectString(current);
        if (dia.useTextColor)
            col = QString("style='color:%1'")
                      .arg(current->headingColor().name());
        QString s = QString("<div class='vym-branch-%1' %2 id='%3'>")
                        .arg(current->depth())
                        .arg(col)
                        .arg(id);
        QString url = current->url();
        QString heading = quoteMeta(current->headingPlain());

        // Task flags
        QString taskFlags;
        QString flagName;;
        QString flagPath;
        if (dia.useTaskFlags) {
            Task *task = current->getTask();
            if (task) {
                QString taskName = task->iconString();
                Flag *f = current->taskFlag();
                if (f) {
                    flagName = f->getName();
                    ImageContainer *ic = f->getImageContainer();
                    if (!ic) {
                        qWarning() << __func__ << "ic == nullptr for task";
                        return "Error...";
                    }

                    flagPath = flagsDir.path() + "/" + ic->originalFilename();

                    if (!activeSystemFlagNames.contains(flagName)) {
                        activeSystemFlagNames << flagName;
                        ic->save(flagPath);
                    }
                    taskFlags +=
                        QString("<img style=\"vertical-align: middle\" %1 src=\"flags/%2\" alt=\"%3\">")
                            .arg(flagWidthInt, basename(flagPath), taskName);
                }
            }
        }

        // Standard and user flags
        QString flags;
        if (dia.useUserFlags) {
            Flag *f;
            foreach (QUuid uid, current->activeFlagUids()) {
                activeFlags << uid;

                f = standardFlagsMaster->findFlagByUid(uid);
                if (!f)
                    f = userFlagsMaster->findFlagByUid(uid);

                if (f)
                    flags +=
                        QString(
                            "<img style=\"vertical-align: middle\" %1 alt=\"%2\" src=\"flags/%3\">")
                            .arg(flagWidthInt)
                            .arg(QObject::tr("Flag: %1", "Alt tag in HTML export").arg(f->getName()))
                            .arg(f->getImageContainer()->originalFilename());
            }
        }

        // Numbering
        QString number;
        if (dia.useNumbering)
            number = getSectionString(current) + " ";

        // Url
        if (!url.isEmpty()) {
            Flag *f = current->urlFlag();
            if (f) {
                flagName = f->getName();
                ImageContainer *ic = f->getImageContainer();
                if (!ic) {
                    qWarning() << __func__ << "ic == nullptr for Url";
                    return "Error...";
                }

                flagPath = flagsDir.path() + "/" + ic->originalFilename();

                if (!activeSystemFlagNames.contains(flagName)) {
                    activeSystemFlagNames << flagName;
                    ic->save(flagPath);
                }
            }
            s += QString("<a href=\"%1\">%2<img style=\"vertical-align: middle\" %3 src=\"flags/%4\" "
                         "alt=\"%5\"></a>")
                     .arg(url)
                     .arg(number + taskFlags + heading + flags)
                     .arg(flagWidthInt)
                     .arg(basename(flagPath))
                     .arg(QObject::tr("Flag: url", "Alt tag in HTML export"));

            QRectF fbox = current->getBranchContainer()->getBBoxURLFlag();
            if (vis)
                imageMap += QString("  <area shape='rect' coords='%1,%2,%3,%4' "
                                    "href='%5' alt='External link: %6'>\n")
                                .arg(fbox.left() - offset.x())
                                .arg(fbox.top() - offset.y())
                                .arg(fbox.right() - offset.x())
                                .arg(fbox.bottom() - offset.y())
                                .arg(url)
                                .arg(heading);

        }
        else
            s += number + taskFlags + heading + flags;

        s += "</div>";

        // Create imagemap
        if (vis && dia.includeMapImage) // FIXME-3 maybe use polygons instead of QRectF for shapes
                                        // shape = "poly" coords="x1,y1,x2,y2,..."
            imageMap += QString("  <area shape='rect' coords='%1,%2,%3,%4' "
                                "href='#%5' alt='%6'>\n")
                            .arg(hr.left() - offset.x())
                            .arg(hr.top() - offset.y())
                            .arg(hr.right() - offset.x())
                            .arg(hr.bottom() - offset.y())
                            .arg(id)
                            .arg(heading);

        // Include images experimental
        if (dia.includeImages) {
            int imageCount = current->imageCount();
            ImageItem *image;
            QString imageName;
            for (int i = 0; i < imageCount; i++) {
                image = current->getImageNum(i);
                imageName = image->getUniqueFilename();
                image->saveImage(dirPath + "/" + imageName);
                s += "</br><img src=\"" + imageName;
                s += "\" alt=\"" +
                     QObject::tr("Image: %1", "Alt tag in HTML export")
                         .arg(image->originalFilename());
                s += "\"></br>";
            }
        }

        // Include note
        if (!current->isNoteEmpty()) {
            VymNote note = current->getNote();
            QString n;
            if (note.isRichText()) {
                n = note.getText();
                QRegularExpression re("<p.*>");
                re.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
                if (current->getNote().getFontHint() == "fixed")
                    n.replace(re, "<p class=\"vym-fixed-note-paragraph\">");
                else
                    n.replace(re, "<p class=\"vym-note-paragraph\">");

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

                // re.setPattern("<!DOCTYPE.*>");
                // n.replace(re,"");
            }
            else {
                n = current->getNoteASCII(0, 0)
                        .replace("<", "&lt;")
                        .replace(">", "&gt;");
                n.replace("\n", "<br/>");
                if (current->getNote().getFontHint() == "fixed")
                    n = "<pre>" + n + "</pre>";
            }
            s += "\n<button class=\"vym-note-button\">&#9654; Note</button>\n"
                 "<div class=\"vym-note-collapsible\"><div class=\"vym-note\">\n" +
                 n +
                 "\n</div></div>\n";
        }
        return s;
    }
    return QString();
}

QString ExportHTML::buildList(BranchItem *current)
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
        itemBegin = "<h2>";
        itemEnd = "</h2>";
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

    if (bi && !bi->hasHiddenParent() && !bi->isHidden()) {
        r += ind + sectionBegin;
        while (bi) {
            if (!bi->hasHiddenParent() && !bi->isHidden()) {
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

QString ExportHTML::createTOC()
{
    QString toc;
    QString number;
    toc += "<table class=\"vym-toc\">\n";
    toc += "<tr><td class=\"vym-toc-title\">\n";
    toc += QObject::tr("Contents:", "Used in HTML export");
    toc += "\n";
    toc += "</td></tr>\n";
    toc += "<tr><td>\n";
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    model->nextBranch(cur, prev);
    while (cur) {
        if (!cur->hasHiddenParent() && !cur->hasScrolledParent()) {
            if (dia.useNumbering)
                number = getSectionString(cur);
            toc +=
                QString("<div class=\"vym-toc-branch-%1\">").arg(cur->depth());
            toc += QString("<a href=\"#%1\"> %2 %3</a></br>\n")
                       .arg(model->getSelectString(cur))
                       .arg(number)
                       .arg(quoteMeta(cur->headingPlain()));
            toc += "</div>";
        }
        model->nextBranch(cur, prev);
    }
    toc += "</td></tr>\n";
    toc += "</table>\n";
    return toc;
}

void ExportHTML::doExport(bool useDialog)
{
    // Setup dialog and read settings
    dia.setMapName(model->getMapName());
    dia.setFilePath(model->getFilePath());
    dia.readSettings();

    if (dirPath != defaultDirPath)
        dia.setDirectory(dirPath);

    if (useDialog) {
        if (dia.exec() != QDialog::Accepted)
            return;
        model->setChanged();
    }

    // Check, if warnings should be used before overwriting
    // the output directory
    if (dia.getDir().exists() && dia.getDir().entryList(QDir::NoDot | QDir::NoDotDot).count() > 0) {
        WarningDialog warn;
        warn.showCancelButton(true);
        warn.setText(QString("The directory %1 is not empty.\n"
                             "Do you risk to overwrite some of its contents?")
                         .arg(dia.getDir().absolutePath()));
        warn.setCaption("Warning: Directory not empty");
        warn.setShowAgainName("mainwindow/export-XML-overwrite-dir");

        if (warn.exec() != QDialog::Accepted) {
            mainWindow->statusMessage(QString(QObject::tr("Export aborted.")));
            return;
        }
    }

    dirPath = dia.getDir().absolutePath();
    filePath = getFilePath();

    // Copy CSS file
    if (dia.css_copy) {
        cssSrc = dia.getCssSrc();
        cssDst = dirPath + "/" + basename(cssSrc);
        if (cssSrc.isEmpty()) {
            QMessageBox::critical(
                0, QObject::tr("Critical"),
                QObject::tr("Could not find stylesheet %1").arg(cssSrc));
            return;
        }
        QString cssContent;
        if (!loadStringFromDisk(cssSrc, cssContent)) {
            QMessageBox::critical(
                0, QObject::tr("Error", "ExportHTML"),
                QObject::tr("Could not read \n%1", "ExportHTML")
                    .arg(cssSrc));
            return;
        }

        // Replace mapBackgroundColor placeholder with actual map background color
        cssContent.replace("$MapBackgroundColor", model->backgroundColor().name());

        QFile dst(cssDst);
        if (dst.exists())
            dst.remove();

        if (!saveStringToDisk(cssDst, cssContent)) {
            QMessageBox::critical(
                0, QObject::tr("Error", "ExportHTML"),
                QObject::tr("Could not write to \n%1", "ExportHTML")
                    .arg(cssDst));
            return;
        }
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

    // Hide stuff during export
    model->setExportMode(true);

    // Write header
    ts << "<html>";
    ts << "\n<meta http-equiv=\"content-type\" content=\"text/html; "
          "charset=UTF-8\"> ";
    ts << "\n<meta name=\"generator=\" content=\" vym - view your mind - " +
              vymVersion + " - " + vymHome + "\">";
    ts << "\n<meta name=\"author\" content=\"" + quoteMeta(model->mapAuthor()) +
              "\"> ";
    ts << "\n<meta name=\"description\" content=\"" +
              quoteMeta(model->mapComment()) + "\"> ";
    ts << "\n<link rel='stylesheet' id='css.stylesheet' href='"
       << basename(cssDst) << "' />\n";
    QString title = model->mapTitle();
    if (title.isEmpty())
        title = model->getMapName();
    ts << "\n<head><title>" + quoteMeta(title) + "</title></head>";
    ts << "\n<body>\n";

    // Include image
    // (be careful: this resets Export mode, so call before exporting branches)
    if (dia.includeMapImage) {
        // Workaround to preserve mapChanged status,
        // which otherwise might become true because of 
        // implicit image export
        setBlockMapChangedDuringExport(true);

        QString mapName = getMapName();
        ts << "<center><img src=\"" << mapName << ".png\"";
        ts << "alt=\""
           << QString("Image of map: %1.vym").arg(mapName)
           << "\"";
        ts << " usemap='#imagemap'></center>\n";
        offset =
            model->exportImage(dirPath + "/" + mapName + ".png", false, "PNG");

        setBlockMapChangedDuringExport(false);
    }

    // Include table of contents
    if (dia.useTOC)
        ts << createTOC();

    // reset flags
    model->resetUsedFlags();
    flagsDir.setPath(dia.getDir().absolutePath() + "/flags");
    if (!flagsDir.exists()) {
        if (!dia.getDir().mkdir("flags")) {
            QMessageBox::critical(
                0, QObject::tr("Critical"),
                QObject::tr("Trying to create directory for flags:") + "\n\n" +
                    QObject::tr("Could not create %1").arg(flagsDir.path()));
            return;
        }
    }
    // Main loop over all mapcenters
    ts << buildList(model->getRootItem()) << "\n";

    // Imagemap
    ts << "<map name='imagemap'>\n" + imageMap + "</map>\n";

    // Write footer
    ts << "<hr/>\n";
    ts << "<table class=\"vym-footer\">   \n\
        <tr> \n\
        <td class=\"vym-footerL\">" +
              filePath + "</td> \n\
            <td class=\"vym-footerC\">" +
              toS(QDate::currentDate()) + "</td> \n\
            <td class=\"vym-footerR\"> <a href='" +
              vymHome + "'>vym " + vymVersion + "</a></td> \n\
            </tr> \n \
            </table>\n";
    ts << "<script>\n"
          "(function() {\n"
          "  var btns = document.querySelectorAll('.vym-note-button');\n"
          "  for (var i = 0; i < btns.length; i++) {\n"
          "    btns[i].addEventListener('click', function() {\n"
          "      var content = this.nextElementSibling;\n"
          "      if (content.style.maxHeight) {\n"
          "        content.style.maxHeight = null;\n"
          "        this.innerHTML = '&#9654; Note';\n"
          "      } else {\n"
          "        content.style.maxHeight = content.scrollHeight + 'px';\n"
          "        this.innerHTML = '&#9660; Note';\n"
          "      }\n"
          "    });\n"
          "  }\n"
          "})();\n"
          "</script>\n";
    ts << "</body></html>";
    file.close();


    // Copy standard flags
    Flag *f;
    foreach (QUuid uid, activeFlags) {
        f = standardFlagsMaster->findFlagByUid(uid);
        if (!f)
            f = userFlagsMaster->findFlagByUid(uid);

        if (f) {
            ImageContainer *ic = f->getImageContainer();
            if (ic)
                ic->save(flagsDir.path() + "/" + ic->originalFilename());
        }
    }

    if (!dia.postscript.isEmpty()) {
        VymProcess p;
        p.runScript(dia.postscript, dirPath + "/" + filePath);
    }

    displayedDestination = filePath;

    result = ExportBase::Success;

    QStringList args;
    args << filePath;
    args << dirPath;
    completeExport(args);

    dia.saveSettings();
    model->setExportMode(false);
}
