//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2010, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// dlgDatabase.h - Database property 
//
//////////////////////////////////////////////////////////////////////////


#ifndef __DLG_DATABASEPROP
#define __DLG_DATABASEPROP

#include "dlg/dlgProperty.h"

class pgDatabase;

class dlgDatabase : public dlgSecurityProperty
{
public:
    dlgDatabase(pgaFactory *factory, frmMain *frame, pgDatabase *db);
    int Go(bool modal);

    void CheckChange();
    wxString GetSql();
    wxString GetSql2();
    pgObject *CreateObject(pgCollection *collection);
    pgObject *GetObject();
    wxString GetHelpPage() const;

private:
    pgDatabase *database;
    wxArrayString varInfo;
    bool schemaRestrictionOk;

    void OnChangeRestr(wxCommandEvent &ev);
    void OnGroupAdd(wxCommandEvent &ev);
    void OnGroupRemove(wxCommandEvent &ev);

    void OnVarAdd(wxCommandEvent &ev);
    void OnVarRemove(wxCommandEvent &ev);
    void OnVarSelChange(wxListEvent &ev);

    void OnVarnameSelChange(wxCommandEvent &ev);
    void OnOK(wxCommandEvent &ev);

    void SetupVarEditor(int var);

	bool dirtyVars;

    DECLARE_EVENT_TABLE()

    friend class pgDatabaseFactory;
};


#endif
