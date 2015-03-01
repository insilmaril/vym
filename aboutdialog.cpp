#include <QDate>
#include <QFrame>
#include <QProcess>
#include <QVBoxLayout>

#include "aboutdialog.h"
#include "settings.h"


extern Settings settings;
extern QString iconPath;    // FIXME-3 embed vym logo as ressource?
extern QString vymVersion;
extern QString vymBuildDate;
extern QString vymCodeName;

AboutDialog::AboutDialog( QWidget *parent)
    : QDialog( parent)
{
 
    mainLayout=new QVBoxLayout( this);

    tabs=new QTabWidget (this);
    credits=new AboutTextBrowser (parent);

    credits->setHtml( QString(
    "<center><img src=\"" + iconPath + "vym-128x128.png\"></center>"
    "<h3><center>VYM - View Your Mind </h3>"
    "<p align=\"center\"> A tool to put the things you have got in your mind into a map.</p>"
    "<p align=\"center\"> (c) 2004-%1 by Uwe Drechsel (<a href=\"mailto:vym@InSilmaril.de\">vym@InSilmaril.de</a>)</p>"
    "<p align=\"center\"> Version " +vymVersion+" - " +vymBuildDate+"</p>"
    "<p align=\"center\"> " +vymCodeName+"</p>"
    "<ul>"
    "<li> Contact</li>"
	"<ul>"
	    "<li> vym homepage:<br> <a href=\"http://www.InSilmaril.de/vym\">"
	    "http://www.InSilmaril.de/vym</a></li>"
	    "<li> Project homepage on Sourceforge:<br> <a href=\"http://sourceforge.net/projects/vym/\">"
	    "http://sourceforge.net/projects/vym/</a></li>"
	    "<li> Mailinglists are also on Sourceforge:"
		"<ul>"
		    "<li>Please ask general questions about vym  on "
		"<a href=\"mailto:vym-forum@lists.sourceforge.net\">vym-forum</a></li>"
		"<li>Subscribe/Unsubscribe and archives can be found  "
		"<a href=\"https://sourceforge.net/mail/?group_id=127802\">here</a></li>"
		"</ul>"
	"</ul>"	    
    "<li> Credits " 
    "<ul>"
    "<li>Documentation"
    "  <ul>"
    "    <li>Peter Adams: documentation proofreading and polishing</li>"
    "  </ul>"
    "</li>"
    "<li>Translation"
    "  <ul>"
    "    <li>Spanish: <a href=\"http://ieee.udistrital.edu.co/aclibre\">"
    "                 ACLibre (Academia y Conocimiento Libre)</a> and David Amian</li>"
    "    <li>French: Marc Sert, Philippe Caillaud and Claude </li>"
    "    <li>Italian: Giovanni Sora, Seyed Puria Nafisi Azizi </li>"
    "    <li>Interlingua: Giovanni Sora</li>"
    "    <li>Brasilian: Amadeu Júnior</li>"
    "    <li>Russian: Anton Olenev</li>"
    "    <li>Simplified Chinese: Moligaloo</li>"
    "    <li>Traditional Chinese: Wei-Lun Chao </li>"
    "  </ul>"
    "</li>"
    "<li> Patches"
    "  <ul>"
    "    <li>Konstantin Goudkov: sort branches</li>"
    "    <li>Jakob Hilmer: image drag and drop in 1.8.1, &quot;About vym&quot; window patch </li>"
    "    <li>Edward Wang: adding close tab buttons</li>"
    "    <li>p0llox (Pierre): various patches for Debian packaging</li>"
    "  </ul>"
    "</li>"
    "<li> Patches in previous vym versions"
    "  <ul>"
    "    <li>Thomas Schraitle for the stylesheet"  
    "        formerly used for XHTML-export and help with XML processing in general</li>"
    "    <li>Matt from <a href=\"http://www.satbp.com\">www.satbp.com</a>: "
    "        <a href=\"http://www.taskjuggler.org\">Taskjuggler</a> export</li>"
    "  </ul>"
    "</li>"
    "<li> Packaging"
    "</li>"
    "  <ul>"
    "    <li>Łukasz Pietrzak, Scott Dillman and Patrick Spendrin: Recent windows patches</li>"
    "    <li>Jon Ciesla: Sourceforge file releases</li>"
    "    <li>Andrew Ng, Juha Ruotsalainen and Thomas Kriener: Older windows patches</li>"
    "    <li>Xavier Oswald, Christoph Thielecke, Pierre, and Steffen Joeris: Debian packaging</li>"
    "  </ul>"
    "<li> General"
    "  <ul>"
    "    <li>CMake setup by Costantino Giuliodori and Patrick Spendrin</li>"
    "    <li>All the guys at Trolltech (now Digia) for their Qt toolkit</li>"
    "    <li>All the guys at SUSE Linux for openSUSE Linux and support,"
    "        e.g. to get Linux running on PowerPC and also Macbooks</li>"
    "  </ul>"
    "</li>"
    "</ul>"
    "</li>").arg( QDate::currentDate().year() ) );;
    credits->setFrameStyle( QFrame::Panel | QFrame::Plain );
    tabs->addTab (credits,"Credits");

    license=new AboutTextBrowser (parent);
    license->setText ( QString(
    "<center>"
    "<h3>VYM - View Your Mind</h3>"
    "<p>Copyright (C) 2004-%1 Uwe Drechsel</p>"  
    "</center>"

    "<p>This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.</p>"

    "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License below for more details.</p>"

    "<p>In addition, as a special exception, Uwe Drechsel gives permission to link the code of this program with the QT libraries from trolltech.com (or with modified versions of QT that use the same license as QT), and distribute linked combinations including the two. You must obey the GNU General Public License in all respects for all of the code used other than QT. If you modify this file, you may extend this exception to your version of the file, but you are not obligated to do so. If you do not wish to do so, delete this exception statement from your version.</p> "

    "<p>Uwe Drechsel can be contacted at <a href=\"mailto:vym@insilmaril.de\">vym@insilmaril.de</a></p>"

    "<hr>"

 "<p align=\"center\">GNU GENERAL PUBLIC LICENSE<br>"
"Version 2, June 1991</p>"

"<p align=\"center\">Copyright (C) 1989, 1991 Free Software Foundation, Inc.  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA</p>"
     
"<p align=\"center\"> Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.</p>"

"<p align=\"center\">Preamble</p>"

"<p>The licenses for most software are designed to take away your freedom to share and change it.  By contrast, the GNU General Public License is intended to guarantee your freedom to share and change free software--to make sure the software is free for all its users.  This General Public License applies to most of the Free Software Foundation's software and to any other program whose authors commit to using it.  (Some other Free Software Foundation software is covered by the GNU Library General Public License instead.)  You can apply it to your programs, too.</p>"

"<p>When we speak of free software, we are referring to freedom, not price.  Our General Public Licenses are designed to make sure that you have the freedom to distribute copies of free software (and charge for this service if you wish), that you receive source code or can get it if you want it, that you can change the software or use pieces of it in new free programs; and that you know you can do these things.</p>"

"<p>  To protect your rights, we need to make restrictions that forbid anyone to deny you these rights or to ask you to surrender the rights.  These restrictions translate to certain responsibilities for you if you distribute copies of the software, or if you modify it.</p>"

"<p>  For example, if you distribute copies of such a program, whether gratis or for a fee, you must give the recipients all the rights that you have.  You must make sure that they, too, receive or can get the source code.  And you must show them these terms so they know their rights.</p>"

"<p>  We protect your rights with two steps: (1) copyright the software, and (2) offer you this license which gives you legal permission to copy, distribute and/or modify the software.</p>"

"<p>  Also, for each author's protection and ours, we want to make certain that everyone understands that there is no warranty for this free software.  If the software is modified by someone else and passed on, we want its recipients to know that what they have is not the original, so that any problems introduced by others will not reflect on the original authors' reputations.</p>"

"<p>  Finally, any free program is threatened constantly by software patents.  We wish to avoid the danger that redistributors of a free program will individually obtain patent licenses, in effect making the program proprietary.  To prevent this, we have made it clear that any patent must be licensed for everyone's free use or not licensed at all.</p>"

"<p>  The precise terms and conditions for copying, distribution and modification follow.</p>"

"<p align=\"center\">		GNU GENERAL PUBLIC LICENSE</p>"
"<p align=\"center\">   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION</p>"

"<p>  0. This License applies to any program or other work which contains a notice placed by the copyright holder saying it may be distributed under the terms of this General Public License.  The \"Program\", below, refers to any such program or work, and a \"work based on the Program\" means either the Program or any derivative work under copyright law: that is to say, a work containing the Program or a portion of it, either verbatim or with modifications and/or translated into another language.  (Hereinafter, translation is included without limitation in the term \"modification\".)  Each licensee is addressed as \"you\".</p>"

"<p>Activities other than copying, distribution and modification are not covered by this License; they are outside its scope.  The act of running the Program is not restricted, and the output from the Program is covered only if its contents constitute a work based on the Program (independent of having been made by running the Program).  Whether that is true depends on what the Program does.</p>"

"<p>  1. You may copy and distribute verbatim copies of the Program's source code as you receive it, in any medium, provided that you conspicuously and appropriately publish on each copy an appropriate copyright notice and disclaimer of warranty; keep intact all the notices that refer to this License and to the absence of any warranty; and give any other recipients of the Program a copy of this License along with the Program.</p>"

"<p>You may charge a fee for the physical act of transferring a copy, and you may at your option offer warranty protection in exchange for a fee.</p>"

"<p>  2. You may modify your copy or copies of the Program or any portion of it, thus forming a work based on the Program, and copy and distribute such modifications or work under the terms of Section 1 above, provided that you also meet all of these conditions:"
"<ol type=\"a\">"
    "<li> You must cause the modified files to carry prominent notices stating that you changed the files and the date of any change.</li>"

    "<li> You must cause any work that you distribute or publish, that in whole or in part contains or is derived from the Program or any part thereof, to be licensed as a whole at no charge to all third parties under the terms of this License.</li>"

    "<li> If the modified program normally reads commands interactively when run, you must cause it, when started running for such interactive use in the most ordinary way, to print or display an announcement including an appropriate copyright notice and a notice that there is no warranty (or else, saying that you provide a warranty) and that users may redistribute the program under these conditions, and telling the user how to view a copy of this License.  (Exception: if the Program itself is interactive but does not normally print such an announcement, your work based on the Program is not required to print an announcement.)</li>"
"</ol>"
"</p>"

"<p>These requirements apply to the modified work as a whole.  If identifiable sections of that work are not derived from the Program, and can be reasonably considered independent and separate works in themselves, then this License, and its terms, do not apply to those sections when you distribute them as separate works.  But when you distribute the same sections as part of a whole which is a work based on the Program, the distribution of the whole must be on the terms of this License, whose permissions for other licensees extend to the entire whole, and thus to each and every part regardless of who wrote it.</p>"

"<p>Thus, it is not the intent of this section to claim rights or contest your rights to work written entirely by you; rather, the intent is to exercise the right to control the distribution of derivative or collective works based on the Program.</p>"

"<p>In addition, mere aggregation of another work not based on the Program with the Program (or with a work based on the Program) on a volume of a storage or distribution medium does not bring the other work under the scope of this License.</p>"

"<p>  3. You may copy and distribute the Program (or a work based on it, under Section 2) in object code or executable form under the terms of Sections 1 and 2 above provided that you also do one of the following:" "<ol type=\"a\">"

"    <li> Accompany it with the complete corresponding machine-readable source code, which must be distributed under the terms of Sections 1 and 2 above on a medium customarily used for software interchange; or,</li>"

"    <li> Accompany it with a written offer, valid for at least three years, to give any third party, for a charge no more than your cost of physically performing source distribution, a complete machine-readable copy of the corresponding source code, to be distributed under the terms of Sections 1 and 2 above on a medium customarily used for software interchange; or,</li>"

"    <li> Accompany it with the information you received as to the offer to distribute corresponding source code.  (This alternative is allowed only for noncommercial distribution and only if you received the program in object code or executable form with such an offer, in accord with Subsection b above.)</li>"
"</ol></p>"

"<p>The source code for a work means the preferred form of the work for making modifications to it.  For an executable work, complete source code means all the source code for all modules it contains, plus any associated interface definition files, plus the scripts used to control compilation and installation of the executable.  However, as a special exception, the source code distributed need not include anything that is normally distributed (in either source or binary form) with the major components (compiler, kernel, and so on) of the operating system on which the executable runs, unless that component itself accompanies the executable.</p>"

"<p>If distribution of executable or object code is made by offering access to copy from a designated place, then offering equivalent access to copy the source code from the same place counts as distribution of the source code, even though third parties are not compelled to copy the source along with the object code.</p>"

"<p>  4. You may not copy, modify, sublicense, or distribute the Program except as expressly provided under this License.  Any attempt otherwise to copy, modify, sublicense or distribute the Program is void, and will automatically terminate your rights under this License.  However, parties who have received copies, or rights, from you under this License will not have their licenses terminated so long as such parties remain in full compliance.</p>"

"<p>  5. You are not required to accept this License, since you have not signed it.  However, nothing else grants you permission to modify or distribute the Program or its derivative works.  These actions are prohibited by law if you do not accept this License.  Therefore, by modifying or distributing the Program (or any work based on the Program), you indicate your acceptance of this License to do so, and all its terms and conditions for copying, distributing or modifying the Program or works based on it.</p>"

"<p>  6. Each time you redistribute the Program (or any work based on the Program), the recipient automatically receives a license from the original licensor to copy, distribute or modify the Program subject to these terms and conditions.  You may not impose any further restrictions on the recipients' exercise of the rights granted herein.  You are not responsible for enforcing compliance by third parties to this License.</p>"

"<p>  7. If, as a consequence of a court judgment or allegation of patent infringement or for any other reason (not limited to patent issues), conditions are imposed on you (whether by court order, agreement or otherwise) that contradict the conditions of this License, they do not excuse you from the conditions of this License.  If you cannot distribute so as to satisfy simultaneously your obligations under this License and any other pertinent obligations, then as a consequence you may not distribute the Program at all.  For example, if a patent license would not permit royalty-free redistribution of the Program by all those who receive copies directly or indirectly through you, then the only way you could satisfy both it and this License would be to refrain entirely from distribution of the Program.</p>"

"<p>If any portion of this section is held invalid or unenforceable under any particular circumstance, the balance of the section is intended to apply and the section as a whole is intended to apply in other circumstances.</p>"

"<p>It is not the purpose of this section to induce you to infringe any patents or other property right claims or to contest validity of any such claims; this section has the sole purpose of protecting the integrity of the free software distribution system, which is implemented by public license practices.  Many people have made generous contributions to the wide range of software distributed through that system in reliance on consistent application of that system; it is up to the author/donor to decide if he or she is willing to distribute software through any other system and a licensee cannot impose that choice.</p>"

"<p>This section is intended to make thoroughly clear what is believed to be a consequence of the rest of this License.</p>"

"<p>  8. If the distribution and/or use of the Program is restricted in certain countries either by patents or by copyrighted interfaces, the original copyright holder who places the Program under this License may add an explicit geographical distribution limitation excluding those countries, so that distribution is permitted only in or among countries not thus excluded.  In such case, this License incorporates the limitation as if written in the body of this License.</p>"

"<p>  9. The Free Software Foundation may publish revised and/or new versions of the General Public License from time to time.  Such new versions will be similar in spirit to the present version, but may differ in detail to address new problems or concerns.</p>"

"<p>Each version is given a distinguishing version number.  If the Program specifies a version number of this License which applies to it and \"any later version\", you have the option of following the terms and conditions either of that version or of any later version published by the Free Software Foundation.  If the Program does not specify a version number of this License, you may choose any version ever published by the Free Software Foundation.</p>"

"<p>  10. If you wish to incorporate parts of the Program into other free programs whose distribution conditions are different, write to the author to ask for permission.  For software which is copyrighted by the Free Software Foundation, write to the Free Software Foundation; we sometimes make exceptions for this.  Our decision will be guided by the two goals of preserving the free status of all derivatives of our free software and of promoting the sharing and reuse of software generally.</p>"

"<p align=\"center\">NO WARRANTY</p>"

"<p>  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.</p>"

"<p>  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.</p>"

"<p align=\"center\">END OF TERMS AND CONDITIONS</p>").arg( QDate::currentDate().year() ) );

    credits->setFrameStyle( QFrame::Panel | QFrame::Plain );
    tabs->addTab (license,"License");

    mainLayout->addWidget (tabs);

    okbutton =new QPushButton (this);
    okbutton->setText (tr("Ok","Ok Button"));
    okbutton->setMaximumSize (QSize (50,30));
    okbutton->setAutoDefault (true);
    mainLayout->addWidget( okbutton); 

    connect( okbutton, SIGNAL( clicked() ), this, SLOT( accept() ) );
}

AboutTextBrowser::AboutTextBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
}

void AboutTextBrowser::setSource(const QUrl &url )
{
    QProcess *proc= new QProcess ();
    proc->start( settings.value("/mainwindow/readerURL").toString(),QStringList ()<<url.toString());
    //if (!proc->waitForStarted() &&mainWindow->settingsURL() ) setSource(url);
    if (!proc->waitForStarted() )
	QMessageBox::warning(0, 
	tr("Warning","About window"),
	tr("Couldn't find a viewer to open %1.\n","About window").arg(url.toString())+
	tr("Please use Settings->")+tr("Set application to open an URL..."));

}
