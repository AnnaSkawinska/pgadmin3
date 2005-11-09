//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgServer.h - PostgreSQL Server
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGSERVER_H
#define PGSERVER_H

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "pgConn.h"
#include "pgObject.h"
// Class declarations


class frmMain;

class pgServer : public pgObject
{
public:
    pgServer(const wxString& newServer = wxT(""), const wxString& newDescription = wxT(""), const wxString& newDatabase = wxT(""), const wxString& newUsername = wxT(""), int newPort = 5432, bool trusted=false, int sslMode=0);
    ~pgServer();
    int GetType() const { return PG_SERVER; }
    wxString GetTypeName() const { return wxT("Server"); }
    int Connect(frmMain *form, bool askPassword=true, const wxString &pwd=wxEmptyString);
    bool Disconnect();

    bool StartService();
    bool StopService();
    bool GetServerRunning();
    bool GetServerControllable();

    pgServer *GetServer() const;

    wxString GetIdentifier() const;
    wxString GetVersionString();
    wxString GetVersionNumber();
    OID GetLastSystemOID();
    wxString GetDatabaseName() const { return database; }
    wxString GetUsername() const { return username; }
    wxString GetPassword() const { return password; }
    bool GetTrusted() const { return trusted; }
    bool GetNeedPwd() const { return !trusted; }
    wxString GetLastError() const;

    bool GetDiscovered() const { return discovered; }
    void iSetDiscovered(const bool b) { discovered=b; }
    wxString GetServiceID() const { return serviceId; }
    void iSetServiceID(const wxString& s);

    bool GetCreatePrivilege() const { return createPrivilege; }
    void iSetCreatePrivilege(const bool b) { createPrivilege=b; }
    bool GetSuperUser() const { return superUser; }
    void iSetSuperUser(const bool b) { superUser=b; }

    pgConn *CreateConn(wxString dbName=wxEmptyString, OID oid=0);

    wxString GetLastDatabase() const { return lastDatabase; }
    void iSetLastDatabase(const wxString& s) { lastDatabase=s; }
    wxString GetLastSchema() const { return lastSchema; }
    void iSetLastSchema(const wxString& s) { lastSchema=s; }
    wxString GetDescription() const { return description; }
    void iSetDescription(const wxString& s) { description=s; }

    wxString GetFullName() const;
    int GetPort() const { return port; }
    int GetSSL() const { return ssl; }
    bool GetConnected() const { return connected; }
    void iSetDatabase(const wxString& newVal) { database = newVal; }
    void iSetPort(int newval) { port=newval; }
    void iSetSSL(int newval) { ssl=newval; }
    void iSetUsername(const wxString& newVal) { username = newVal; }
    void iSetPassword(const wxString& newVal) { password = newVal; }
    void iSetTrusted(const bool b) { trusted=b; }
    void iSetNeedPwd(const bool b) { trusted=!b; }
    bool SetPassword(const wxString& newVal);
    wxDateTime GetUpSince() { return upSince; }
    void iSetUpSince(const wxDateTime &d) { upSince = d; }

    bool HasPrivilege(const wxString &objTyp, const wxString &objName, const wxString &priv) { return conn->HasPrivilege(objTyp, objName, priv); }
    bool ExecuteVoid(const wxString& sql) { return conn->ExecuteVoid(sql); }
    wxString ExecuteScalar(const wxString& sql) { return conn->ExecuteScalar(sql); }
    pgSet *ExecuteSet(const wxString& sql) { return conn->ExecuteSet(sql); }
    void ShowTreeDetail(wxTreeCtrl *browser, frmMain *form=0, ctlListView *properties=0, ctlSQLBox *sqlPane=0);
    void ShowStatistics(frmMain *form, ctlListView *statistics);
    wxString GetHelpPage(bool forCreate) const { return wxT("managing-databases"); }
    int GetIcon() { return PGICON_SERVER; }
    wxMenu *GetNewMenu();

    bool DropObject(wxFrame *frame, wxTreeCtrl *browser) { return true; }
    bool CanEdit() { return true; }
    bool CanDrop() { return true; }

    pgConn *connection() { return conn; }
    
private:
    pgConn *conn;
    bool connected;
    wxString database, username, password, ver, error;
    wxString lastDatabase, lastSchema, description, serviceId;
    wxDateTime upSince;
    int port, ssl;
    bool trusted, discovered, createPrivilege, superUser;
    OID lastSystemOID;
    OID dbOid;
    wxString versionNum;

#ifdef WIN32
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;
#endif
};

#endif
