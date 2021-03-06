
/**************************************************************************
*                    Universitat Autonoma de Barcelona,					  *
*              HPC4SE: http://grupsderecerca.uab.cat/hpca4se/             *
*                        Analysis and Tuning Group, 					  *
*					            2002 - 2018                  			  */
/**************************************************************************
*	  See the LICENSE.md file in the base directory for more details      *
*									-- 									  *
*	This file is part of MATE.											  *	
*																		  *
*	MATE is free software: you can redistribute it and/or modify		  *
*	it under the terms of the GNU General Public License as published by  *
*	the Free Software Foundation, either version 3 of the License, or     *
*	(at your option) any later version.									  *
*																		  *
*	MATE is distributed in the hope that it will be useful,				  *
*	but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 		  *
*	GNU General Public License for more details.						  *
*																		  *
*	You should have received a copy of the GNU General Public License     *
*	along with MATE.  If not, see <http://www.gnu.org/licenses/>.         *
*																		  *
***************************************************************************/

// ------------------------------------------------------------------
// Syslog.cpp
//
// ------------------------------------------------------------------

#include "Syslog.h"
#include "SysException.h"
#include "Env.h"
#include "Paths.h"
#include <iomanip>
#include <strstream>
#include <cassert>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

#include <sys/param.h>
#include <unistd.h>

const char SPACE = ' ';
const char LB = '[';
const char RB = ']';

using namespace std;

bool Syslog::_masterSwitch = false;  // master log switch
Syslog::LoggerVector Syslog::_loggers;		// vector of loggers


// default LogSeverity mask
const int DEFAULT_SEVERITY_MASK = (INFO | WARNING | ERROR | FATAL);

// All LogSeverity mask
const int ALL_SEVERITY_MASK = 0xFF;	

const int SeverityMasks [] = 
{
	ALL_SEVERITY_MASK,				// level 0
	INFO | WARNING | ERROR | FATAL, // level 1
	WARNING | ERROR | FATAL,		// level 2
	ERROR | FATAL,					// level 3
	FATAL							// level 4
};
	
// log severity names
static char const * SeverityName [] =
{
//  0	 1        2     3     4       
	0, "DEBUG", "INFO", 0, "WARNING", 
//  5, 6, 7,   8,         
	0, 0, 0, "ERROR",
//  9,10,11, 12
	0, 0, 0, 0,
// 13, 14,15,16
	0, 0, 0, "FATAL"
};

BasicLogFormatter::BasicLogFormatter (Config & cfg)
{
	_showTimestamp = cfg.GetBoolValue ("Syslog", "ShowTimestamp", true);
	_showSeverity = cfg.GetBoolValue ("Syslog", "ShowSeverity", true);
	_prefix = cfg.GetStringValue ("Syslog", "Prefix");
	//string dateFormat = cfg.GetStringValue ("Syslog", "DateFormat", DefDateFormat);
	//SetTimestampFormat (dateFormat);
}

string BasicLogFormatter::GetSeverityString (LogSeverity s)
{
	char const * str = SeverityName [s];
	if (str != 0)
		return str;
	else
		return "UNKNOWN";
}

string BasicLogFormatter::FormatTimestamp (DateTime const & dt) const
{
	return dt.GetStringValue ();
}

string BasicLogFormatter::Format (LogEntry const & entry) const
{
	ostrstream stream;
	if (_prefix.length () > 0)
	{
		stream << LB
			   << setw (8) << setfill (SPACE)
			   << setiosflags (ios::left)
			   << _prefix 
			   << resetiosflags (ios::adjustfield)
			   << RB << SPACE;
	}

	if (_showTimestamp)
		stream << FormatTimestamp (entry.GetTimestamp ()) << SPACE;
	if (_showSeverity)
	{
		string s = GetSeverityString (entry.GetSeverity ());
		stream << LB
			   << setw (7) << setfill (SPACE)
			   << setiosflags (ios::left)
			   << s 
			   << resetiosflags (ios::adjustfield)
			   << RB << SPACE;
	}
	stream << entry.GetMessage () << endl << ends;
	string copy = stream.str ();
	stream.freeze (0);
	return copy;
}

string BasicLogFormatter::GetLogHeader () const
{
	return "SYSLOG START";
}

string BasicLogFormatter::GetLogFooter () const
{
	return "SYSLOG END";
}

BasicLogFilter::BasicLogFilter ()
: _mask (DEFAULT_SEVERITY_MASK)
{
}

BasicLogger::BasicLogger ()
	: _filter (new BasicLogFilter ()), 
	  _formatter (new BasicLogFormatter ())
{
}

bool BasicLogger::Accept (LogEntry const & entry) const
{
	if (_filter.get () != 0)
		return _filter->Accept (entry);
	else	
		return true;
}


StreamLogger::StreamLogger (ostream & stream)
: _stream (stream)
{
	assert (_formatter.get () != 0);
	_stream << _formatter->GetLogHeader () 
			<< endl;
	_stream.flush ();
}

StreamLogger::~StreamLogger ()
{
	assert (_formatter.get () != 0);
	_stream << _formatter->GetLogFooter () 
			<< endl;
	_stream.flush ();
}

void StreamLogger::Log (LogEntry const & entry)
{
	if (!Accept (entry))
		return;
	assert (_formatter.get () != 0);
	string s = _formatter->Format (entry);
	_stream << s;
	_stream.flush ();
}

FileLogger::FileLogger (string const & filepath, bool append)
{
	if (append) // append to file
		_stream.open (filepath.c_str (), std::ios::out | std::ios::app);
	else // empty file
		_stream.open (filepath.c_str (), std::ios::out | std::ios::trunc);
	if (!_stream)
		throw SysException ("Log file open failed", filepath);
	assert (_formatter.get () != 0);
	_stream << "FileLogger" << _formatter->GetLogHeader () << endl;
	_stream.flush ();
}

FileLogger::~FileLogger ()
{
	assert (_formatter.get () != 0);
	_stream << _formatter->GetLogFooter () 
			<< endl;
	_stream.flush ();
}

void FileLogger::Log (LogEntry const & entry)
{
	if (!Accept (entry))
		return;
	assert (_formatter.get () != 0);
	string s = _formatter->Format (entry);
	_stream << s;
	_stream.flush ();
}

void Syslog::Configure ()
{
	_masterSwitch = true;
	// add the logger that writes to stderr
	Logger * logger = new StreamLogger (cerr);
	LoggerPtr pLogger (logger);
	logger->SetName ("stderr");
	AddLogger (pLogger);
}

void Syslog::Configure (Config & cfg, string loggerSuffix)
{
	string section = "Syslog";
	if (loggerSuffix.length () > 0)
	{
		section.append (".");
		section.append (loggerSuffix);
	}
	
	_masterSwitch = cfg.GetBoolValue (section, "MasterSwitch", true);
	// add the logger that writes to stderr, if it is required
	bool fStdErr = cfg.GetBoolValue (section, "StdErr", false);
	if (fStdErr)
	{	
		Syslog::Debug ("add the logger that writes to stderr, if it is required");
		LoggerPtr pStdLogger (new StreamLogger (cerr));
		pStdLogger->SetName ("stderr");
		int level = cfg.GetIntValue (section, "StdErrLogLevel", INFO);
		int mask = GetSeverityMask (level);
		LogFilterPtr pFilter (new BasicLogFilter (mask));
		LogFormatterPtr pFmt (new BasicLogFormatter (cfg));		
		pStdLogger->SetFormatter (pFmt);
		pStdLogger->SetFilter (pFilter);
   		AddLogger (pStdLogger);
	}
    // add a logger that writes to a file if its filename is specified
    if (cfg.Contains (section, "LogFile") && cfg.Contains (section, "LogPath"))
    {
    	Syslog::Debug ("add a logger that writes to a file if its filename is specified");
    	string logFileName = cfg.GetStringValue (section, "LogFile");
    	string logPath = ExpandPath (cfg.GetStringValue (section, "LogPath"));
    	MakeDirectory (logPath);
		char buf[MaxLineLen+MAXHOSTNAMELEN];
		bool append = cfg.GetBoolValue (section, "AppendFile", true);
		bool appendMachineName = cfg.GetBoolValue (section, "AppendMachineName", true); 
		if (appendMachineName)
		{
			Syslog::Debug ("appendMachineName");
			char localHost [MAXHOSTNAMELEN];
			int status = gethostname (localHost, sizeof (localHost));
			sprintf (buf, "%s/%s_%s-%d.log", logPath.c_str(), logFileName.c_str(), localHost, getpid());
		}
		else 
			sprintf (buf, "%s/%s-%d.log", logPath.c_str(), logFileName.c_str(), getpid());
		LoggerPtr pLogger (new FileLogger (buf, append));
		pLogger->SetName ("LogFile");
		// get filter level
		int level = cfg.GetIntValue (section, "LogLevel", DEBUG);
		int mask = GetSeverityMask (level);		
		LogFilterPtr pFilter (new BasicLogFilter (mask));
		pLogger->SetFilter (pFilter);
		LogFormatterPtr pFmt (new BasicLogFormatter (cfg));
		pLogger->SetFormatter (pFmt);			
		AddLogger (pLogger);
	}
	Syslog::Debug ("Configuration taken from section: %s %d", section.c_str (), getpid());
}

// general
void Syslog::LogEvent (LogSeverity s, string const & message)
{
	if (!_masterSwitch)
	   	return;
	// create log entry object	
	LogEntry e (s, message);
 	// log using all loggers
	LoggerIterator iter = _loggers.begin ();
	while (iter != _loggers.end ())
	{
		iter->Log (e);
		iter++;
	}
}

void Syslog::Info (char * formatStr, ...)
{
	char message [512];
	va_list argp;
	va_start (argp, formatStr);
	vsprintf (message, formatStr, argp);
	Syslog::LogEvent (INFO, message);
}

void Syslog::Debug (char * formatStr, ...)
{
	char message [512];
	va_list argp;
	va_start (argp, formatStr);
	vsprintf (message, formatStr, argp);
	Syslog::LogEvent (DEBUG, message);
}

void Syslog::Warn (char * formatStr, ...)
{
	char message [512];
	va_list argp;
	va_start (argp, formatStr);
	vsprintf (message, formatStr, argp);
	Syslog::LogEvent (WARNING, message);
}

void Syslog::Fatal (char * formatStr, ...)
{
	char message [512];
	va_list argp;
	va_start (argp, formatStr);
	vsprintf (message, formatStr, argp);
	Syslog::LogEvent (FATAL, message);
}

void Syslog::Error (char * formatStr, ...)
{
	char message [512];
	va_list argp;
	va_start (argp, formatStr);
	vsprintf (message, formatStr, argp);
	Syslog::LogEvent (ERROR, message);
}

// misc
Logger const * Syslog::GetLogger (string const & name)
{
	LoggerIterator iter = _loggers.begin ();
	while (iter != _loggers.end ())
	{
		string const & loggerName = iter->GetName ();
		if (loggerName == name)
			return *iter; // found
		iter++;
	}
	return 0; // not found
}

void Syslog::AddLogger (LoggerPtr & logger)
{
	_loggers.push_back (logger);
}
	
void Syslog::RemoveLogger (string const & name)
{
	throw "Not implemented";
}

int Syslog::GetSeverityMask (int level)
{
	/* Levels:
		0 = logs all messages (debug, info, warning, error, fatal)
		1 = logs all except debug messages (info, warning, error, fatal), DEFAULT
		2 = logs warnings, errors and fatal errors
		3 = logs only errors and fatal errors
		4 = logs only fatal errors
	*/
	if (level >= 0 && level <= 4)
		return SeverityMasks [level];
	else
		return DEFAULT_SEVERITY_MASK;
}

