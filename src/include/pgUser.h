//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgUser.h - PostgreSQL User
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGUSER_H
#define PGUSER_H

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "pgConn.h"
#include "pgObject.h"
#include "pgServer.h"

// Class declarations
class pgUser : public pgServerObject
{
public:
    pgUser(const wxString& newName = wxT(""));
    ~pgUser();


    // User Specific
    bool GetSystemObject() const { return userId < 100; }
    long GetUserId() const { return userId; }
    void iSetUserId(const long l) { userId=l; }
    wxDateTime GetAccountExpires() const { return accountExpires; }
    void iSetAccountExpires(const wxDateTime & dt) { accountExpires=dt; }
    wxString GetPassword() const { return password; }
    void iSetPassword(const wxString& s) { password=s; }
    bool GetCreateDatabase() const { return createDatabase; }
    void iSetCreateDatabase(const bool b) { createDatabase=b; }
    bool GetSuperuser() const { return superuser; }
    void iSetSuperuser(const bool b) { superuser=b; }
    bool GetUpdateCatalog() const { return updateCatalog; }
    void iSetUpdateCatalog(const bool b) { updateCatalog=b; }
    wxArrayString& GetGroupsIn() { return groupsIn; }
    wxArrayString& GetConfigList() { return configList; }


    // Tree object creation
    int GetIcon() { return PGICON_USER; }
    void ShowTreeDetail(wxTreeCtrl *browser, frmMain *form=0, ctlListView *properties=0, ctlSQLBox *sqlPane=0);
    static pgObject *ReadObjects(pgCollection *collection, wxTreeCtrl *browser, const wxString &restriction=wxT(""));
    void ShowReferencedBy(frmMain *form, ctlListView *referencedBy, const wxString &where);
    
    // virtual methods
    wxString GetSql(wxTreeCtrl *browser);
    pgObject *Refresh(wxTreeCtrl *browser, const wxTreeItemId item);
    bool DropObject(wxFrame *frame, wxTreeCtrl *browser);

private:
    wxString password;
    wxDateTime accountExpires;
    bool createDatabase, superuser, updateCatalog;
    wxArrayString groupsIn;
    wxArrayString configList;
    long userId;
};

#endif
