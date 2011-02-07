#include "file.h"
#include "imports.h"
#include "linkablemapobj.h"
#include "misc.h"
#include "mainwindow.h"
#include "xsltproc.h"

extern Main *mainWindow;
extern QDir vymBaseDir;

ImportBase::ImportBase()
{
    bool ok;
    tmpDir.setPath (makeTmpDir(ok,"vym-import"));
    if (!tmpDir.exists() || !ok)
	QMessageBox::critical( 0, QObject::tr( "Error" ),
		       QObject::tr("Couldn't access temporary directory\n"));
}


ImportBase::~ImportBase()
{
    // Remove tmpdir
    removeDir (tmpDir);
}

void ImportBase::setDir(const QString &p)
{
    inputDir=p;
}

void ImportBase::setFile (const QString &p)
{
    inputFile=p;
}

bool ImportBase::transform()
{
    return true;
}

QString ImportBase::getTransformedFile()
{
    return transformedFile;
}

/////////////////////////////////////////////////
bool ImportKDE3Bookmarks::transform()
{
    transformedFile=tmpDir.path()+"/bookmarks.xml";

    XSLTProc p;
    p.setInputFile (tmpDir.home().path()+"/.kde/share/apps/konqueror/bookmarks.xml");
    p.setOutputFile (transformedFile);
    p.setXSLFile (vymBaseDir.path()+"/styles/kdebookmarks2vym.xsl");
    p.process();

    return true;
}

/////////////////////////////////////////////////
bool ImportKDE4Bookmarks::transform() //FIXME-3 hm, seems to work on macbook
{
    transformedFile=tmpDir.path()+"/bookmarks.xml";

    XSLTProc p;
    p.setInputFile (tmpDir.home().path()+"/.kde4/share/apps/konqueror/bookmarks.xml");
    p.setOutputFile (transformedFile);
    p.setXSLFile (vymBaseDir.path()+"/styles/kdebookmarks2vym.xsl");
    p.process();

    return true;
}



/////////////////////////////////////////////////
bool ImportFirefoxBookmarks::transform()
{
    transformedFile=tmpDir.path()+"/bookmarks.xml";

    QStringList lines;
    QFile file( inputFile );
    if ( file.open( QIODevice::ReadOnly ) ) 
    {
	QTextStream stream( &file );
	while ( !stream.atEnd() ) 
	    lines += stream.readLine(); // line of text excluding '\n'
	file.close();
    }
    // TODO Generate vym from broken bookmarks above...

    return true;
}

/////////////////////////////////////////////////
bool ImportMM::transform()
{
    // try to unzip 
    if (success==unzipDir (tmpDir, inputFile))
    {
	
	// Set short name, too. Search from behind:
	transformedFile=inputFile;
	int i=transformedFile.lastIndexOf ("/");
	if (i>=0) transformedFile=transformedFile.remove (0,i+1);
	transformedFile.replace(".mmap",".xml");
	transformedFile=tmpDir.path()+"/"+transformedFile;

	XSLTProc p;
	p.setInputFile (tmpDir.path()+"/Document.xml");
	p.setOutputFile (transformedFile);
	p.setXSLFile (vymBaseDir.path()+"/styles/mmap2vym.xsl");
	p.process();

	return true;
    } else
	return false;
    
}
