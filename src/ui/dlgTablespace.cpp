//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2004, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// dlgTablespace.cpp - Tablespace property 
//
//////////////////////////////////////////////////////////////////////////



#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>

// Images
#include "images/tablespace.xpm"

// App headers
#include "misc.h"
#include "dlgTablespace.h"
#include "pgTablespace.h"


// pointer to controls
#define txtLocation     CTRL_TEXT("txtLocation")


BEGIN_EVENT_TABLE(dlgTablespace, dlgProperty)
    EVT_TEXT(XRCID("txtName"),                      dlgTablespace::OnChange)
    EVT_TEXT(XRCID("txtLocation"),                  dlgTablespace::OnChange)
    EVT_TEXT(XRCID("cbOwner"),                      dlgTablespace::OnOwnerChange)
    EVT_TEXT(XRCID("txtComment"),                   dlgTablespace::OnChange)
END_EVENT_TABLE();



dlgTablespace::dlgTablespace(frmMain *frame, pgTablespace *node)
: dlgProperty(frame, wxT("dlgTablespace"))
{
    tablespace=node;
    SetIcon(wxIcon(tablespace_xpm));
    btnOK->Disable();
}


pgObject *dlgTablespace::GetObject()
{
    return tablespace;
}


int dlgTablespace::Go(bool modal)
{
    AddUsers(cbOwner);
    txtComment->Disable();

    if (tablespace)
    {
        // Edit Mode
        txtName->SetValue(tablespace->GetIdentifier());
        txtLocation->SetValue(tablespace->GetLocation());
        cbOwner->SetValue(tablespace->GetOwner());

        txtName->Disable();
        txtLocation->Disable();
        cbOwner->Disable();
    }
    else
    {
    }

    return dlgProperty::Go(modal);
}


void dlgTablespace::OnOwnerChange(wxCommandEvent &ev)
{
    cbOwner->GuessSelection();
    OnChange(ev);
}


void dlgTablespace::OnChange(wxCommandEvent &ev)
{
    if (tablespace)
    {
        EnableOK(txtComment->GetValue() != tablespace->GetComment());
    }
    else
    {
        wxString name=GetName();

        bool enable=true;
        CheckValid(enable, !name.IsEmpty(), _("Please specify name."));
        CheckValid(enable, !txtLocation->GetValue().IsEmpty(), _("Please specify location."));
        EnableOK(enable);
    }
}


pgObject *dlgTablespace::CreateObject(pgCollection *collection)
{
    wxString name=GetName();

    pgObject *obj=pgTablespace::ReadObjects(collection, 0, wxT("\n WHERE spcname=") + qtString(name));
    return obj;
}


wxString dlgTablespace::GetSql()
{
    wxString sql;
    wxString name=GetName();
    

    if (tablespace)
    {
        // Edit Mode
    }
    else
    {
        // Create Mode
        sql = wxT("CREATE TABLESPACE ") + qtIdent(name);
        AppendIfFilled(sql, wxT(" OWNER "), qtIdent(cbOwner->GetValue()));
        sql += wxT(" LOCATION ") + qtString(txtLocation->GetValue())
            +  wxT(";\n");
    }
    return sql;
}

