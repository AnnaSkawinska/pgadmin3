//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2007, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgAdmin3.cpp - The application
//
//////////////////////////////////////////////////////////////////////////


#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/imagjpeg.h>
#include <wx/imaggif.h>
#include <wx/imagpng.h>

// Windows headers
#ifdef __WXMSW__
  #include <winsock.h>
#endif

// Linux headers
#ifdef __LINUX__
#include <signal.h>
#endif

#if wxCHECK_VERSION(2,5,1)
#ifdef __WXGTK__
#include <wx/renderer.h>
#endif
#endif 

// App headers
#include "copyright.h"
#include "version.h"
#include "misc.h"
#include "sysSettings.h"
#include "update.h"
#include "pgServer.h"
#include "frmMain.h"
#include "frmConfig.h"
#include "frmQuery.h"
#include "frmSplash.h"
#include "dlgSelectConnection.h"
#include <wx/dir.h>
#include <wx/fs_zip.h>
#include "ctl/xh_calb.h"
#include "ctl/xh_timespin.h"
#include "ctl/xh_sqlbox.h"
#include "ctl/xh_ctlcombo.h"
#include "ctl/xh_ctltree.h"

#include "base/appbase.h"

#include <wx/ogl/ogl.h>

#include "frmHint.h"

// Globals
frmMain *winMain=0;
wxThread *updateThread=0;

sysSettings *settings;
wxArrayInt existingLangs;
wxArrayString existingLangNames;
wxLocale *locale=0;
pgAppearanceFactory *appearanceFactory=0;

wxString backupExecutable;      // complete filename of pg_dump and pg_restore, if available
wxString restoreExecutable;


bool dialogTestMode=false;


#define LANG_FILE   wxT("pgadmin3.lng")



// Class declarations
class pgAdmin3 : public pgAppBase
{
public:
    virtual bool OnInit();
    virtual int OnExit();

private:

    bool LoadAllXrc(const wxString dir);
};

IMPLEMENT_APP(pgAdmin3)


#if wxCHECK_VERSION(2,5,1)
#ifdef __WXGTK__

class pgRendererNative : public wxDelegateRendererNative
{
public:
	pgRendererNative() : wxDelegateRendererNative(wxRendererNative::GetDefault()) {}

	void DrawTreeItemButton(wxWindow* win,wxDC& dc, const wxRect& rect, int flags)
	{
		GetGeneric().DrawTreeItemButton(win, dc, rect, flags);
	}
};

#endif
#endif
#define CTL_LB  100
class frmDlgTest : public wxFrame
{
public:
    frmDlgTest();
private:
    void OnSelect(wxCommandEvent &ev);
    wxListBox *dlgList;
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(frmDlgTest, wxFrame)
    EVT_LISTBOX_DCLICK(CTL_LB, frmDlgTest::OnSelect)
END_EVENT_TABLE();


frmDlgTest::frmDlgTest() : wxFrame(0, -1, wxT("pgAdmin III Translation test mode"))
{
    dlgList=new wxListBox(this, CTL_LB);

    // unfortunately, the MemoryFS has no search functions implemented 
    // so we can't extract the names in the EMBED_XRC case

    wxDir dir(uiPath);
    wxString filename;

    bool found=dir.GetFirst(&filename, wxT("*.xrc"));
    while (found)
    {
        dlgList->Append(filename.Left(filename.Length()-4));
        found = dir.GetNext(&filename);
    }
    if (!dlgList->GetCount())
    {
        dlgList->Append(wxT("No xrc files in directory"));
        dlgList->Append(uiPath);
        dlgList->Disable();
    }
}


void frmDlgTest::OnSelect(wxCommandEvent &ev)
{
    wxString dlgName=dlgList->GetStringSelection();
    if (!dlgName.IsEmpty())
    {
        pgDialog *dlg=new pgDialog;
        dlg->wxWindowBase::SetFont(settings->GetSystemFont());
        dlg->LoadResource(this, dlgName); 
        dlg->SetTitle(dlgName);
        dlg->Show();
    }
}


// The Application!
bool pgAdmin3::OnInit()
{
	static const wxCmdLineEntryDesc cmdLineDesc[] = 
	{
		{wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), _("show this help message"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
		{wxCMD_LINE_OPTION, wxT("s"), wxT("server"), _("auto-connect to specified server"), wxCMD_LINE_VAL_STRING},
		{wxCMD_LINE_SWITCH, wxT("q"), wxT("query"), _("open query tool"), wxCMD_LINE_VAL_NONE},
		{wxCMD_LINE_OPTION, wxT("qc"), wxT("queryconnect"), _("connect query tool to database"), wxCMD_LINE_VAL_STRING},
		{wxCMD_LINE_OPTION, wxT("cm"), NULL, _("edit main configuration file"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE},
		{wxCMD_LINE_OPTION, wxT("ch"), NULL, _("edit HBA configuration file"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE},
		{wxCMD_LINE_OPTION, wxT("c"), NULL, _("edit configuration files in cluster directory"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE},
		{wxCMD_LINE_SWITCH, wxT("t"), NULL, _("dialog translation test mode"), wxCMD_LINE_VAL_NONE},
		{wxCMD_LINE_NONE}
	};


    // we are here
    InitPaths();

    frmConfig::tryMode configMode=frmConfig::NONE;
	wxString configFile;

	wxCmdLineParser cmdParser(cmdLineDesc, argc, argv);
	if (cmdParser.Parse() != 0) 
		return false;

	if (cmdParser.Found(wxT("q")) && cmdParser.Found(wxT("qc")))
	{
		cmdParser.Usage();
		return false;
	}

	if (cmdParser.Found(wxT("cm"), &configFile)) 
		configMode = frmConfig::MAINFILE;
	else if (cmdParser.Found(wxT("ch"), &configFile))
		configMode = frmConfig::HBAFILE;
	else if (cmdParser.Found(wxT("c"), &configFile))
		configMode = frmConfig::ANYFILE;

	if (cmdParser.Found(wxT("t")))
		dialogTestMode = true;


    // evaluate all working paths

#if defined(__WXMSW__)
    backupExecutable  = path.FindValidPath(wxT("pg_dump.exe"));
    restoreExecutable = path.FindValidPath(wxT("pg_restore.exe"));
#else
    backupExecutable  = path.FindValidPath(wxT("pg_dump"));
    restoreExecutable = path.FindValidPath(wxT("pg_restore"));
#endif

    // Load the Settings
#ifdef __WXMSW__
    settings = new sysSettings(APPNAME_L);
#else
    settings = new sysSettings(APPNAME_S);
#endif

	// Setup logging
    InitLogger();

    wxString msg;
    msg << wxT("# ") << APPNAME_L << wxT(" Version ") << VERSION_STR << wxT(" Startup");
    wxLogInfo(wxT("##############################################################"));
    wxLogInfo(msg);
    wxLogInfo(wxT("##############################################################"));

#if wxCHECK_VERSION(2,5,0)
    // that's what we expect
#else
    wxLogInfo(wxT("Not compiled against wxWindows 2.5 or above: using ") wxVERSION_STRING);
#endif

#ifdef SSL
    wxLogInfo(wxT("Compiled with dynamically linked SSL support"));
#endif

    // Log the path info
    wxLogInfo(wxT("i18n path: %s"), i18nPath.c_str());
    wxLogInfo(wxT("UI path  : %s"), uiPath.c_str());
    wxLogInfo(wxT("Doc path : %s"), docPath.c_str());

    wxLogInfo(wxT("Executable search directories:"));
    for (unsigned int x=0; x<path.Count(); x++)
        wxLogInfo(wxT("    %s"), path[x].c_str());

#if wxCHECK_VERSION(2,5,1)
#ifdef __WXGTK__
	static pgRendererNative *renderer=new pgRendererNative();
	wxRendererNative::Get();
	wxRendererNative::Set(renderer);
#endif
#endif

#ifdef __LINUX__
	signal(SIGPIPE, SIG_IGN);
#endif

    locale = new wxLocale();
    locale->AddCatalogLookupPathPrefix(i18nPath);

    wxLanguage langId = (wxLanguage)settings->Read(wxT("LanguageId"), wxLANGUAGE_DEFAULT);
    if (locale->Init(langId))
    {
#ifdef __LINUX__
        {
            wxLogNull noLog;
            locale->AddCatalog(wxT("fileutils"));
        }
#endif
        locale->AddCatalog(wxT("pgadmin3"));
    }


    long langCount=0;
    const wxLanguageInfo *langInfo;
    int langNo;

    wxString langfile=FileRead(i18nPath + wxT("/") LANG_FILE, 1);

    if (!langfile.IsEmpty())
    {
        wxStringTokenizer tk(langfile, wxT("\n\r"));

        while (tk.HasMoreTokens())
        {
            wxString line=tk.GetNextToken().Strip(wxString::both);
            if (line.IsEmpty() || line.StartsWith(wxT("#")))
                continue;

            wxString englishName=line.BeforeFirst(',').Trim(true);
            wxString translatedName=line.AfterFirst(',').Trim(false);

            langNo=2;       // skipping default, unknown

            while (true)
            {
                langInfo=wxLocale::GetLanguageInfo(langNo);
                if (!langInfo)
                    break;

                if (englishName == langInfo->Description && 
                    (langInfo->CanonicalName == wxT("en_US") || 
                    (!langInfo->CanonicalName.IsEmpty() && 
                     wxDir::Exists(i18nPath + wxT("/") + langInfo->CanonicalName))))
                {
                    existingLangs.Add(langNo);
                    existingLangNames.Add(translatedName);
                    langCount++;
                }
                langNo++;
            }
        }
    }


#ifndef __WXDEBUG__
    // Show the splash screen
    frmSplash* winSplash = new frmSplash((wxFrame *)NULL);
    if (!winSplash) 
        wxLogError(__("Couldn't create the splash screen!"));
    else
    {
        SetTopWindow(winSplash);
        winSplash->Show();
	    winSplash->Update();
        wxTheApp->Yield(true);
    }
#else
	frmSplash *winSplash = NULL;
#endif

	
    // Startup the windows sockets if required
    InitNetwork();

    wxImage::AddHandler(new wxJPEGHandler());
    wxImage::AddHandler(new wxPNGHandler());
    wxImage::AddHandler(new wxGIFHandler());

    wxFileSystem::AddHandler(new wxZipFSHandler);

    // Setup the XML resources
    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->AddHandler(new wxCalendarBoxXmlHandler);
    wxXmlResource::Get()->AddHandler(new wxTimeSpinXmlHandler);
    wxXmlResource::Get()->AddHandler(new ctlSQLBoxXmlHandler);
    wxXmlResource::Get()->AddHandler(new ctlComboBoxXmlHandler);
    wxXmlResource::Get()->AddHandler(new ctlTreeXmlHandler);


    appearanceFactory = new pgAppearanceFactory();
    InitXml();

    wxOGLInitialize();

    // Set some defaults
    SetAppName(APPNAME_L);

#ifndef __WXDEBUG__
    wxTheApp->Yield(true);
    wxSleep(2);
#endif


    if (configMode)
    {
		if (configMode == frmConfig::ANYFILE && wxDir::Exists(configFile))
		{
			frmConfig::Create(APPNAME_L, configFile + wxT("/pg_hba.conf"), frmConfig::HBAFILE);
			frmConfig::Create(APPNAME_L, configFile + wxT("/postgresql.conf"), frmConfig::MAINFILE);
		}
		else
		{
			frmConfig::Create(APPNAME_L, configFile, configMode);
		}
        if (winSplash)
        {
            winSplash->Close();
            delete winSplash;
        }
    }

    else
    {
        if (dialogTestMode)
        {
//            wxXmlResource::Get()->Load(uiPath + wxT("/*.xrc"));
            wxFrame *dtf=new frmDlgTest();
            dtf->Show();
            SetTopWindow(dtf);
        }
        else if ((cmdParser.Found(wxT("q")) || cmdParser.Found(wxT("qc"))) &&!cmdParser.Found(wxT("s")))
        {
			// -q specified, but not -s. Open a query tool but do *not* open the main window
			pgConn *conn = NULL;
			wxString connstr;

			if (cmdParser.Found(wxT("q")))
			{
				dlgSelectConnection dlg(NULL, NULL);
		        int rc=dlg.Go(conn, NULL);
				if (rc != wxID_OK)
					return false;
				conn = dlg.CreateConn();
			}
			else if (cmdParser.Found(wxT("qc"), &connstr))
			{
				wxString host, database, username, tmps;
				int sslmode=0,port=0;
				wxStringTokenizer tkn(connstr, wxT(" "), wxTOKEN_STRTOK);
				while (tkn.HasMoreTokens())
				{
					wxString str = tkn.GetNextToken();
					if (str.StartsWith(wxT("host="), &host))
						continue;
					if (str.StartsWith(wxT("dbname="), &database))
						continue;
					if (str.StartsWith(wxT("user="), &username))
						continue;
					if (str.StartsWith(wxT("port="), &tmps))
					{
						port = StrToLong(tmps);
						continue;
					}
					if (str.StartsWith(wxT("sslmode="), &tmps))
					{
						if (!tmps.Cmp(wxT("require")))
							sslmode = 1;
						else if (!tmps.Cmp(wxT("prefer")))
							sslmode = 2;
						else if (!tmps.Cmp(wxT("allow")))
							sslmode = 3;
						else if (!tmps.Cmp(wxT("disable")))
							sslmode = 4;
						else
						{
							wxMessageBox(_("Unknown SSL mode: ") + tmps);
							return false;
						}
						continue;
					}
					wxMessageBox(_("Unknown token in connection string: ") + str);
					return false;
				}
				dlgSelectConnection dlg(NULL, NULL);
				conn = dlg.CreateConn(host, database, username, port, sslmode);
			}
			else
			{
				/* Can't happen.. */
				return false;
			}
			if (!conn)
				return false;
			frmQuery *fq = new frmQuery(NULL, wxEmptyString, conn, wxEmptyString);
			fq->Go();
		}
		else
		{
            // Create & show the main form
            winMain = new frmMain(APPNAME_L);

            if (!winMain) 
                wxLogFatalError(__("Couldn't create the main window!"));

            // updateThread = BackgroundCheckUpdates(winMain);

            winMain->Show();
            SetTopWindow(winMain);

	        // Display a Tip if required.
			extern sysSettings *settings;
			wxCommandEvent evt = wxCommandEvent();
			if (winMain && settings->GetShowTipOfTheDay())
			{
				tipOfDayFactory tip(0, 0, 0);
				tip.StartDialog(winMain, 0);
			}

			wxString str;
			if (cmdParser.Found(wxT("s"), &str))
			{
				pgServer *srv = winMain->ConnectToServer(str, !cmdParser.Found(wxT("q")));
				if (srv && cmdParser.Found(wxT("q")))
				{
					pgConn *conn;
					conn = srv->CreateConn();
					if (conn)
					{
						frmQuery *fq = new frmQuery(winMain, wxEmptyString, conn, wxEmptyString);
						fq->Go();
					}
				}
			}
		}

        SetExitOnFrameDelete(true);

        if (winSplash)
        {
            winSplash->Close();
            delete winSplash;
        }
    }

    return true;
}


// Not the Application!
int pgAdmin3::OnExit()
{
    if (updateThread)
    {
        updateThread->Delete();
        delete updateThread;
    }

    // Delete the settings object to ensure settings are saved.
    delete settings;

    return pgAppBase::OnExit();
}

#include "images/pgAdmin3.xpm"
#include "images/elephant32.xpm"
#include "images/splash.xpm"

pgAppearanceFactory::pgAppearanceFactory()
{
}

void pgAppearanceFactory::SetIcons(wxDialog *dlg)
{
    wxIconBundle icons;
    icons.AddIcon(wxIcon(pgAdmin3_xpm));
    icons.AddIcon(wxIcon(elephant32_xpm));
    dlg->SetIcons(icons);
}

void pgAppearanceFactory::SetIcons(wxTopLevelWindow *dlg)
{
    wxIconBundle icons;
    icons.AddIcon(wxIcon(pgAdmin3_xpm));
    icons.AddIcon(wxIcon(elephant32_xpm));
    dlg->SetIcons(icons);
}

char **pgAppearanceFactory::GetSmallIconImage()
{
    return pgAdmin3_xpm;
}

char **pgAppearanceFactory::GetBigIconImage()
{
    return elephant32_xpm;
}

char **pgAppearanceFactory::GetSplashImage()
{
    return splash_xpm;
}


#ifdef __WIN32__
#define SPLASH_FONTSIZE 8
#else
#ifdef __WXMAC__
#define SPLASH_FONTSIZE 11
#else
#if wxCHECK_VERSION(2,5,0)
#define SPLASH_FONTSIZE 9
#else
#define SPLASH_FONTSIZE 11
#endif
#endif
#endif

#define SPLASH_X0       128
#define SPLASH_Y0       281
#define SPLASH_OFFS     15


wxPoint pgAppearanceFactory::GetSplashTextPos()
{
    return wxPoint(SPLASH_X0, SPLASH_Y0);
}


int pgAppearanceFactory::GetSplashTextOffset()
{
    return SPLASH_OFFS;
}


wxFont pgAppearanceFactory::GetSplashTextFont()
{
    wxFont fnt(*wxNORMAL_FONT);
    fnt.SetPointSize(SPLASH_FONTSIZE);
    return fnt;
}


wxColour pgAppearanceFactory::GetSplashTextColour()
{
    return wxColour(255, 255, 255);
}
