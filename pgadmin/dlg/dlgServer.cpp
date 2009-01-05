//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2009, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// dlgServer.cpp - PostgreSQL Database Property
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "frm/frmMain.h"
#include "frm/frmHint.h"
#include "dlg/dlgServer.h"
#include "schema/pgDatabase.h"

// pointer to controls
#define txtDescription  CTRL_TEXT("txtDescription")
#define txtService      CTRL_TEXT("txtService")
#define cbDatabase      CTRL_COMBOBOX("cbDatabase")
#define txtPort         CTRL_TEXT("txtPort")
#define cbSSL           CTRL_COMBOBOX("cbSSL")
#define txtUsername     CTRL_TEXT("txtUsername")
#define stTryConnect    CTRL_STATIC("stTryConnect")
#define chkTryConnect   CTRL_CHECKBOX("chkTryConnect")
#define stStorePwd      CTRL_STATIC("stStorePwd")
#define chkStorePwd     CTRL_CHECKBOX("chkStorePwd")
#define stRestore       CTRL_STATIC("stRestore")
#define chkRestore      CTRL_CHECKBOX("chkRestore")
#define stPassword      CTRL_STATIC("stPassword")
#define txtPassword     CTRL_TEXT("txtPassword")
#define txtDbRestriction CTRL_TEXT("txtDbRestriction")



BEGIN_EVENT_TABLE(dlgServer, dlgProperty)
    EVT_NOTEBOOK_PAGE_CHANGED(XRCID("nbNotebook"),  dlgServer::OnPageSelect)  
    EVT_TEXT(XRCID("txtDescription"),               dlgProperty::OnChange)
    EVT_TEXT(XRCID("txtService"),                   dlgProperty::OnChange)
    EVT_TEXT(XRCID("cbDatabase"),                   dlgProperty::OnChange)
    EVT_COMBOBOX(XRCID("cbDatabase"),               dlgProperty::OnChange)
    EVT_TEXT(XRCID("txtPort")  ,                    dlgProperty::OnChange)
    EVT_TEXT(XRCID("txtUsername"),                  dlgProperty::OnChange)
    EVT_TEXT(XRCID("txtDbRestriction"),             dlgServer::OnChangeRestr)
    EVT_COMBOBOX(XRCID("cbSSL"),                    dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkStorePwd"),              dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkRestore"),               dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkTryConnect"),            dlgServer::OnChangeTryConnect)
    EVT_BUTTON(wxID_OK,                             dlgServer::OnOK)
END_EVENT_TABLE();


dlgProperty *pgServerFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
{
    return new dlgServer(this, frame, (pgServer*)node);
}


dlgServer::dlgServer(pgaFactory *f, frmMain *frame, pgServer *node)
: dlgProperty(f, frame, wxT("dlgServer"))
{
    server=node;
    dbRestrictionOk=true;

    cbDatabase->Append(wxT("postgres"));
    cbDatabase->Append(wxT("edb"));
    cbDatabase->Append(wxT("template1"));
    wxString lastDB = settings->GetLastDatabase();
    if (lastDB != wxT("postgres")&& lastDB != wxT("edb") && lastDB != wxT("template1"))
        cbDatabase->Append(lastDB);
    cbDatabase->SetSelection(0);

    txtPort->SetValue(NumToStr((long)settings->GetLastPort()));    
    if (!cbSSL->IsEmpty())
        cbSSL->SetSelection(settings->GetLastSSL());
    txtUsername->SetValue(settings->GetLastUsername());
 
    chkTryConnect->SetValue(true);
    chkStorePwd->SetValue(true);
    chkRestore->SetValue(true);
    if (node)
    {
        chkTryConnect->SetValue(false);
        chkTryConnect->Disable();
    }
}


dlgServer::~dlgServer()
{
    if (!server)
    {
        settings->SetLastDatabase(cbDatabase->GetValue());
        settings->SetLastPort(StrToLong(txtPort->GetValue()));
        settings->SetLastSSL(cbSSL->GetCurrentSelection());
        settings->SetLastUsername(txtUsername->GetValue());
    }
}


pgObject *dlgServer::GetObject()
{
    return server;
}


void dlgServer::OnOK(wxCommandEvent &ev)
{
    // Display the 'save password' hint if required
    if(chkStorePwd->GetValue())
    {
        if (frmHint::ShowHint(this, HINT_SAVING_PASSWORDS) == wxID_CANCEL)
                return;
    }

    // notice: changes active after reconnect

    EnableOK(false);


    if (server)
    {
        server->iSetName(GetName());
        server->iSetDescription(txtDescription->GetValue());
        if (txtService->GetValue() != server->GetServiceID())
        {
            mainForm->StartMsg(_("Checking server status"));
            server->iSetServiceID(txtService->GetValue());
            mainForm->EndMsg();
        }
        server->iSetPort(StrToLong(txtPort->GetValue()));
        server->iSetSSL(cbSSL->GetCurrentSelection());
        server->iSetDatabase(cbDatabase->GetValue());
        server->iSetUsername(txtUsername->GetValue());
        server->iSetStorePwd(chkStorePwd->GetValue());
        server->iSetRestore(chkRestore->GetValue());
        server->iSetDbRestriction(txtDbRestriction->GetValue().Trim());
        mainForm->execSelChange(server->GetId(), true);
        mainForm->GetBrowser()->SetItemText(item, server->GetFullName());

        wxMessageBox(_("Note: changes to server settings will take effect the next time the server is connected."), _("Server settings"), wxICON_INFORMATION);
    }

    if (IsModal())
    {
        EndModal(wxID_OK);
        return;
    }
    else
        Destroy();
}


void dlgServer::OnChangeRestr(wxCommandEvent &ev)
{
    if (!connection || txtDbRestriction->GetValue().IsEmpty())
        dbRestrictionOk = true;
    else
    {
        wxString sql=wxT("EXPLAIN SELECT 1 FROM pg_database DB\n");
        if (connection->BackendMinimumVersion(8, 0))
            sql += wxT(" JOIN pg_tablespace ta ON db.dattablespace=ta.OID\n");
        sql += wxT(" WHERE (") + txtDbRestriction->GetValue() + wxT(")");


        wxLogNull nix;
        wxString result=connection->ExecuteScalar(sql);

        dbRestrictionOk = !result.IsEmpty();
    }
    dlgProperty::OnChange(ev);
}


void dlgServer::OnPageSelect(wxNotebookEvent &event)
{
    // to prevent dlgProperty from catching it
}


wxString dlgServer::GetHelpPage() const
{
    return wxT("connect");
}


int dlgServer::GoNew()
{
    if (cbSSL->IsEmpty())
        return Go(true);
    else
    {
        CheckChange();
        return ShowModal();
    }
}


int dlgServer::Go(bool modal)
{
    cbSSL->Append(wxT(" "));

#ifdef SSL
    cbSSL->Append(_("require"));
    cbSSL->Append(_("prefer"));

    if (pgConn::GetLibpqVersion() > 7.3)
    {
        cbSSL->Append(_("allow"));
        cbSSL->Append(_("disable"));
    }
#endif

    if (server)
    {
        if (cbDatabase->FindString(server->GetDatabaseName()) < 0)
            cbDatabase->Append(server->GetDatabaseName());
        txtDescription->SetValue(server->GetDescription());
        txtService->SetValue(server->GetServiceID());
        txtPort->SetValue(NumToStr((long)server->GetPort()));
        cbSSL->SetSelection(server->GetSSL());
        cbDatabase->SetValue(server->GetDatabaseName());
        txtUsername->SetValue(server->GetUsername());
        chkStorePwd->SetValue(server->GetStorePwd());
        chkRestore->SetValue(server->GetRestore());
        txtDbRestriction->SetValue(server->GetDbRestriction());

        stPassword->Disable();
        txtPassword->Disable();
        if (connection)
        {
            txtName->Disable();
            cbDatabase->Disable();
            txtPort->Disable();
            cbSSL->Disable();
            txtUsername->Disable();
            chkStorePwd->Disable();
        }
    }
    else
    {
        SetTitle(_("Add server"));
    }

    int rc=dlgProperty::Go(modal);

    return rc;
}


bool dlgServer::GetTryConnect()
{
    return chkTryConnect->GetValue();
}


wxString dlgServer::GetPassword()
{
    return txtPassword->GetValue();
}


pgObject *dlgServer::CreateObject(pgCollection *collection)
{
    wxString name=GetName();

    pgObject *obj=new pgServer(GetName(), txtDescription->GetValue(), cbDatabase->GetValue(), 
        txtUsername->GetValue(), StrToLong(txtPort->GetValue()), chkTryConnect->GetValue() && chkStorePwd->GetValue(), chkRestore->GetValue(), cbSSL->GetCurrentSelection());

    return obj;
}


void dlgServer::OnChangeTryConnect(wxCommandEvent &ev)
{
    chkStorePwd->Enable(chkTryConnect->GetValue());
    txtPassword->Enable(chkTryConnect->GetValue());
    OnChange(ev);
}


void dlgServer::CheckChange()
{
    wxString name=GetName();
    bool enable=true;

    if (server)
    {
        enable =  name != server->GetName()
               || txtDescription->GetValue() != server->GetDescription()
               || txtService->GetValue() != server->GetServiceID()
               || StrToLong(txtPort->GetValue()) != server->GetPort()
               || cbDatabase->GetValue() != server->GetDatabaseName()
               || txtUsername->GetValue() != server->GetUsername()
               || cbSSL->GetCurrentSelection() != server->GetSSL()
               || chkStorePwd->GetValue() != server->GetStorePwd()
               || chkRestore->GetValue() != server->GetRestore()
               || txtDbRestriction->GetValue() != server->GetDbRestriction();
    }


#ifdef __WXMSW__
    CheckValid(enable, !name.IsEmpty(), _("Please specify address."));
#else
    bool isPipe = (name.IsEmpty() || name.StartsWith(wxT("/")));
    cbSSL->Enable(!isPipe);
#endif
    CheckValid(enable, !txtDescription->GetValue().IsEmpty(), _("Please specify description."));
    CheckValid(enable, StrToLong(txtPort->GetValue()) > 0, _("Please specify port."));
    CheckValid(enable, !txtUsername->GetValue().IsEmpty(), _("Please specify user name"));
    CheckValid(enable, dbRestrictionOk, _("Restriction not valid."));

    EnableOK(enable);
}


wxString dlgServer::GetSql()
{
    return wxEmptyString;
}
