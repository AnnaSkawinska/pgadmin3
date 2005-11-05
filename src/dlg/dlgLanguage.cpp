//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// dlgLanguage.cpp - PostgreSQL Language Property
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "misc.h"
#include "pgDefs.h"

#include "dlgLanguage.h"
#include "pgLanguage.h"


// pointer to controls
#define chkTrusted      CTRL_CHECKBOX("chkTrusted")
#define cbHandler       CTRL_COMBOBOX("cbHandler")
#define cbValidator     CTRL_COMBOBOX("cbValidator")


dlgProperty *pgLanguageFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
{
    return new dlgLanguage(this, frame, (pgLanguage*)node);
}


BEGIN_EVENT_TABLE(dlgLanguage, dlgSecurityProperty)
    EVT_TEXT(XRCID("cbHandler"),                    dlgProperty::OnChange)
    EVT_COMBOBOX(XRCID("cbHandler"),                dlgProperty::OnChange)
END_EVENT_TABLE();


dlgLanguage::dlgLanguage(pgaFactory *f, frmMain *frame, pgLanguage *node)
: dlgSecurityProperty(f, frame, node, wxT("dlgLanguage"), wxT("USAGE"), "U")
{
    language=node;
}


pgObject *dlgLanguage::GetObject()
{
    return language;
}


int dlgLanguage::Go(bool modal)
{
    if (!connection->BackendMinimumVersion(7, 5))
        txtComment->Disable();

    AddGroups();
    AddUsers();
    if (language)
    {
        // edit mode
        chkTrusted->SetValue(language->GetTrusted());
        cbHandler->Append(language->GetHandlerProc());
        cbHandler->SetSelection(0);
        wxString val=language->GetValidatorProc();
        if (!val.IsEmpty())
        {
            cbValidator->Append(val);
            cbValidator->SetSelection(0);
        }

        if (!connection->BackendMinimumVersion(7, 4))
            txtName->Disable();
        cbHandler->Disable();
        chkTrusted->Disable();
        cbValidator->Disable();
    }
    else
    {
        // create mode
        cbValidator->Append(wxT(""));
        pgSet *set=connection->ExecuteSet(
            wxT("SELECT nspname, proname, prorettype\n")
            wxT("  FROM pg_proc p\n")
            wxT("  JOIN pg_namespace nsp ON nsp.oid=pronamespace\n")
            wxT(" WHERE prorettype=2280 OR (prorettype=") + NumToStr(PGOID_TYPE_VOID) +
            wxT(" AND proargtypes[0]=") + NumToStr(PGOID_TYPE_LANGUAGE_HANDLER) + wxT(")"));
        if (set)
        {
            while (!set->Eof())
            {
                wxString procname = database->GetSchemaPrefix(set->GetVal(wxT("nspname"))) + set->GetVal(wxT("proname"));

                if (set->GetOid(wxT("prorettype")) == 2280)
                    cbHandler->Append(procname);
                else
                    cbValidator->Append(procname);
                set->MoveNext();
            }
            delete set;
        }
    }

    return dlgSecurityProperty::Go(modal);
}


pgObject *dlgLanguage::CreateObject(pgCollection *collection)
{
    wxString name=GetName();

    pgObject *obj=languageFactory.CreateObjects(collection, 0, wxT("\n   AND lanname ILIKE ") + qtString(name));
    return obj;
}


void dlgLanguage::CheckChange()
{
    wxString name=GetName();
    if (language)
    {
        EnableOK(name != language->GetName() || txtComment->GetValue() != language->GetComment());
    }
    else
    {

        bool enable=true;
        CheckValid(enable, !name.IsEmpty(), _("Please specify name."));
        CheckValid(enable, !cbHandler->GetValue().IsEmpty(), _("Please specify language handler."));
        EnableOK(enable);
    }
}



wxString dlgLanguage::GetSql()
{
    wxString sql, name;
    name=GetName();

    if (language)
    {
        // edit mode
        AppendNameChange(sql);
    }
    else
    {
        // create mode
        sql = wxT("CREATE ");
        if (chkTrusted->GetValue())
            sql += wxT("TRUSTED ");
        sql += wxT("LANGUAGE ") + qtIdent(name) + wxT("\n   HANDLER ") + qtIdent(cbHandler->GetValue());
        AppendIfFilled(sql, wxT("\n   VALIDATOR "), qtIdent(cbValidator->GetValue()));
        sql += wxT(";\n");

    }

    sql += GetGrant(wxT("X"), wxT("LANGUAGE ") + qtIdent(name));
    AppendComment(sql, wxT("LANGUAGE"), 0, language);

    return sql;
}


