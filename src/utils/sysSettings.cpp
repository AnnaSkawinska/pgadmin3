//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// sysSettings.cpp - Settings handling class
//
// Note: This class stores and manages all the applications settings.
//       Settings are all read in the ctor, but may be written either in
//       the relevant SetXXX() member function for rarely written settings
//       or in the dtor for reguarly changed settings such as form sizes.
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"


// wxWindows headers
#include <wx/wx.h>
#include <wx/config.h>
#include <wx/url.h>

// App headers
#include "sysSettings.h"
#include "sysLogger.h"
#include "misc.h"

extern wxString docPath;



sysSettings::sysSettings(const wxString& name) : wxConfig(name)
{
    // Convert setting from pre-1.3
#ifdef __WXMSW__
    DWORD type=0;
    HKEY hkey=0;
    RegOpenKeyEx(HKEY_CURRENT_USER, wxT("Software\\") + GetAppName(), 0, KEY_READ, &hkey);
    if (hkey)
    {
        RegQueryValueEx(hkey, wxT("ShowTipOfTheDay"), 0, &type, 0, 0);
        if (type == REG_DWORD)
        {
            long value;
            Read(wxT("ShowTipOfTheDay"), &value, 0L);

            Write(wxT("ShowTipOfTheDay"), value != 0);
        }
        RegCloseKey(hkey);
    }
#endif

    // Tip Of The Day
    Read(wxT("ShowTipOfTheDay"), &showTipOfTheDay, true); 
    Read(wxT("NextTipOfTheDay"), &nextTipOfTheDay, 0); 

    // Log. Try to get a vaguely usable default path.
    char *homedir;
#ifdef __WXMSW__
    char *homedrive;
#endif

    wxString deflog;
    
#ifdef __WXMSW__
    homedrive = getenv("HOMEDRIVE");
    homedir = getenv("HOMEPATH");
#else
    homedir = getenv("HOME");
#endif

    if (!homedir)
        deflog = wxT("pgadmin.log");
    else 
    {
        
#ifdef __WXMSW__
        deflog = wxString::FromAscii(homedrive);
        deflog += wxString::FromAscii(homedir);
        deflog += wxT("\\pgadmin.log");
#else
        deflog = wxString::FromAscii(homedir);
        deflog += wxT("/pgadmin.log");
#endif
    }

    Read(wxT("LogFile"), &logFile, deflog); 
    Read(wxT("LogLevel"), &logLevel, LOG_ERRORS);
    sysLogger::logFile = logFile;
    sysLogger::logLevel = logLevel;

    // Last Connection
    Read(wxT("LastServer"), &lastServer, wxT("localhost")); 
    Read(wxT("LastDatabase"), &lastDatabase, wxEmptyString); 
	Read(wxT("LastDescription"), &lastDescription, wxT("PostgreSQL Server")); 
    Read(wxT("LastUsername"), &lastUsername, wxT("postgres")); 
    Read(wxT("LastPort"), &lastPort, 5432);
    Read(wxT("LastSSL"), &lastSSL, 0);

    // Show System Objects
    Read(wxT("ShowSystemObjects"), &showSystemObjects, false); 

//    Read(wxT("SqlHelpSite"), &sqlHelpSite, docPath + wxT("/en_US/pg/"));
    Read(wxT("SqlHelpSite"), &sqlHelpSite, wxT(""));
    if (sqlHelpSite.length() > 0) {
        if (sqlHelpSite.Last() != '/' && sqlHelpSite.Last() != '\\')
            sqlHelpSite += wxT("/");
    }
    Read(wxT("Proxy"), &proxy, wxGetenv(wxT("HTTP_PROXY")));
    SetProxy(proxy);

    maxRows=Read(wxT("frmQuery/MaxRows"), 100L);
    maxColSize=Read(wxT("frmQuery/MaxColSize"), 256L);
    Read(wxT("frmQuery/ExplainVerbose"), &explainVerbose, false);
    Read(wxT("frmQuery/ExplainAnalyze"), &explainAnalyze, false);
    askSaveConfirmation=StrToBool(Read(wxT("AskSaveConfirmation"), wxT("Yes")));
    confirmDelete=StrToBool(Read(wxT("ConfirmDelete"), wxT("Yes")));
    showUsersForPrivileges=StrToBool(Read(wxT("ShowUsersForPrivileges"), wxT("No")));
    autoRowCountThreshold=Read(wxT("AutoRowCount"), 2000);
    Read(wxT("StickySql"), &stickySql, false);
    Read(wxT("DoubleClickProperties"), &doubleClickProperties, true);
    Read(wxT("SuppressGuruHints"), &suppressGuruHints, false);
    Read(wxT("WriteUnicodeFile"), &unicodeFile, false);
    Read(wxT("SystemSchemas"), &systemSchemas, wxEmptyString);
    Read(wxT("MaxServerLogSize"), &maxServerLogSize, 100000L);
    Read(wxT("Export/Unicode"), &exportUnicode, false);
    Read(wxT("SlonyPath"), &slonyPath, wxEmptyString);

    wxString val;
#ifdef __WXMSW__
    Read(wxT("Export/RowSeparator"), &val, wxT("CR/LF"));
#else
    Read(wxT("Export/RowSeparator"), &val, wxT("LF"));
#endif
    if (val == wxT("CRLF"))
        exportRowSeparator = wxT("\r\n");
    else
        exportRowSeparator = wxT("\n");
    Read(wxT("Export/ColSeparator"), &exportColSeparator, wxT(";"));
    Read(wxT("Export/QuoteChar"), &exportQuoteChar, wxT("\""));
    Read(wxT("Export/Quote"), &val, wxT("Strings"));
    if (val == wxT("All"))
        exportQuoting = 2;
    else if (val == wxT("Strings"))
        exportQuoting = 1;
    else
        exportQuoting = 0;



    const wxLanguageInfo *langInfo;
    langInfo = wxLocale::GetLanguageInfo(Read(wxT("LanguageId"), wxLANGUAGE_UNKNOWN));
    if (langInfo)
        canonicalLanguage=langInfo->CanonicalName;

	wxString fontName;
    Read(wxT("Font"), &fontName, wxEmptyString);

    if (fontName.IsEmpty())
        systemFont = wxSystemSettings::GetFont(wxSYS_ICONTITLE_FONT);
    else
        systemFont = wxFont(fontName);

    Read(wxT("frmQuery/Font"), &fontName, wxEmptyString);

    if (fontName.IsEmpty())
    {
#ifdef __WXMSW__
        sqlFont = wxFont(9, wxTELETYPE, wxNORMAL, wxNORMAL);
#else
        sqlFont = wxFont(12, wxTELETYPE, wxNORMAL, wxNORMAL);
#endif
    }
    else
    	sqlFont = wxFont(fontName);
}


sysSettings::~sysSettings()
{
    wxLogInfo(wxT("Destroying sysSettings object and saving settings"));
    // frMain size/position
	Save();
}

void sysSettings::Save()
{
    Write(wxT("LogFile"), logFile);
    Write(wxT("LogLevel"), logLevel);

    Write(wxT("frmQuery/MaxRows"), maxRows);
    Write(wxT("frmQuery/MaxColSize"), maxColSize);
    Write(wxT("frmQuery/ExplainVerbose"), explainVerbose);
    Write(wxT("frmQuery/ExplainAnalyze"), explainAnalyze);
	Write(wxT("frmQuery/Font"), sqlFont.GetNativeFontInfoDesc());
    Write(wxT("AskSaveConfirmation"), BoolToStr(askSaveConfirmation));
    Write(wxT("ConfirmDelete"), BoolToStr(confirmDelete));
    Write(wxT("ShowUsersForPrivileges"), BoolToStr(showUsersForPrivileges));
    Write(wxT("SqlHelpSite"), sqlHelpSite);
    Write(wxT("Proxy"), proxy);
    Write(wxT("AutoRowCount"), autoRowCountThreshold);
    Write(wxT("WriteUnicodeFile"), unicodeFile);
    Write(wxT("SystemSchemas"), systemSchemas);
    Write(wxT("MaxServerLogSize"), maxServerLogSize);
    Write(wxT("SuppressGuruHints"), suppressGuruHints);
    Write(wxT("SlonyPath"), slonyPath);


    Write(wxT("Export/Unicode"), exportUnicode);
    Write(wxT("Export/QuoteChar"), exportQuoteChar);
    Write(wxT("Export/ColSeparator"), exportColSeparator);
    if (exportRowSeparator == wxT("\r\n"))
        Write(wxT("Export/RowSeparator"), wxT("CR/LF"));
    else
        Write(wxT("Export/RowSeparator"), wxT("LF"));


    switch(exportQuoting)
    {
        case 2:
            Write(wxT("Export/Quote"), wxT("All"));
            break;
        case 1:
            Write(wxT("Export/Quote"), wxT("Strings"));
            break;
        case 0:
            Write(wxT("Export/Quote"), wxT("None"));
            break;
        default:
            break;
    }

    wxString fontName = systemFont.GetNativeFontInfoDesc();

	if (fontName == wxSystemSettings::GetFont(wxSYS_ICONTITLE_FONT).GetNativeFontInfoDesc())
        Write(wxT("Font"), wxEmptyString);
    else
        Write(wxT("Font"), fontName);
}


void sysSettings::SetProxy(const wxString &s)
{
    proxy=s;
    if (!s.IsEmpty() && s.Find(':') < 0)
        proxy += wxT(":80");
    wxURL::SetDefaultProxy(proxy);
}


bool sysSettings::Read(const wxString& key, bool *val, bool defaultVal) const
{
    wxString str;
    Read(key, &str, BoolToStr(defaultVal));
	*val = StrToBool(str);
	return true;
}

bool sysSettings::Write(const wxString &key, bool value)
{
    return Write(key, BoolToStr(value));
}

bool sysSettings::Write(const wxString &key, const wxPoint &value)
{
    bool rc=wxConfig::Write(key + wxT("/Left"), value.x);
    if (rc)
        rc=wxConfig::Write(key + wxT("/Top"), value.y);
    return rc;
}


bool sysSettings::Write(const wxString &key, const wxSize &value)
{
    bool rc=wxConfig::Write(key + wxT("/Width"), value.x);
    if (rc)
        rc=wxConfig::Write(key + wxT("/Height"), value.y);
    return rc;
}


wxPoint sysSettings::Read(const wxString& key, const wxPoint &defaultVal) const
{
    return wxPoint(wxConfig::Read(key + wxT("/Left"), defaultVal.x), 
                   wxConfig::Read(key + wxT("/Top"), defaultVal.y));
}


wxSize sysSettings::Read(const wxString& key, const wxSize &defaultVal) const
{
    return wxSize(wxConfig::Read(key + wxT("/Width"), defaultVal.x), 
                  wxConfig::Read(key + wxT("/Height"), defaultVal.y));
}

//////////////////////////////////////////////////////////////////////////
// Tip of the Day
//////////////////////////////////////////////////////////////////////////

void sysSettings::SetShowTipOfTheDay(const bool newval)
{
    showTipOfTheDay = newval;
    Write(wxT("ShowTipOfTheDay"), showTipOfTheDay);
}

void sysSettings::SetNextTipOfTheDay(const int newval)
{
    nextTipOfTheDay = newval;
    Write(wxT("NextTipOfTheDay"), nextTipOfTheDay);
}

//////////////////////////////////////////////////////////////////////////
// Log
//////////////////////////////////////////////////////////////////////////

void sysSettings::SetLogFile(const wxString& newval)
{
    logFile = newval;
    sysLogger::logFile = newval;
}

void sysSettings::SetLogLevel(const int newval)
{
    logLevel = newval;
    sysLogger::logLevel = newval;
}

//////////////////////////////////////////////////////////////////////////
// Last Connection
//////////////////////////////////////////////////////////////////////////

void sysSettings::SetLastServer(const wxString& newval)
{
    lastServer = newval;
    Write(wxT("LastServer"), lastServer);
}

void sysSettings::SetLastDescription(const wxString& newval)
{
    lastDescription = newval;
    Write(wxT("LastDescription"), lastDescription);
}

void sysSettings::SetLastDatabase(const wxString& newval)
{
    lastDatabase = newval;
    Write(wxT("LastDatabase"), lastDatabase);
}

void sysSettings::SetLastUsername(const wxString& newval)
{
    lastUsername = newval;
    Write(wxT("LastUsername"), lastUsername);
}

void sysSettings::SetLastPort(const int newval)
{
    lastPort = newval;
    Write(wxT("LastPort"), lastPort);
}

void sysSettings::SetLastSSL(const int newval)
{
    lastSSL = newval;
    Write(wxT("LastSSL"), lastSSL);
}

//////////////////////////////////////////////////////////////////////////
// Show System Objects
//////////////////////////////////////////////////////////////////////////

void sysSettings::SetShowSystemObjects(const bool newval)
{
    showSystemObjects = newval;
    Write(wxT("ShowSystemObjects"), showSystemObjects);
}


//////////////////////////////////////////////////////////////////////////
// Sticky SQL
//////////////////////////////////////////////////////////////////////////

void sysSettings::SetStickySql(const bool newval)
{
    stickySql = newval;
    Write(wxT("StickySql"), stickySql);
}

//////////////////////////////////////////////////////////////////////////
// Double click for properties
//////////////////////////////////////////////////////////////////////////

void sysSettings::SetDoubleClickProperties(const bool newval)
{
    doubleClickProperties = newval;
    Write(wxT("DoubleClickProperties"), doubleClickProperties);
}
