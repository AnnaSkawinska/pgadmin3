//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2010, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// frmRestore.cpp - Restore database dialogue
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>
#include <wx/settings.h>


// App headers
#include "pgAdmin3.h"
#include "frm/frmRestore.h"
#include "frm/frmMain.h"
#include "utils/sysLogger.h"
#include "schema/pgTable.h"
#include <wx/process.h>
#include <wx/textbuf.h>
#include <wx/file.h>
#include "schema/pgLanguage.h"
#include "schema/pgConstraints.h"
#include "schema/pgForeignKey.h"

// Icons
#include "images/restore.xpm"


#define nbNotebook              CTRL_NOTEBOOK("nbNotebook")
#define txtFilename             CTRL_TEXT("txtFilename")
#define btnFilename             CTRL_BUTTON("btnFilename")
#define chkOnlyData             CTRL_CHECKBOX("chkOnlyData")
#define chkOnlySchema           CTRL_CHECKBOX("chkOnlySchema")
#define chkSingleObject         CTRL_CHECKBOX("chkSingleObject")
#define chkNoOwner              CTRL_CHECKBOX("chkNoOwner")
#define chkDisableTrigger       CTRL_CHECKBOX("chkDisableTrigger")
#define chkVerbose              CTRL_CHECKBOX("chkVerbose")
#define stSingleObject          CTRL_STATIC("stSingleObject")

#define lstContents             CTRL_LISTVIEW("lstContents")
#define btnView                 CTRL_BUTTON("btnView")


BEGIN_EVENT_TABLE(frmRestore, ExternProcessDialog)
    EVT_TEXT(XRCID("txtFilename"),          frmRestore::OnChangeName)
    EVT_CHECKBOX(XRCID("chkOnlyData"),      frmRestore::OnChangeData)
    EVT_CHECKBOX(XRCID("chkOnlySchema"),    frmRestore::OnChangeSchema)
    EVT_CHECKBOX(XRCID("chkSingleObject"),  frmRestore::OnChange)
    EVT_BUTTON(XRCID("btnFilename"),        frmRestore::OnSelectFilename)
    EVT_BUTTON(wxID_OK,                     frmRestore::OnOK)
    EVT_BUTTON(XRCID("btnView"),            frmRestore::OnView)
    EVT_END_PROCESS(-1,                     frmRestore::OnEndProcess)
    EVT_LIST_ITEM_SELECTED(XRCID("lstContents"), frmRestore::OnChangeList)
    EVT_CLOSE(                              ExternProcessDialog::OnClose)
END_EVENT_TABLE()



frmRestore::frmRestore(frmMain *_form, pgObject *obj) : ExternProcessDialog(form)
{
    object=obj;

    if (object->GetMetaType() == PGM_SERVER)
        server = (pgServer*)object;
    else
        server=object->GetDatabase()->GetServer();

    form=_form;

    wxWindowBase::SetFont(settings->GetSystemFont());
    LoadResource(_form, wxT("frmRestore"));
    RestorePosition();

    SetTitle(wxString::Format(_("Restore %s %s"), object->GetTranslatedTypeName().c_str(), object->GetFullIdentifier().c_str()));


    if (object->GetMetaType() != PGM_DATABASE)
    {
        if (object->GetMetaType() == PGM_TABLE)
        {
            chkOnlySchema->SetValue(false);
            chkOnlyData->SetValue(true);
        }
        else
        {
            chkOnlySchema->SetValue(true);
            chkOnlyData->SetValue(false);
        }
        chkSingleObject->SetValue(true);
        chkOnlyData->Disable();
        chkOnlySchema->Disable();
        chkSingleObject->Disable();
    }

    wxString val;
    settings->Read(wxT("frmRestore/LastFile"), &val, wxEmptyString);
    txtFilename->SetValue(val);

    // Icon
    SetIcon(wxIcon(restore_xpm));

    txtMessages = CTRL_TEXT("txtMessages");
    txtMessages->SetMaxLength(0L);
    btnOK->Disable();
    filenameValid=false;
    if (!server->GetPasswordIsStored())
        environment.Add(wxT("PGPASSWORD=") + server->GetPassword());

    wxCommandEvent ev;
    OnChangeName(ev);
}


frmRestore::~frmRestore()
{
    SavePosition();
}


wxString frmRestore::GetHelpPage() const
{
    wxString page;
    page = wxT("pg/app-pgrestore");
    return page;
}


void frmRestore::OnSelectFilename(wxCommandEvent &ev)
{
    
    wxString FilenameOnly;    
    wxFileName::SplitPath(txtFilename->GetValue(), NULL, NULL, &FilenameOnly, NULL);
    
    wxFileDialog file(this, _("Select backup filename"), ::wxPathOnly(txtFilename->GetValue()), FilenameOnly, 
        _("Backup files (*.backup)|*.backup|All files (*.*)|*.*"));

    if (file.ShowModal() == wxID_OK)
    {
        txtFilename->SetValue(file.GetPath());
        OnChange(ev);
    }
}


void frmRestore::OnChangeData(wxCommandEvent &ev)
{
    if (chkOnlyData->GetValue())
    {
        chkOnlySchema->SetValue(false);
        chkOnlySchema->Disable();
    }
    else
        chkOnlySchema->Enable();

    OnChange(ev);
}


void frmRestore::OnChangeSchema(wxCommandEvent &ev)
{
    if (chkOnlySchema->GetValue())
    {
        chkOnlyData->SetValue(false);
        chkOnlyData->Disable();
    }
    else
        chkOnlyData->Enable();

    OnChange(ev);
}


void frmRestore::OnChangeName(wxCommandEvent &ev)
{
    wxString name=txtFilename->GetValue();
    if (name.IsEmpty() || !wxFile::Exists(name))
        filenameValid=false;
    else
    {
        wxFile file(name, wxFile::read);
        if (file.IsOpened())
        {
            char buffer[8];
            off_t size=file.Read(buffer, 8);
            if (size == 8)
            {
                if (memcmp(buffer, "PGDMP", 5) && !memcmp(buffer, "toc.dat", 8))
                {
                    // tar format?
                    file.Seek(512);
                    size=file.Read(buffer, 8);
                }
                if (size == 8 && !memcmp(buffer, "PGDMP", 5))
                {
                    // check version here?
                    filenameValid=true;
                }
            }
        }
    }
    OnChange(ev);
}


void frmRestore::OnChangeList(wxListEvent &ev)
{
    OnChange(ev);
}


void frmRestore::OnChange(wxCommandEvent &ev)
{
    bool singleValid = !chkSingleObject->GetValue();
    if (!singleValid)
    {
        switch(object->GetMetaType())
        {
            case PGM_DATABASE:
            {
                int sel=lstContents->GetSelection();
                if (sel >= 0)
                {
                    wxString type=lstContents->GetText(sel, 0);
                    if ((type == _("Function") && !chkOnlyData->GetValue()) || 
                        (type == _("Table") && !chkOnlySchema->GetValue()))
                    {
                        singleValid = true;
                        stSingleObject->SetLabel(type + wxT(" ") + lstContents->GetText(sel, 1));
                    }
                }
                break;
            }
            case PGM_TABLE:
            case PGM_FUNCTION:
            {
                singleValid=true;
                stSingleObject->SetLabel(object->GetTranslatedTypeName() + wxT(" ") + object->GetName());

                break;
            }
            default:
                break;
        }
    }
    btnOK->Enable(filenameValid && singleValid);
    btnView->Enable(filenameValid);
}


wxString frmRestore::GetCmd(int step)
{
    wxString cmd = getCmdPart1();

    return cmd + getCmdPart2(step);
}


wxString frmRestore::GetDisplayCmd(int step)
{
    wxString cmd = getCmdPart1();

    return cmd + getCmdPart2(step);
}


wxString frmRestore::getCmdPart1()
{
    extern wxString pgRestoreExecutable;
    extern wxString edbRestoreExecutable;

    wxString cmd;

    if (object->GetConnection()->EdbMinimumVersion(8,0))
        cmd=edbRestoreExecutable;
    else
        cmd=pgRestoreExecutable;

    if (!server->GetName().IsEmpty())
        cmd += wxT(" -h ") + server->GetName();

    cmd += wxT(" -p ") + NumToStr((long)server->GetPort())
         + wxT(" -U ") + qtIdent(server->GetUsername())
         + wxT(" -d ") + commandLineCleanOption(object->GetDatabase()->GetQuotedIdentifier());
    return cmd;
}


wxString frmRestore::getCmdPart2(int step)
{
    wxString cmd;
    // if (server->GetSSL())
    // pg_dump doesn't support ssl

    if (step)
    {
        cmd.Append(wxT(" -l"));
    }
    else
    {
        if (chkOnlyData->GetValue())
        {
            cmd.Append(wxT(" -a"));
        }
        else
        {
            if (chkNoOwner->GetValue())
                cmd.Append(wxT(" -O"));
        }

        if (chkOnlySchema->GetValue())
        {
            cmd.Append(wxT(" -s"));
        }
        else
        {
            if (chkDisableTrigger->GetValue())
            cmd.Append(wxT(" --disable-triggers"));
        }

        if (chkSingleObject->GetValue())
        {
            switch (object->GetMetaType())
            {
                case PGM_DATABASE:
                {
                    int sel=lstContents->GetSelection();
                    if (lstContents->GetText(sel, 0) == _("Function"))
                        cmd.Append(wxT(" -P ") + qtIdent(lstContents->GetText(sel, 1).BeforeLast('(')));
                    else if (lstContents->GetText(sel, 0) == _("Table"))
                        cmd.Append(wxT(" -t ") + qtIdent(lstContents->GetText(sel, 1)));
                    else
                        return wxT("restore: internal pgadmin error.");   // shouldn't happen!

                    break;
                }
                case PGM_TABLE:
                    cmd.Append(wxT(" -t ") + object->GetQuotedIdentifier());
                    break;
                case PGM_FUNCTION:
                    cmd.Append(wxT(" -P ") + object->GetQuotedIdentifier());
                    break;
                default:
                    break;
            }
        }
        if (chkVerbose->GetValue())
            cmd.Append(wxT(" -v"));
    }


    cmd.Append(wxT(" \"") + txtFilename->GetValue() + wxT("\""));

    return cmd;
}


void frmRestore::OnView(wxCommandEvent &ev)
{
    btnView->Disable();
    btnOK->Disable();
    viewRunning = true;
    lstContents->DeleteAllItems();
    Execute(1, false);
    btnOK->SetLabel(_("OK"));
    done=0;
}


void frmRestore::OnOK(wxCommandEvent &ev)
{
    if (!done)
    {
        if (processedFile == txtFilename->GetValue())
        {
            if (wxMessageBox(_("Are you sure you wish to run a restore from this file again?"), _("Repeat restore?"), wxICON_QUESTION | wxYES_NO) == wxNO)
                return;
        }

        processedFile = txtFilename->GetValue();
    }

    settings->Write(wxT("frmRestore/LastFile"), txtFilename->GetValue());
    viewRunning = false;
    btnView->Disable();

    ExternProcessDialog::OnOK(ev);

}


void frmRestore::OnEndProcess(wxProcessEvent& ev)
{
    ExternProcessDialog::OnEndProcess(ev);

    if (done && viewRunning && !ev.GetExitCode())
    {
        done = false;

        lstContents->CreateColumns(0, _("Type"), _("Name"));

        wxString str=wxTextBuffer::Translate(txtMessages->GetValue(), wxTextFileType_Unix);

        wxStringTokenizer line(str, wxT("\n"));
        line.GetNextToken();
        
        lstContents->Freeze();
        wxBeginBusyCursor();

        while (line.HasMoreTokens())
        {
            str=line.GetNextToken();
            if (str.Left(2) == wxT(";"))
                continue;

            wxStringTokenizer col(str, wxT(" "));
            col.GetNextToken();
            if (!StrToLong(col.GetNextToken().c_str()))
                continue;
            col.GetNextToken();
            wxString type=col.GetNextToken();

            int icon = -1;
            wxString typname;
            pgaFactory *factory=0;

            if (type == wxT("PROCEDURAL"))
            {
                factory=&languageFactory;
                type = col.GetNextToken();
            }
            else if (type == wxT("FK"))
            {
                factory=&foreignKeyFactory;
                type = col.GetNextToken();
            }
            else if (type == wxT("CONSTRAINT"))
            {
                factory=constraintFactory.GetCollectionFactory();
                // ??? type = col.GetNextToken();
            }
            else
                factory = pgaFactory::GetFactory(type);

            wxString name = str.Mid(str.Find(type)+type.Length()+1).BeforeLast(' ');
            if (factory)
            {
                typname=factory->GetTypeName();
                icon = factory->GetIconId();
            }
            else if (typname.IsEmpty())
                typname = type;

            lstContents->AppendItem(icon, typname, name);
        }

        lstContents->Thaw();
        wxEndBusyCursor();
        nbNotebook->SetSelection(1);
    }
}


void frmRestore::Go()
{
    txtFilename->SetFocus();
    Show(true);
}



restoreFactory::restoreFactory(menuFactoryList *list, wxMenu *mnu, wxToolBar *toolbar) : contextActionFactory(list)
{
    mnu->Append(id, _("&Restore..."), _("Restores a backup from a local file"));
}


wxWindow *restoreFactory::StartDialog(frmMain *form, pgObject *obj)
{
    frmRestore *frm=new frmRestore(form, obj);
    frm->Go();
    return 0;
}


bool restoreFactory::CheckEnable(pgObject *obj)
{
    extern wxString pgRestoreExecutable;
    extern wxString edbRestoreExecutable;

    if (!obj)
        return false;

    if (obj->GetConnection() && obj->GetConnection()->EdbMinimumVersion(8, 0))
        return obj->CanCreate() && obj->CanRestore() && !edbRestoreExecutable.IsEmpty();
    else
        return obj->CanCreate() && obj->CanRestore() && !pgRestoreExecutable.IsEmpty();
}
