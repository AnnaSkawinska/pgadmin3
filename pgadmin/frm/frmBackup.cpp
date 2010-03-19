//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2010, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// frmBackup.cpp - Backup database dialogue
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>
#include <wx/settings.h>


// App headers
#include "pgAdmin3.h"
#include "frm/frmMain.h"
#include "frm/frmBackup.h"
#include "utils/sysLogger.h"
#include "schema/pgSchema.h"
#include "schema/pgTable.h"

// Icons
#include "images/backup.xpm"


#define nbNotebook              CTRL_NOTEBOOK("nbNotebook")
#define txtFilename             CTRL_TEXT("txtFilename")
#define btnFilename             CTRL_BUTTON("btnFilename")
#define rbxFormat               CTRL_RADIOBOX("rbxFormat")
#define chkBlobs                CTRL_CHECKBOX("chkBlobs")
#define chkOid                  CTRL_CHECKBOX("chkOid")
#define chkInsert               CTRL_CHECKBOX("chkInsert")
#define chkDisableDollar        CTRL_CHECKBOX("chkDisableDollar")
#define sbxPlainOptions         CTRL_STATICBOX("sbxPlainOptions")
#define chkOnlyData             CTRL_CHECKBOX("chkOnlyData")
#define chkOnlySchema           CTRL_CHECKBOX("chkOnlySchema")
#define chkNoOwner              CTRL_CHECKBOX("chkNoOwner")
#define chkCreateDb             CTRL_CHECKBOX("chkCreateDb")
#define chkDropDb               CTRL_CHECKBOX("chkDropDb")
#define chkDisableTrigger       CTRL_CHECKBOX("chkDisableTrigger")
#define chkVerbose              CTRL_CHECKBOX("chkVerbose")


BEGIN_EVENT_TABLE(frmBackup, ExternProcessDialog)
    EVT_TEXT(XRCID("txtFilename"),          frmBackup::OnChange)
    EVT_BUTTON(XRCID("btnFilename"),        frmBackup::OnSelectFilename)
    EVT_BUTTON(wxID_OK,                     frmBackup::OnOK)
    EVT_RADIOBOX(XRCID("rbxFormat"),        frmBackup::OnChangePlain)
    EVT_CHECKBOX(XRCID("chkOnlyData"),      frmBackup::OnChangePlain)
    EVT_CHECKBOX(XRCID("chkOnlySchema"),    frmBackup::OnChangePlain)
    EVT_CHECKBOX(XRCID("chkNoOwner"),       frmBackup::OnChangePlain)
    EVT_CLOSE(                              ExternProcessDialog::OnClose)
END_EVENT_TABLE()



frmBackup::frmBackup(frmMain *form, pgObject *obj) : ExternProcessDialog(form)
{
    object=obj;

    wxWindowBase::SetFont(settings->GetSystemFont());
    LoadResource(form, wxT("frmBackup"));
    RestorePosition();

    SetTitle(wxString::Format(_("Backup %s %s"), object->GetTranslatedTypeName().c_str(), object->GetFullIdentifier().c_str()));

    canBlob = (obj->GetMetaType() == PGM_DATABASE);
    chkBlobs->SetValue(canBlob);
    chkDisableDollar->Enable(obj->GetConnection()->BackendMinimumVersion(7, 5));

    wxString val;
    settings->Read(wxT("frmBackup/LastFile"), &val, wxEmptyString);
    txtFilename->SetValue(val);

    if (!object->GetDatabase()->GetServer()->GetPasswordIsStored())
        environment.Add(wxT("PGPASSWORD=") + object->GetServer()->GetPassword());

    // Pass the SSL mode via the environment
    environment.Add(wxT("PGSSLMODE=") + object->GetServer()->GetConnection()->GetSslModeName());

    // Icon
    SetIcon(wxIcon(backup_xpm));

    // fix translation problem
    wxString dollarLabel=wxGetTranslation(_("Disable $$ quoting"));
    dollarLabel.Replace(wxT("$$"), wxT("$"));
    chkDisableDollar->SetLabel(dollarLabel);
    chkDisableDollar->SetSize(chkDisableDollar->GetBestSize());

    txtMessages = CTRL_TEXT("txtMessages");
    txtMessages->SetMaxLength(0L);
    btnOK->Disable();

    wxCommandEvent ev;
    OnChangePlain(ev);
}


frmBackup::~frmBackup()
{
    SavePosition();
}


wxString frmBackup::GetHelpPage() const
{
    wxString page;
    page = wxT("pg/app-pgdump");
    return page;
}


void frmBackup::OnSelectFilename(wxCommandEvent &ev)
{
    wxString title, prompt, FilenameOnly;

    if (rbxFormat->GetSelection() == 2) // plain
    {
        title  = _("Select output file");
        prompt = _("Query files (*.sql)|*.sql|All files (*.*)|*.*");
    }
    else
    {
        title  = _("Select backup filename");
        prompt = _("Backup files (*.backup)|*.backup|All files (*.*)|*.*");
    }

    wxFileName::SplitPath(txtFilename->GetValue(), NULL, NULL, &FilenameOnly, NULL);
    wxFileDialog file(this, title, ::wxPathOnly(txtFilename->GetValue()), FilenameOnly, prompt, wxFD_SAVE);

    if (file.ShowModal() == wxID_OK)
    {
        txtFilename->SetValue(file.GetPath());
        OnChange(ev);
    }
}


void frmBackup::OnChange(wxCommandEvent &ev)
{
    if (!process && !done)
        btnOK->Enable(!txtFilename->GetValue().IsEmpty());
}


void frmBackup::OnChangePlain(wxCommandEvent &ev)
{
    bool isPlain = (rbxFormat->GetSelection() == 2);
    sbxPlainOptions->Enable(isPlain);
    chkBlobs->Enable(canBlob && !isPlain);
    chkOnlyData->Enable(isPlain && !chkOnlySchema->GetValue());
    if (isPlain)
        isPlain = !chkOnlyData->GetValue();

    chkOnlySchema->Enable(isPlain);
    chkNoOwner->Enable(isPlain);
    chkDropDb->Enable(isPlain);
    chkCreateDb->Enable(isPlain);
    chkDisableTrigger->Enable(chkOnlyData->GetValue());

    wxCommandEvent nullEvent;
    OnChange(nullEvent);
}


wxString frmBackup::GetCmd(int step)
{
    wxString cmd = getCmdPart1();

    return cmd + getCmdPart2();
}


wxString frmBackup::GetDisplayCmd(int step)
{
    wxString cmd = getCmdPart1();

    return cmd + getCmdPart2();
}


wxString frmBackup::getCmdPart1()
{
    pgServer *server=object->GetDatabase()->GetServer();

    wxString cmd;
    if (object->GetConnection()->EdbMinimumVersion(8,0))
        cmd=edbBackupExecutable;
    else if (object->GetConnection()->GetIsGreenplum())
        cmd=gpBackupExecutable;
    else
        cmd=pgBackupExecutable;

    if (!server->GetName().IsEmpty())
        cmd += wxT(" --host ") + server->GetName();

    cmd +=  wxT(" --port ") + NumToStr((long)server->GetPort())
         +  wxT(" --username ") + commandLineCleanOption(qtIdent(server->GetUsername()));

    if (object->GetConnection()->GetIsGreenplum())
        cmd += wxT(" --gp-syntax ");
    return cmd;
}


wxString frmBackup::getCmdPart2()
{
    wxString backupExecutable;
    if (object->GetConnection()->EdbMinimumVersion(8,0))
        backupExecutable=edbBackupExecutable;
    else if (object->GetConnection()->GetIsGreenplum())
        backupExecutable=gpBackupExecutable;
    else
        backupExecutable=pgBackupExecutable;

    wxString cmd;

    switch (rbxFormat->GetSelection())
    {
        case 0: // compressed
        {
            cmd.Append(wxT(" --format custom"));
            if (chkBlobs->GetValue())
                cmd.Append(wxT(" --blobs"));
            break;
        }
        case 1: // tar
        {
            cmd.Append(wxT(" --format tar"));
            if (chkBlobs->GetValue())
                cmd.Append(wxT(" --blobs"));
            break;
        }
        case 2:
        {
            cmd.Append(wxT(" --format plain"));
            if (chkOnlyData->GetValue())
            {
                cmd.Append(wxT(" --data-only"));
                if (chkDisableTrigger->GetValue())
                    cmd.Append(wxT(" --disable-triggers"));
            }
            else
            {
                if (chkOnlySchema->GetValue())
                    cmd.Append(wxT(" --schema-only"));
                if (chkNoOwner->GetValue())
                    cmd.Append(wxT(" --no-owner"));
                if (chkCreateDb->GetValue())
                    cmd.Append(wxT(" --create"));
                if (chkDropDb->GetValue())
                    cmd.Append(wxT(" --clean"));
            }
            break;
        }
    }

    if (chkOid->GetValue())
        cmd.Append(wxT(" --oids"));
    if (chkInsert->GetValue())
        cmd.Append(wxT(" --column-inserts"));
    if (chkDisableDollar->GetValue())
        cmd.Append(wxT(" --disable-dollar-quoting"));
    if (settings->GetIgnoreVersion())
        cmd.Append(wxT(" --ignore-version"));
    if (chkVerbose->GetValue())
        cmd.Append(wxT(" --verbose"));

    cmd.Append(wxT(" --file \"") + txtFilename->GetValue() + wxT("\""));

    if (object->GetMetaType() == PGM_SCHEMA)
        cmd.Append(wxT(" --schema ") + commandLineCleanOption(((pgSchema*)object)->GetQuotedIdentifier(), true));

    else if (object->GetMetaType() == PGM_TABLE || object->GetMetaType() == GP_PARTITION) 
    {
        // The syntax changed in 8.2 :-(
        if (pgAppMinimumVersion(backupExecutable, 8, 2))
        {
            cmd.Append(wxT(" --table ") +
                  commandLineCleanOption(((pgTable*)object)->GetSchema()->GetQuotedIdentifier(), true) + 
                  wxT(".") +
                  commandLineCleanOption(((pgTable*)object)->GetQuotedIdentifier(), true));
        }
        else
        {
            cmd.Append(wxT(" --table ") + commandLineCleanOption(((pgTable*)object)->GetQuotedIdentifier(), true));
            cmd.Append(wxT(" --schema ") + commandLineCleanOption(((pgTable*)object)->GetSchema()->GetQuotedIdentifier(), true));
        }
    }

    cmd.Append(wxT(" ") + commandLineCleanOption(object->GetDatabase()->GetQuotedIdentifier()));

    return cmd;
}


void frmBackup::Go()
{
    txtFilename->SetFocus();
    Show(true);
}

void frmBackup::OnOK(wxCommandEvent &ev)
{
    if (!done)
    {
        if (processedFile == txtFilename->GetValue())
        {
            if (wxMessageBox(_("Are you sure you wish to run a backup to this file again?"), _("Repeat backup?"), wxICON_QUESTION | wxYES_NO) == wxNO)
                return;
        }
        else if (wxFile::Exists(txtFilename->GetValue()))
        {
            wxString msg;
            msg.Printf(_("The file: \n\n%s\n\nalready exists. Do you want to overwrite it?"), txtFilename->GetValue().c_str());
            if (wxMessageBox(msg, _("Overwrite file?"), wxICON_WARNING | wxYES_NO) == wxNO)
                return;
        }
        processedFile = txtFilename->GetValue();
    }

    settings->Write(wxT("frmBackup/LastFile"), txtFilename->GetValue());
    ExternProcessDialog::OnOK(ev);
}

backupFactory::backupFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : contextActionFactory(list)
{
    mnu->Append(id, _("&Backup..."), _("Creates a backup of the current database to a local file"));
}


wxWindow *backupFactory::StartDialog(frmMain *form, pgObject *obj)
{
    frmBackup *frm=new frmBackup(form, obj);
    frm->Go();
    return 0;
}


bool backupFactory::CheckEnable(pgObject *obj)
{
    if (!obj)
        return false;

    if (obj->GetConnection() && obj->GetConnection()->EdbMinimumVersion(8, 0))
        return obj->CanBackup() && !edbBackupExecutable.IsEmpty();
    else if (obj->GetConnection() && obj->GetConnection()->GetIsGreenplum())
        return obj->CanBackup() && !gpBackupExecutable.IsEmpty();
    else
        return obj->CanBackup() && !pgBackupExecutable.IsEmpty();

}


