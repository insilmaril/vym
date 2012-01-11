/****************************************************************************
**
** Copyright (C) 2005-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

// highlighting rules have been adapted by Uwe Drechsel to match vym syntax


#include <QtGui>

#include "highlighter.h"

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\baddBranch\\b"
		    << "\\baddBranchBefore\\b"
                    << "\\baddMapCenter\\b"
                    << "\\baddMapInsert\\b"
		    << "\\baddMapReplace\\b"
                    << "\\bcolorBranch\\b"
		    << "\\bcolorSubtree\\b"
		    << "\\bcopy\\b"
                    << "\\bcut\\b"
                    << "\\bcycleTaskStatus\\b"
		    << "\\bdelete\\b"
		    << "\\bdeleteKeepChilds\\b"
		    << "\\bdeleteChilds\\b"
		    << "\\bexportAO\\b"
		    << "\\bexportASCII\\b"
		    << "\\bexportHTML\\b"
		    << "\\bexportImage\\b"
		    << "\\bexportPDF\\b"
		    << "\\bexportSVG\\b"
		    << "\\bexportXML\\b"
		    << "\\bimportDir\\b"
		    << "\\blinkTo\\b"
		    << "\\bloadImage\\b"
		    << "\\bloadNote\\b"
		    << "\\bmoveBranchUp\\b"
		    << "\\bmoveBranchDown\\b"
		    << "\\bmove\\b"
		    << "\\bmoveRel\\b"
		    << "\\bnop\\b"
		    << "\\bnote2URLs\\b"
		    << "\\bpaste\\b"
		    << "\\bqa\\b"
		    << "\\brelinkTo\\b"
		    << "\\bsaveImage\\b"
		    << "\\bsaveNote\\b"
		    << "\\bscroll\\b"
		    << "\\bselect\\b"
		    << "\\bselectLastBranch\\b"
		    << "\\bselectLastImage\\b"
		    << "\\bselectLatestAdded\\b"
		    << "\\bsetFrameType\\b"
		    << "\\bsetFramePenColor\\b"
		    << "\\bsetFrameBrushColor\\b"
		    << "\\bsetFramePadding\\b"
		    << "\\bsetFrameBorderWidth\\b"
		    << "\\bsetTaskSleep\\b"
		    << "\\btoggleFrameIncludeChildren\\b"
		    << "\\bsetFrameIncludeChildren\\b"
		    << "\\bsetHideLinkUnselected\\b"
		    << "\\bsetMapAuthor\\b"
		    << "\\bsetMapComment\\b"
		    << "\\bsetMapBackgroundColor\\b"
		    << "\\bsetMapDefLinkColor\\b"
		    << "\\bsetMapDefLinkStyle\\b"
		    << "\\bsetNote\\b"
		    << "\\bsetHeading\\b"
		    << "\\bsetHideExport\\b"
		    << "\\bsetIncludeImagesHorizontally\\b"
		    << "\\bsetIncludeImagesVertically\\b"
		    << "\\bsetURL\\b"
		    << "\\bsetVymLink\\b"
		    << "\\bsetFlag\\b"
		    << "\\bsetScale\\b"
		    << "\\bsetSelectionColor\\b"
		    << "\\bsortChildren\\b"
		    << "\\btoggleFlag\\b"
		    << "\\btoggleTask\\b"
		    << "\\bunscroll\\b"
		    << "\\bunscrollChilds\\b"
		    << "\\bunsetFlag\\b"
		    ;
    foreach (QString pattern, keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // QT keywords
    /*
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);
    */

    // Single line comments
    singleLineCommentFormat.setForeground(Qt::red);
    rule.pattern = QRegExp("#[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // multiline comments
    multiLineCommentFormat.setForeground(Qt::red);
    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");

    // Quotations
    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    QStringList valuePatterns;
    valuePatterns << "\\btrue\\b" << "\\bfalse\\b";
    foreach (QString pattern, valuePatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = quotationFormat;
        highlightingRules.append(rule);
    }



    // Funtions
    /*
    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    */

}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (HighlightingRule rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        int endIndex = text.indexOf(commentEndExpression, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression,
                                                startIndex + commentLength);
    }
}
