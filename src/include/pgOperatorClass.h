//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgOperatorClass.h PostgreSQL OperatorClass
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGOperatorClass_H
#define PGOperatorClass_H

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "pgObject.h"
#include "pgServer.h"
#include "pgDatabase.h"

class pgCollection;

class pgOperatorClass : public pgSchemaObject
{
public:
    pgOperatorClass(pgSchema *newSchema, const wxString& newName = wxT(""));
    ~pgOperatorClass();

    int GetIcon() { return PGICON_OPERATORCLASS; }
    void ShowTreeDetail(wxTreeCtrl *browser, frmMain *form=0, ctlListView *properties=0, ctlSQLBox *sqlPane=0);
    static pgObject *ReadObjects(pgCollection *collection, wxTreeCtrl *browser, const wxString &restriction=wxT(""));

    wxString GetFullName() const { return GetName() + wxT("(") + GetAccessMethod() + wxT(")"); }
        wxString GetAccessMethod() const { return accessMethod; }
    void iSetAccessMethod(const wxString&s) { accessMethod=s; }

    wxArrayString GetOperators() { return operators; }
    wxArrayString GetFunctions() { return functions; }
    wxArrayString GetQuotedFunctions() { return quotedFunctions; }
    wxString GetInType() const {return inType; }
    void iSetInType(const wxString&s) { inType=s; }
    wxString GetKeyType() const {return keyType; }
    void iSetKeyType(const wxString&s) { keyType=s; }
    wxString GetSql(wxTreeCtrl *browser);
    bool GetOpcDefault() const { return opcDefault; }
    void iSetOpcDefault(const bool b) { opcDefault=b; }

    bool CanCreate() { return false; }
    bool CanEdit() { return false; }
    bool DropObject(wxFrame *frame, wxTreeCtrl *browser);
    wxString GetHelpPage(bool forCreate) const { return wxT("sql-createopclass"); }
    pgObject *Refresh(wxTreeCtrl *browser, const wxTreeItemId item);

private:
    wxString inType, keyType, accessMethod;
    wxArrayString operators;
    wxArrayString functions, quotedFunctions;
    wxArrayString functionOids;
    bool opcDefault;
};

#endif
