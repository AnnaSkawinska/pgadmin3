//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// Copyright (C) 2002 - 2003, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// dlgDatabase.cpp - PostgreSQL Database Property
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "misc.h"
#include "dlgDatabase.h"
#include "pgDatabase.h"

// Images
#include "images/database.xpm"


// pointer to controls
#define cbOwner         CTRL("cbOwner", wxComboBox)
#define cbEncoding      CTRL("cbEncoding", wxComboBox)
#define cbTemplate      CTRL("cbTemplate", wxComboBox)
#define txtPath         CTRL("txtPath", wxTextCtrl)

#define lstVariables    CTRL("lstVariables", wxListCtrl)
#define cbVarname       CTRL("cbVarname", wxComboBox)
#define txtValue        CTRL("txtValue", wxTextCtrl)
#define chkValue        CTRL("chkValue", wxCheckBox)
#define btnAdd          CTRL("btnAdd", wxButton)
#define btnRemove       CTRL("btnRemove", wxButton)



BEGIN_EVENT_TABLE(dlgDatabase, dlgSecurityProperty)
    EVT_TEXT(XRCID("txtName"),                      dlgDatabase::OnChange)
    EVT_TEXT(XRCID("txtComment"),                   dlgDatabase::OnChange)

    EVT_LIST_ITEM_SELECTED(XRCID("lstVariables"),   dlgDatabase::OnVarSelChange)
    EVT_BUTTON(XRCID("btnAdd"),                     dlgDatabase::OnVarAdd)
    EVT_BUTTON(XRCID("btnRemove"),                  dlgDatabase::OnVarRemove)
    EVT_TEXT(XRCID("cbVarname"),                    dlgDatabase::OnVarnameSelChange)
END_EVENT_TABLE();


dlgDatabase::dlgDatabase(frmMain *frame, pgDatabase *node)
: dlgSecurityProperty(frame, node, wxT("dlgDatabase"), wxT("CREATE,TEMP"), "CT")
{
    SetIcon(wxIcon(database_xpm));
    database=node;
    CreateListColumns(lstVariables, _("Variable"), _("Value"));

    txtOID->Disable();
    chkValue->Hide();
}


pgObject *dlgDatabase::GetObject()
{
    return database;
}


int dlgDatabase::Go(bool modal)
{
    if (!database)
        cbOwner->Append(wxT(""));

    AddGroups();
    AddUsers(cbOwner);

    if (database)
    {
        // edit mode

        readOnly = !database->GetServer()->GetCreatePrivilege();
        wxStringTokenizer cfgTokens(database->GetVariables(), wxT(","));
        while (cfgTokens.HasMoreTokens())
        {
            wxString token=cfgTokens.GetNextToken();
            AppendListItem(lstVariables, token.BeforeFirst('='), token.AfterFirst('='), 0);
        }

        txtName->SetValue(database->GetName());
        txtOID->SetValue(NumToStr((long)database->GetOid()));
        txtPath->SetValue(database->GetPath());
        cbEncoding->Append(database->GetEncoding());
        cbEncoding->SetSelection(0);
        txtComment->SetValue(database->GetComment());
        cbOwner->SetValue(database->GetOwner());

        txtName->Disable();
        cbOwner->Disable();
        txtPath->Disable();
        cbTemplate->Disable();
        cbEncoding->Disable();

        if (readOnly)
        {
            cbVarname->Disable();
            txtValue->Disable();
            btnAdd->Disable();
            btnRemove->Disable();
        }
        else
        {
            pgSet *set;
            if (connection->BackendMinimumVersion(7, 4))
                set=connection->ExecuteSet(wxT("SELECT name, vartype, min_val, max_val\n")
                        wxT("  FROM pg_settings WHERE context='user'"));
            else
                set=connection->ExecuteSet(wxT("SELECT name, 'string' as vartype, '' as min_val, '' as max_val FROM pg_settings"));
            if (set)
            {
                while (!set->Eof())
                {
                    cbVarname->Append(set->GetVal(0));
                    varInfo.Add(set->GetVal(wxT("vartype")) + wxT(" ") + 
                                set->GetVal(wxT("min_val")) + wxT(" ") +
                                set->GetVal(wxT("max_val")));
                    set->MoveNext();
                }
                delete set;

                cbVarname->SetSelection(0);
            }
        }
    }
    else
    {
        // create mode

        txtComment->Disable();
        cbTemplate->Append(wxT(""));

        pgSet *set=connection->ExecuteSet(wxT(
            "SELECT datname FROM pg_database ORDER BY datname"));
        if (set)
        {
            while (!set->Eof())
            {
                cbTemplate->Append(set->GetVal(0));
                set->MoveNext();
            }
            delete set;
        }
        long encNo=0;
        wxString encStr;
        do
        {
            encStr=connection->ExecuteScalar(
                wxT("SELECT pg_encoding_to_char(") + NumToStr(encNo) + wxT(")"));
            if (!encStr.IsEmpty())
                cbEncoding->Append(encStr);

            encNo++;
        }
        while (!encStr.IsEmpty());

#if wxUSE_UNICODE
        encNo=cbEncoding->FindString(wxT("UNICODE"));
#else
        encNo=cbEncoding->FindString(wxT("SQL_ASCII"));
#endif
        if (encNo >= 0)
            cbEncoding->SetSelection(encNo);

    }
    return dlgSecurityProperty::Go(modal);
}


pgObject *dlgDatabase::CreateObject(pgCollection *collection)
{
    wxString name=GetName();

    pgObject *obj=pgDatabase::ReadObjects(collection, 0, wxT(" WHERE datname=") + qtString(name) + wxT("\n"));
    return obj;
}


void dlgDatabase::OnChange(wxNotifyEvent &ev)
{
    if (database)
    {
        EnableOK(!GetSql().IsEmpty());
    }
    else
    {
        bool enable=true;
        CheckValid(enable, !GetName().IsEmpty(), _("Please specify name."));
        EnableOK(enable);
    }
}


void dlgDatabase::OnVarnameSelChange(wxNotifyEvent &ev)
{
    int sel=cbVarname->GetSelection();
    if (sel >= 0)
    {
        wxStringTokenizer vals(varInfo.Item(sel));
        wxString typ=vals.GetNextToken();

        if (typ == wxT("bool"))
        {
            txtValue->Hide();
            chkValue->Show();
        }
        else
        {
            chkValue->Hide();
            txtValue->Show();
            if (typ == wxT("string"))
                txtValue->SetValidator(wxTextValidator());
            else
                txtValue->SetValidator(numericValidator);
        }
    }
}


void dlgDatabase::OnVarSelChange(wxListEvent &ev)
{
    long pos=GetListSelected(lstVariables);
    if (pos >= 0)
    {
        wxString value=GetListText(lstVariables, pos, 1);
        cbVarname->SetValue(lstVariables->GetItemText(pos));
        wxNotifyEvent nullev;
        OnVarnameSelChange(nullev);

        txtValue->SetValue(value);
        chkValue->SetValue(value == wxT("on"));
    }
}


void dlgDatabase::OnVarAdd(wxNotifyEvent &ev)
{
    wxString name=cbVarname->GetValue();
    wxString value;
    if (chkValue->IsShown())
        value = chkValue->GetValue() ? wxT("on") : wxT("off");
    else
        value = txtValue->GetValue().Strip(wxString::both);

    if (value.IsEmpty())
        value = wxT("DEFAULT");

    if (!name.IsEmpty())
    {
        long pos=lstVariables->FindItem(-1, name);
        if (pos < 0)
        {
            pos = lstVariables->GetItemCount();
            lstVariables->InsertItem(pos, name, 0);
        }
        lstVariables->SetItem(pos, 1, value);
    }
    OnChange(ev);
}


void dlgDatabase::OnVarRemove(wxNotifyEvent &ev)
{
    lstVariables->DeleteItem(lstVariables->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED));
    OnChange(ev);
}


wxString dlgDatabase::GetSql()
{
    wxString sql, name;
    name=GetName();

    if (database)
    {
        // edit mode

        wxArrayString vars;

        wxStringTokenizer cfgTokens(database->GetVariables(), wxT(","));
        while (cfgTokens.HasMoreTokens())
            vars.Add(cfgTokens.GetNextToken());

        int cnt=lstVariables->GetItemCount();
        int pos;

        // check for changed or added vars
        for (pos=0 ; pos < cnt ; pos++)
        {
            wxString newVar=lstVariables->GetItemText(pos);
            wxString newVal=GetListText(lstVariables, pos, 1);

            wxString oldVal;

            int index;
            for (index=0 ; index < (int)vars.GetCount() ; index++)
            {
                wxString var=vars.Item(index);
                if (var.BeforeFirst('=').IsSameAs(newVar, false))
                {
                    oldVal = var.Mid(newVar.Length()+1);
                    vars.RemoveAt(index);
                    break;
                }
            }
            if (oldVal != newVal)
            {
                sql += wxT("ALTER DATABASE ") + database->GetQuotedFullIdentifier()
                    +  wxT(" SET ") + newVar
                    +  wxT("=") + newVal
                    +  wxT(";\n");
            }
        }
        
        // check for removed vars
        for (pos=0 ; pos < (int)vars.GetCount() ; pos++)
        {
            sql += wxT("ALTER DATABASE ") + database->GetQuotedFullIdentifier()
                +  wxT(" RESET ") + vars.Item(pos).BeforeFirst('=')
                + wxT(";\n");
        }

        AppendComment(sql, wxT("DATABASE"), 0, database);
    }
    else
    {
        // create mode
        sql = wxT("CREATE DATABASE ") + qtIdent(name) 
            + wxT("\n  WITH ENCODING=") + qtString(cbEncoding->GetValue());

        AppendIfFilled(sql, wxT("\n       OWNER="), qtIdent(cbOwner->GetValue()));
        AppendIfFilled(sql, wxT("\n       TEMPLATE="), qtIdent(cbTemplate->GetValue()));
        AppendIfFilled(sql, wxT("\n       LOCATION="), txtPath->GetValue());

        sql += wxT(";\n");

    }
    sql += GetGrant(wxT("CT"), wxT("DATABASE ") + qtIdent(name));

    return sql;
}


