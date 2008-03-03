//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2008, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// frmOptions.cpp - The main options dialogue
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/fontdlg.h>
#include <wx/fontutil.h>
#include <wx/file.h>

// App headers
#include "frm/frmOptions.h"
#include "frm/frmMain.h"
#include "frm/frmHint.h"
#include "utils/sysSettings.h"
#include "utils/sysLogger.h"
#include "utils/misc.h"
#include "frm/menu.h"

#include "images/properties.xpm"

extern wxLocale *locale;
extern wxArrayInt existingLangs;
extern wxArrayString existingLangNames;

extern wxString pgBackupExecutable;
extern wxString pgBackupAllExecutable;
extern wxString pgRestoreExecutable;

extern wxString edbBackupExecutable;
extern wxString edbBackupAllExecutable;
extern wxString edbRestoreExecutable;

#define nbOptions                   CTRL_NOTEBOOK("nbOptions")
#define txtPgHelpPath               CTRL_TEXT("txtPgHelpPath")
#define txtEdbHelpPath              CTRL_TEXT("txtEdbHelpPath")
#define txtSlonyHelpPath            CTRL_TEXT("txtSlonyHelpPath")
#define txtSlonyPath                CTRL_TEXT("txtSlonyPath")
#define txtPostgresqlPath           CTRL_TEXT("txtPostgresqlPath")
#define txtEnterprisedbPath         CTRL_TEXT("txtEnterprisedbPath")
#define txtSystemSchemas            CTRL_TEXT("txtSystemSchemas")
#define txtLogfile                  CTRL_TEXT("txtLogfile")
#define radLoglevel                 CTRL_RADIOBOX("radLoglevel")
#define txtMaxRows                  CTRL_TEXT("txtMaxRows")
#define txtMaxColSize               CTRL_TEXT("txtMaxColSize")
#define txtFont                     CTRL_TEXT("txtFont")
#define chkUnicodeFile              CTRL_CHECKBOX("chkUnicodeFile")
#define chkAskSaveConfirm           CTRL_CHECKBOX("chkAskSaveConfirm")
#define chkAskDelete                CTRL_CHECKBOX("chkAskDelete")
#define chkShowUsersForPrivileges   CTRL_CHECKBOX("chkShowUsersForPrivileges")
#define txtAutoRowCount             CTRL_TEXT("txtAutoRowCount")
#define txtIndent                   CTRL_TEXT("txtIndent")
#define chkSpacesForTabs			CTRL_CHECKBOX("chkSpacesForTabs")
#define cbCopyQuote					CTRL_COMBOBOX("cbCopyQuote")
#define cbCopyQuoteChar				CTRL_COMBOBOX("cbCopyQuoteChar")
#define cbCopySeparator				CTRL_COMBOBOX("cbCopySeparator")
#define chkTabForCompletion			CTRL_CHECKBOX("chkTabForCompletion")
#define chkStickySql                CTRL_CHECKBOX("chkStickySql")
#define chkIndicateNull             CTRL_CHECKBOX("chkIndicateNull")
#define chkDoubleClickProperties    CTRL_CHECKBOX("chkDoubleClickProperties")
#define cbLanguage                  CTRL_COMBOBOX("cbLanguage")
#define txtSqlFont                  CTRL_TEXT("txtSqlFont")
#define chkSuppressHints            CTRL_CHECKBOX("chkSuppressHints")
#define chkResetHints               CTRL_CHECKBOX("chkResetHints")
#define lstDisplay					CTRL_CHECKLISTBOX("lstDisplay")
#define chkSystemObjects            CTRL_CHECKBOX("chkSystemObjects")

BEGIN_EVENT_TABLE(frmOptions, pgDialog)
    EVT_MENU(MNU_HELP,                        frmOptions::OnHelp)
    EVT_BUTTON (XRCID("btnFont"),             frmOptions::OnFontSelect)
    EVT_BUTTON (XRCID("btnSqlFont"),          frmOptions::OnSqlFontSelect)
    EVT_BUTTON (XRCID("btnBrowseLogfile"),    frmOptions::OnBrowseLogFile)
    EVT_BUTTON (XRCID("btnSlonyPath"),        frmOptions::OnSlonyPathSelect)
    EVT_BUTTON (XRCID("btnPostgresqlPath"),   frmOptions::OnPostgresqlPathSelect)
    EVT_BUTTON (XRCID("btnEnterprisedbPath"), frmOptions::OnEnterprisedbPathSelect)
    EVT_BUTTON (XRCID("btnDefault"),          frmOptions::OnDefault)
    EVT_CHECKBOX(XRCID("chkSuppressHints"),   frmOptions::OnSuppressHints)
    EVT_CHECKBOX(XRCID("chkResetHints"),      frmOptions::OnResetHints)
    EVT_BUTTON (wxID_OK,                      frmOptions::OnOK)
    EVT_BUTTON (wxID_HELP,                    frmOptions::OnHelp)
    EVT_BUTTON (wxID_CANCEL,                  frmOptions::OnCancel)
    EVT_COMBOBOX(XRCID("cbCopyQuote"),		  frmOptions::OnChangeCopyQuote)
END_EVENT_TABLE()

frmOptions::frmOptions(frmMain *parent)
{
    mainForm=parent;
    wxWindowBase::SetFont(settings->GetSystemFont());
    LoadResource(parent, wxT("frmOptions")); 

    // Icon
    SetIcon(wxIcon(properties_xpm));
    RestorePosition();

    wxAcceleratorEntry entries[1];

    entries[0].Set(wxACCEL_NORMAL,              WXK_F1,        MNU_HELP);

    wxAcceleratorTable accel(1, entries);
    SetAcceleratorTable(accel);


    wxTextValidator numval(wxFILTER_NUMERIC);
    txtMaxRows->SetValidator(numval);
    txtMaxColSize->SetValidator(numval);
    txtAutoRowCount->SetValidator(numval);
    txtIndent->SetValidator(numval);
    
    txtLogfile->SetValue(settings->GetLogFile());
    radLoglevel->SetSelection(settings->GetLogLevel());
    txtMaxRows->SetValue(NumToStr(settings->GetMaxRows()));
    txtMaxColSize->SetValue(NumToStr(settings->GetMaxColSize()));
    chkAskSaveConfirm->SetValue(!settings->GetAskSaveConfirmation());
    chkAskDelete->SetValue(settings->GetConfirmDelete());
    chkShowUsersForPrivileges->SetValue(settings->GetShowUsersForPrivileges());
    txtAutoRowCount->SetValue(NumToStr(settings->GetAutoRowCountThreshold()));
    txtIndent->SetValue(NumToStr(settings->GetIndentSpaces()));
    chkSpacesForTabs->SetValue(settings->GetSpacesForTabs());
	cbCopyQuote->SetSelection(settings->GetCopyQuoting());
	cbCopyQuoteChar->SetValue(settings->GetCopyQuoteChar());

    wxString copySeparator = settings->GetCopyColSeparator();
    if (copySeparator == wxT("\t"))
        copySeparator = _("Tab");
    cbCopySeparator->SetValue(copySeparator);

	chkTabForCompletion->SetValue(settings->GetTabForCompletion());
    chkStickySql->SetValue(settings->GetStickySql());
    chkIndicateNull->SetValue(settings->GetIndicateNull());
    chkDoubleClickProperties->SetValue(settings->GetDoubleClickProperties());

    txtPgHelpPath->SetValue(settings->GetPgHelpPath());
    txtEdbHelpPath->SetValue(settings->GetEdbHelpPath());
    txtSlonyHelpPath->SetValue(settings->GetSlonyHelpPath());
    
    txtSystemSchemas->SetValue(settings->GetSystemSchemas());
    chkUnicodeFile->SetValue(settings->GetUnicodeFile());
    chkSuppressHints->SetValue(settings->GetSuppressGuruHints());
    txtSlonyPath->SetValue(settings->GetSlonyPath());
    txtPostgresqlPath->SetValue(settings->GetPostgresqlPath());
    txtEnterprisedbPath->SetValue(settings->GetEnterprisedbPath());

    cbLanguage->Append(_("Default"));
    int sel=0;
    int langId=settings->Read(wxT("LanguageId"), wxLANGUAGE_UNKNOWN);

    int langCount=existingLangs.GetCount();
    if (langCount)
    {
        int langNo;
        const wxLanguageInfo *langInfo;
        for (langNo = 0; langNo < langCount ; langNo++)
        {
            langInfo = wxLocale::GetLanguageInfo(existingLangs.Item(langNo));
            cbLanguage->Append(wxT("(") + langInfo->CanonicalName + wxT(") ")
                + existingLangNames.Item(langNo));
            if (langId == langInfo->Language)
                sel=langNo+1;
        }
    }
    cbLanguage->SetSelection(sel);

    currentFont=settings->GetSystemFont();
    txtFont->SetValue(currentFont.GetNativeFontInfoUserDesc());
    currentSqlFont=settings->GetSQLFont();
    txtSqlFont->SetValue(currentSqlFont.GetNativeFontInfoUserDesc());

	// Load the display options
	lstDisplay->Append(_("Databases"));
	lstDisplay->Append(_("Tablespaces"));
	lstDisplay->Append(_("pgAgent jobs"));
	lstDisplay->Append(_("Groups/group roles"));
	lstDisplay->Append(_("Users/login roles"));
	lstDisplay->Append(_("Catalogs"));
	lstDisplay->Append(_("Casts"));
	lstDisplay->Append(_("Languages"));
	lstDisplay->Append(_("Public synonyms"));
	lstDisplay->Append(_("Schemas"));
	lstDisplay->Append(_("Slony-I clusters"));
	lstDisplay->Append(_("Aggregates"));
	lstDisplay->Append(_("Conversions"));
	lstDisplay->Append(_("Domains"));
	lstDisplay->Append(_("Functions"));
	lstDisplay->Append(_("Trigger functions"));
	lstDisplay->Append(_("Packages"));
	lstDisplay->Append(_("Procedures"));
	lstDisplay->Append(_("Operators"));
	lstDisplay->Append(_("Operator classes"));
	lstDisplay->Append(_("Operator families"));
	lstDisplay->Append(_("Rules"));
	lstDisplay->Append(_("Sequences"));
	lstDisplay->Append(_("Tables"));
	lstDisplay->Append(_("Types"));
	lstDisplay->Append(_("Views"));

	for (unsigned int x=0; x < lstDisplay->GetCount(); x++)
		lstDisplay->Check(x, settings->GetDisplayOption(lstDisplay->GetString(x)));

    chkSystemObjects->SetValue(settings->GetShowSystemObjects());

	wxCommandEvent e;
	OnChangeCopyQuote(e);
}


frmOptions::~frmOptions()
{
    SavePosition();
}

void frmOptions::OnHelp(wxCommandEvent &ev)
{
    long page=nbOptions->GetSelection();
    DisplayHelp(wxT("options-tab") + NumToStr(page+1L), HELP_PGADMIN);
}

void frmOptions::OnDefault(wxCommandEvent &ev)
{
    // Reset the display options to the defaults.
    // Clear them all first
    for (unsigned int x=0; x < lstDisplay->GetCount(); x++)
		lstDisplay->Check(x, settings->GetDisplayOption(lstDisplay->GetString(x), true));
}

void frmOptions::OnSlonyPathSelect(wxCommandEvent &ev)
{
    wxDirDialog dlg(this, _("Select directory with Slony-I creation scripts"), txtSlonyPath->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        txtSlonyPath->SetValue(dlg.GetPath());
}

void frmOptions::OnPostgresqlPathSelect(wxCommandEvent &ev)
{
    wxDirDialog dlg(this, _("Select directory with PostgreSQL utilities"), txtPostgresqlPath->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        txtPostgresqlPath->SetValue(dlg.GetPath());
}

void frmOptions::OnEnterprisedbPathSelect(wxCommandEvent &ev)
{
    wxDirDialog dlg(this, _("Select directory with EnterpriseDB utilities"), txtEnterprisedbPath->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        txtEnterprisedbPath->SetValue(dlg.GetPath());
}

void frmOptions::OnSuppressHints(wxCommandEvent &ev)
{
    if (chkSuppressHints->GetValue())
        chkResetHints->SetValue(false);
}


void frmOptions::OnResetHints(wxCommandEvent &ev)
{
    if (chkResetHints->GetValue())
        chkSuppressHints->SetValue(false);
}


void frmOptions::OnOK(wxCommandEvent &ev)
{
    // Check the PostgreSQL and EnterpriseDB paths
#ifdef __WXMSW__
    if (!txtPostgresqlPath->GetValue().IsEmpty() && !isPgApp(txtPostgresqlPath->GetValue() + wxT("\\pg_dump.exe")))
#else
    if (!txtPostgresqlPath->GetValue().IsEmpty() && !isPgApp(txtPostgresqlPath->GetValue() + wxT("/pg_dump")))
#endif
    {
        wxMessageBox(_("The PostgreSQL bin path specified is not valid or does not contain a PostgreSQL pg_dump executable.\n\nPlease select another directory, or leave the path blank."), _("Error"), wxICON_ERROR); 
        txtPostgresqlPath->SetFocus();
        return;
    }   

#ifdef __WXMSW__
    if (!txtEnterprisedbPath->GetValue().IsEmpty() && !isEdbApp(txtEnterprisedbPath->GetValue() + wxT("\\pg_dump.exe")))
#else
    if (!txtEnterprisedbPath->GetValue().IsEmpty() && !isEdbApp(txtEnterprisedbPath->GetValue() + wxT("/pg_dump")))
#endif
    {
        wxMessageBox(_("The EnterpriseDB bin path specified is not valid or does not contain an EnterpriseDB pg_dump executable.\n\nPlease select another directory, or leave the path blank."), _("Error"), wxICON_ERROR); 
        txtEnterprisedbPath->SetFocus();
        return;
    }

    // Clean and check the help paths
    txtPgHelpPath->SetValue(CleanHelpPath(txtPgHelpPath->GetValue()));
    if (!HelpPathValid(txtPgHelpPath->GetValue()))
    {
        wxMessageBox(_("An invalid PostgreSQL help path was specified.\n\nPlease enter another filename, directory or URL, or leave the path blank."), _("Error"), wxICON_ERROR);
        txtPgHelpPath->SetFocus();
        return;
    }

    txtEdbHelpPath->SetValue(CleanHelpPath(txtEdbHelpPath->GetValue()));
    if (!HelpPathValid(txtEdbHelpPath->GetValue()))
    {
        wxMessageBox(_("An invalid EnterpriseDB help path was specified.\n\nPlease enter another filename, directory or URL, or leave the path blank."), _("Error"), wxICON_ERROR);
        txtEdbHelpPath->SetFocus();
        return;
    }

    txtSlonyHelpPath->SetValue(CleanHelpPath(txtSlonyHelpPath->GetValue()));
    if (!HelpPathValid(txtSlonyHelpPath->GetValue()))
    {
        wxMessageBox(_("An invalid Slony help path was specified.\n\nPlease enter another filename, directory or URL, or leave the path blank."), _("Error"), wxICON_ERROR);
        txtSlonyHelpPath->SetFocus();
        return;
    }

    // Logfile
    wxString logFile = txtLogfile->GetValue();
    wxLogInfo(wxT("Setting logfile to: %s"), logFile.c_str());
    settings->SetLogFile(logFile);

    // Loglevel
    wxString logInfo = radLoglevel->GetStringSelection();
    wxLogInfo(wxT("Setting loglevel to: %s"),logInfo.c_str());
    int sel = radLoglevel->GetSelection();

    switch(sel) {
        case(0):
          settings->SetLogLevel(LOG_NONE);
          break;
        case(1):
          settings->SetLogLevel(LOG_ERRORS);
          break;
        case(2):
          settings->SetLogLevel(LOG_NOTICE);
          break;
        case(3):
          settings->SetLogLevel(LOG_SQL);
          break;
        case(4):
          settings->SetLogLevel(LOG_DEBUG);
          break;
        default:
          settings->SetLogLevel(LOG_ERRORS);
          break;
    }

    // Query parameter
    settings->SetMaxRows(StrToLong(txtMaxRows->GetValue()));
    settings->SetMaxColSize(StrToLong(txtMaxColSize->GetValue()));

    settings->SetAskSaveConfirmation(!chkAskSaveConfirm->GetValue());
    settings->SetConfirmDelete(chkAskDelete->GetValue());
    settings->SetShowUsersForPrivileges(chkShowUsersForPrivileges->GetValue());
    settings->SetAutoRowCountThreshold(StrToLong(txtAutoRowCount->GetValue()));
    settings->SetIndentSpaces(StrToLong(txtIndent->GetValue()));
    settings->SetSpacesForTabs(chkSpacesForTabs->GetValue());
	settings->SetCopyQuoting(cbCopyQuote->GetCurrentSelection());
	settings->SetCopyQuoteChar(cbCopyQuoteChar->GetValue());
	
    wxString copySeparator = cbCopySeparator->GetValue();
    if (copySeparator == _("Tab"))
        copySeparator = wxT("\t");
    settings->SetCopyColSeparator(copySeparator);

	settings->SetTabForCompletion(chkTabForCompletion->GetValue());
    settings->SetStickySql(chkStickySql->GetValue());
    settings->SetIndicateNull(chkIndicateNull->GetValue());
    settings->SetDoubleClickProperties(chkDoubleClickProperties->GetValue());
    settings->SetUnicodeFile(chkUnicodeFile->GetValue());
    settings->SetFont(currentFont);
    settings->SetSQLFont(currentSqlFont);
    settings->SetSuppressGuruHints(chkSuppressHints->GetValue());
    settings->SetSlonyPath(txtSlonyPath->GetValue());
    settings->SetPostgresqlPath(txtPostgresqlPath->GetValue());
    settings->SetEnterprisedbPath(txtEnterprisedbPath->GetValue());

    // Setup PostgreSQL/EnterpriseDB working paths
#if defined(__WXMSW__)
    pgBackupExecutable  = settings->GetPostgresqlPath() + wxT("\\pg_dump.exe");
    pgBackupAllExecutable  = settings->GetPostgresqlPath() + wxT("\\pg_dumpall.exe");
    pgRestoreExecutable = settings->GetPostgresqlPath() + wxT("\\pg_restore.exe");

    edbBackupExecutable  = settings->GetEnterprisedbPath() + wxT("\\pg_dump.exe");
    edbBackupAllExecutable  = settings->GetEnterprisedbPath() + wxT("\\pg_dumpall.exe");
    edbRestoreExecutable = settings->GetEnterprisedbPath() + wxT("\\pg_restore.exe");
#else
    pgBackupExecutable  = settings->GetPostgresqlPath() + wxT("/pg_dump");
	pgBackupAllExecutable  = settings->GetPostgresqlPath() + wxT("/pg_dumpall");
    pgRestoreExecutable = settings->GetPostgresqlPath() + wxT("/pg_restore");

    edbBackupExecutable  = settings->GetEnterprisedbPath() + wxT("/pg_dump");
	edbBackupAllExecutable  = settings->GetEnterprisedbPath() + wxT("/pg_dumpall");
    edbRestoreExecutable = settings->GetEnterprisedbPath() + wxT("/pg_restore");
#endif

    if (!wxFile::Exists(pgBackupExecutable))
        pgBackupExecutable = wxEmptyString;
    if (!wxFile::Exists(pgBackupAllExecutable))
        pgBackupAllExecutable = wxEmptyString;
    if (!wxFile::Exists(pgRestoreExecutable))
        pgRestoreExecutable = wxEmptyString;

    if (!wxFile::Exists(edbBackupExecutable))
        edbBackupExecutable = wxEmptyString;
    if (!wxFile::Exists(edbBackupAllExecutable))
        edbBackupAllExecutable = wxEmptyString;
    if (!wxFile::Exists(edbRestoreExecutable))
        edbRestoreExecutable = wxEmptyString;

    if (chkResetHints->GetValue())
        frmHint::ResetHints();

    // Set the help paths
    settings->SetPgHelpPath(txtPgHelpPath->GetValue());
    settings->SetEdbHelpPath(txtEdbHelpPath->GetValue());
    settings->SetSlonyHelpPath(txtSlonyHelpPath->GetValue());

    settings->SetSystemSchemas(txtSystemSchemas->GetValue());

    int langNo=cbLanguage->GetCurrentSelection();
    if (langNo >= 0)
    {
        wxLanguage langId;
        if (langNo == 0)
            langId = wxLANGUAGE_DEFAULT;
        else
        {
            const wxLanguageInfo *langInfo=wxLocale::GetLanguageInfo(existingLangs.Item(langNo-1));
            langId = (wxLanguage) langInfo->Language;
        }
        if (langId != (wxLanguage)settings->Read(wxT("LanguageId"), wxLANGUAGE_UNKNOWN))
        {
            delete locale;
            locale = new wxLocale();
            if (locale->Init(langId))
            {
#ifdef __LINUX__
                {
                    wxLogNull noLog;
                    locale->AddCatalog(wxT("fileutils"));
                }
#endif
                locale->AddCatalog(wxT("pgadmin3"));
                settings->Write(wxT("LanguageId"), (long)langId);
            }
        }

    }

	// Save the display options
	int changed = false;
	for (unsigned int x=0; x < lstDisplay->GetCount(); x++)
	{
		if (lstDisplay->IsChecked(x) != settings->GetDisplayOption(lstDisplay->GetString(x)))
        {
			changed = true;
		    settings->SetDisplayOption(lstDisplay->GetString(x), lstDisplay->IsChecked(x));
        }
	}

    if (chkSystemObjects->GetValue() != settings->GetShowSystemObjects())
    {
        changed = true;
        settings->SetShowSystemObjects(chkSystemObjects->GetValue());
    }

	if (changed)
		wxMessageBox(_("Changes to the display options may not be visible until the browser tree is refreshed."), _("Display options"), wxICON_INFORMATION);

    settings->Save();
    Destroy();
}



void frmOptions::OnSqlFontSelect(wxCommandEvent &ev)
{
    wxFontData fd;
    fd.SetInitialFont(settings->GetSQLFont());
    wxFontDialog dlg(this, fd);

    if (dlg.ShowModal() == wxID_OK)
    {
        currentSqlFont=dlg.GetFontData().GetChosenFont();
        txtSqlFont->SetValue(currentSqlFont.GetNativeFontInfoUserDesc());
    }
}


void frmOptions::OnFontSelect(wxCommandEvent &ev)
{
    wxFontData fd;
    fd.SetInitialFont(settings->GetSystemFont());
    wxFontDialog dlg(this, fd);

    if (dlg.ShowModal() == wxID_OK)
    {
        currentFont=dlg.GetFontData().GetChosenFont();
        txtFont->SetValue(currentFont.GetNativeFontInfoUserDesc());
    }
}
void frmOptions::OnCancel(wxCommandEvent &ev)
{
    Destroy();
}

void frmOptions::OnBrowseLogFile(wxCommandEvent &ev)
{
    wxFileDialog logFile(this, _("Select log file"), wxT(""), wxT(""), _("Log files (*.log)|*.log|All files (*.*)|*.*"));
    logFile.SetDirectory(wxGetHomeDir());

    if (logFile.ShowModal() == wxID_OK)
        txtLogfile->SetValue(logFile.GetPath());
}


optionsFactory::optionsFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : actionFactory(list)
{
    mnu->Append(id, _("&Options..."), _("Show options dialog."));
}


wxWindow *optionsFactory::StartDialog(frmMain *form, pgObject *obj)
{
    frmOptions *frm=new frmOptions(form);
    frm->Show();
    return 0;
}

// Enable/disable the copy quote option as required.
void frmOptions::OnChangeCopyQuote(wxCommandEvent& WXUNUSED(ev))
{
	if (cbCopyQuote->GetValue() == _("None"))
		cbCopyQuoteChar->Disable();
	else
		cbCopyQuoteChar->Enable();
}
