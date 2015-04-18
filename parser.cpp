#include "parser.h"

#include <QDebug>
#include <QRegExp>
#include <iostream>

#include "command.h"
#include "treeitem.h"

extern QList <Command*> modelCommands;

Parser::Parser()
{
    initParser();
}

void Parser::initParser()
{
    initAtom();
    current=-1;
}

void Parser::initAtom()
{
    atom="";
    com="";
    paramList.clear();
    errLevel=NoError;
    errDescription="";
    errMessage="";
}

void Parser::parseAtom (QString s)
{
    initAtom();
    QRegExp re;
    int pos;

    // Strip WS at beginning
    while (s.length() > 0 && (
               s.at(0) == '\n' ||
               s.at(0) == '\r' ||
               s.at(0) == ' ') )
        s = s.mid(1);
    if (s.length() == 0)
        return;

    // Get command
    re.setPattern ("^(\\w*)");
    pos=re.indexIn (s);
    if (pos>=0)
	com=re.cap(1);

    // Get parameters
    paramList.clear();
    QString t;
    int leftParenthesis;
    int rightParenthesis;
    if (!nextParenthesisContents(s, leftParenthesis, rightParenthesis, t)) return;

    paramList = findParameters(t);
    atom = s;
}

QString Parser::getAtom()
{
    return atom;
}

QString Parser::getCommand()
{
    return com;
}

QStringList Parser::getParameters()
{
    return paramList;
}

int Parser::parCount()
{
    return paramList.count();
}

QString Parser::errorMessage()
{
    QString l;
    switch (errLevel)
    {
	case NoError: l="No Error"; break;
	case Warning: l="Warning"; break;
	case Aborted: l="Aborted"; break;
        default: l="";
    }
    return QString ("Error Level: '%1'  Command: '%2' Description: '%3'")
	.arg(l).arg(com).arg(errDescription);
}

QString Parser::errorDescription()
{
    return errDescription;
}

ErrorLevel Parser::errorLevel()
{
    return errLevel;
}

void Parser::setError(ErrorLevel level, const QString &description)
{
    errDescription=description;
    errLevel=level;
}

bool Parser::checkParameters(TreeItem *selti)
{
    foreach (Command *c, modelCommands)
    {
	if (c->getName() == com)
	{
	    // Check type of selection
	    if (selti)
	    {
		bool ok;
		ok=false;
		TreeItem::Type st=selti->getType();
		Command::SelectionType ct=c->getSelectionType();
		if (ct==Command::TreeItem || ct==Command::BranchOrImage)
		{
		    if (st==TreeItem::MapCenter ||
			st==TreeItem::Branch ||
			st==TreeItem::XLink ||
			st==TreeItem::Image ) 
			ok=true;
		} else if ( ct==Command::BranchOrImage )
		{
		    if (st==TreeItem::MapCenter ||
			st==TreeItem::Branch ||
			st==TreeItem::Image ) 
			ok=true;
		} else if ( ct==Command::Branch || ct==Command::BranchLike)
		{

		    if (st == TreeItem::MapCenter ||
			st == TreeItem::Branch )
			ok=true;
		} else if ( ct==Command::Image )	    
		{
		    if (st==TreeItem::Image )
			ok=true;
		} else if ( ct==Command::Any)	    
		{
		    ok=true;
		} else if ( ct==Command::XLink)	    
		{
		    if (st==TreeItem::XLink)
			ok=true;
		} else
		    qWarning()<<"Parser::checkParameters  Unknown selection type";
		if (!ok)
		{
		    setError (Aborted, "Selection does not match command");
		    return false;
	    	}
	    }

	    // Check for number of parameters
	    int optPars=0;
	    for (int i=0; i < c->parCount(); i++ )
		if (c->isParOptional(i) ) optPars++;
	    if (paramList.count() < (c->parCount() - optPars) ||
	        paramList.count() > c->parCount() )
	    {
		QString expected;
		if (optPars>0)
		    expected=QString("%1..%2").arg(c->parCount()-optPars).arg(c->parCount() );
		else 
		    expected=QString().setNum(c->parCount());
		setError (
		    Aborted,
		    QString("Wrong number of parameters: Expected %1, but found %2").arg(expected).arg(paramList.count()));
		return false;
	    }

	    // Check types of parameters
	    bool ok;
	    for (int i=0; i < paramList.count(); i++ )
	    {	
		switch (c->getParType(i) )
		{
		    case Command::String:
			parString (ok,i);
                        if (!ok) 
                        {
                            // Convert to string implicitly
                            paramList[i]='"' + paramList[i] + '"';
                            ok=true;
                        }
			break;
		    case Command::Int:	
			parInt (ok,i);
			break;
		    case Command::Double:	
			parDouble (ok,i);
			break;
		    case Command::Color:	
			parColor (ok,i);
			break;
		    case Command::Bool:	
			parBool (ok,i);
			break;
		    default: ok=false;	
		}
		if (!ok)
		{
		    setError (
			Aborted, 
			QString("Parameter %1 has wrong type").arg(i));
		    return false;
		}
	    }
	    resetError();
	    return true;
	}    
    } 
    setError (Aborted,"Unknown command");
    return false;
}

void Parser::resetError ()
{
    errMessage="";
    errDescription="";
    errLevel=NoError;
}

bool Parser::checkParCount (const int &expected)
{
    if (paramList.count()!=expected)
    {
	setError (
	    Aborted,
	    QString("Wrong number of parameters: Expected %1, but found %2").arg(expected).arg(paramList.count()));
	return false;
    } 
    return true;    
}

bool Parser::checkParIsInt(const int &index)
{
    bool ok;
    if (index > paramList.count())
    {
	setError (
	    Aborted,
	    QString("Parameter index %1 is outside of parameter list").arg(index));
	return false;
    } else
    {
	paramList[index].toInt (&ok, 10);
	if (!ok)
	{
	    setError (
		Aborted,
		QString("Parameter %1 is not an integer").arg(index));
	    return false;
	} 
    }	
    return true;
}

bool Parser::checkParIsDouble(const int &index)
{
    bool ok;
    if (index > paramList.count())
    {
	setError (
	    Aborted,
	    QString("Parameter index %1 is outside of parameter list").arg(index));
	return false;
    } else
    {
	paramList[index].toDouble (&ok);
	if (!ok)
	{
	    setError (
		Aborted,
		QString("Parameter %1 is not double").arg(index));
	    return false;
	} 
    }	
    return true;
}

int Parser::parInt (bool &ok,const uint &index)
{
    if (checkParIsInt (index))
	return paramList[index].toInt (&ok, 10);
    ok=false;
    return 0;
}

QString Parser::parString (bool &ok, const int &index)
{
    // return the string at index, this could be also stored in
    // a variable later
    
    // Try to find out if string boundaries are "" or ''
    QRegExp rx;
    int pos = paramList[index].indexOf("\"");
    int n   = paramList[index].indexOf("'");

    if ( n < 0 && pos < 0 )
    {
        // Neither " nor ' found
        ok = false;
        return "";
    } else if ( pos >= 0 && n < 0) 
        // Found ", but no ' 
        rx.setPattern("\"(.*)\"");
    else if ( n >= 0 && pos < 0) 
        // Found ', but no "
        rx.setPattern("'(.*)'");
    else if ( pos > n )
        // "" is within ''
        rx.setPattern("'(.*)'");
    else
        // '' is within ""
        rx.setPattern("\"(.*)\"");


    qDebug() << "Parser:: parString  a) pos="<<pos<<" n="<<n<< "  pattern: " << rx.pattern() <<  " of " << paramList[index];

    QString r;
    pos=rx.indexIn (paramList[index]);
    if (pos>=0)
    {
	r = rx.cap (1);
	ok = true;
    } else    
    {
	r = "";
	ok = false;
    }
    qDebug() << "Parser:: parString  b) r="<<r;
    return r;
}

bool Parser::parBool (bool &ok,const int &index)
{
    // return the bool at index, this could be also stored in
    // a variable later
    QString r;
    ok=true;
    QString p=paramList[index];
    if (p=="true" || p=="1")
	return true;
    else if (p=="false" || p=="0")
	return false;
    ok=false;
    return ok;
}

QColor Parser::parColor(bool &ok,const int &index)
{
    // return the QColor at index
    ok = false;
    QString r;
    QColor c;

    // testscipts use single quotes, convert them first
    paramList[index].replace ("'", "\"");
    QRegExp re("\"(.*)\"");
    int pos = re.indexIn (paramList[index]);
    if (pos >= 0)
    {
	r = re.cap (1);
	c.setNamedColor(r);
	ok = c.isValid();
    }	
    return c;
}

double Parser::parDouble (bool &ok,const int &index)
{
    if (checkParIsDouble (index))
	return paramList[index].toDouble (&ok);
    ok=false;
    return 0;
}

void Parser::setScript(const QString &s)
{
    script=s;
}   

QString Parser::getScript()
{
    return script;
}   

void Parser::execute()
{
    current=0;
}   

bool Parser::next() //FIXME-3 parser does not detect missing closing " or '("foo" ()'
{
    int start=current;
    if (current<0) execute();
    if (current+1>=script.length()) return false;

    bool inQuote=false;
    QChar bnd;

    while (true)
    {
        // Check if we are inside a string
        if (script.at(current) == '\"')
        {
            if (inQuote)
            {
                if (script.at(current) == bnd)
                    inQuote=false;
            } else
            {
                inQuote = true;
                bnd = '\"';
            }
        } else if (script.at(current) == '\'' )
        {
            if (inQuote)
            {
                if (script.at(current) == bnd)
                    inQuote=false;
            } else
            {
                inQuote = true;
                bnd = '\'';
            }
        }

        // Check for EOL
        if (script.at(current) == '\n')
        {
            if (current+1 < script.length())
            {
                current++;
                if (script.at(current) == '\r')
                    current++;
            }
        }

        // Check if we are in a comment
        if (!inQuote && script.at(current)=='#')
        {
            while (script.at(current)!='\n')
            {
                current++;
                if (script.at(current) == '\r')
                    current++;
                if (current+1>=script.length())
                    return false;
            }
            start=current;
        }

        // Check for end of atom
        if (!inQuote && script.at(current)==';')
        {
            parseAtom(script.mid(start,current-start) );
            current++;
            return true;
        }

        // Check for end of script
        if (current+1>=script.length() )
        {
            if (inQuote)
            {
                setError (Aborted,"Runaway string");
                return false;
            } else
            {
                atom=script.mid(start);
                return true;
            }
        }
        current++;
    }
}   

QStringList Parser::getCommands() 
{
    QStringList list;
    foreach (Command *c, modelCommands)
	list.append (c->getName() );
    return list;	
}

QStringList Parser::findParameters(const QString &s)
{
    QStringList ret;
    int left = 0;
    bool inquote = false;

    // Try to find out if string boundaries are "" or ''
    QString bnd;
    int pos = s.indexOf("\"");
    int n   = s.indexOf("'");

    if ( n < 0 && pos < 0 )
    {
        // Neither " nor ' found, ignore later
        bnd = "\"";
    } else if ( pos >= 0 && n < 0) 
        // Found ", but no ' 
        bnd = "\"";
    else if ( n >= 0 && pos < 0) 
        // Found ', but no "
        bnd = "'";
    else if ( pos > n )
        // "" is within ''
        bnd = "'";
    else
        // '' is within ""
        bnd = "\"";

    qDebug() << "Parser::findParams a)  bnd=" << bnd << "s=" << s;
    pos = 0;
    while (pos < s.length())
    {
        //FIXME-0 qDebug()<< QString("s[%1]=%2  inquote=%3").arg(pos).arg(s.at(pos)).arg(inquote);
        if (s.at(pos) == bnd ) 
        {
            if (inquote)
                inquote = false;
            else    
                inquote = true;
        }
        if (s.at(pos) == ',' && !inquote)
        {
            qDebug()<<"findParameters:   , !!!";  //FIXME-0
            ret << s.mid(left, pos - left );
            left = pos + 1;
        }
        pos++;
    }
    if (left > 0) 
        ret << s.mid(left, pos - left );
    else
        if (!s.isEmpty()) ret << s;
    qDebug() << "Parser::findParams b)  count="<<ret.count()<<"  ret=" << ret;
    return ret;
}

bool Parser::nextParenthesisContents(
        const QString &s, 
        int &leftParenthesis, 
        int &rightParenthesis, 
        QString &contents)
{
    int pos = 0;
    int leftP  = -1;
    int rightP = -1;
    int openParenthesis = 0;
    bool inQuote = false;
    QChar bnd;
    while (pos < s.length())
    {
        // Check if we are inside a string
        if (s.at(current) == '\"')
        {
            if (inQuote)
            {
                if (s.at(current) == bnd)
                    inQuote=false;
            } else
            {
                inQuote = true;
                bnd = '\"';
            }
        } else if (s.at(current) == '\'' )
        {
            if (inQuote)
            {
                if (s.at(current) == bnd)
                    inQuote=false;
            } else
            {
                inQuote = true;
                bnd = '\'';
            }
        }

        if (s.at(pos) == '(' && !inQuote)
        {
            openParenthesis++;
            if (openParenthesis == 1) leftP=pos;
        }

        if (s.at(pos) == ')' && !inQuote)
        {
            openParenthesis--;
            if (openParenthesis == 0) rightP=pos;
        }

        if (openParenthesis < 0) 
        {
            setError(Aborted, "Error, too many closing parenthesis!");
            return false;
        }

        pos++;
    }

    if (leftP < 0)
    {
        setError(Aborted, "Error: No left parenthesis found");
        return false;
    }

    if (rightP < 0) 
    {
        setError(Aborted, "Error: No right parenthesis found");
        return false;
    }

    contents = s.mid(leftP+1, rightP - leftP - 1);
    pos = leftParenthesis;
    leftParenthesis  = leftP;
    rightParenthesis = rightP;

    qDebug()<<"Parser::nextParContents        s="<<s;
    qDebug()<<"Parser::nextParContents contents="<<contents;
    return true;
}

