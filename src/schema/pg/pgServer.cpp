//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// Copyright (C) 2002, The pgAdmin Development Team
// This software is released under the pgAdmin Public Licence
//
// pgServer.cpp - PostgreSQL Server
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "../../pgAdmin3.h"
#include "../../ui/forms/frmConnect.h"
#include "pgServer.h"
#include "pgObject.h"


pgServer::pgServer(const wxString& szNewServer, const wxString& szNewDatabase, const wxString& szNewUsername, int iNewPort)
: pgObject()
{  
    wxLogInfo(wxT("Creating a pgServer object"));
    szServer = szNewServer;
    szDatabase = szNewDatabase;
    szUsername = szNewUsername;
    iPort = iNewPort;
    bConnected = FALSE;
    fVer = 0.0;
    lLastSystemOID = 0;
}

pgServer::~pgServer()
{
    wxLogInfo(wxT("Destroying a pgServer object"));
}

int pgServer::GetType()
{
    return PG_SERVER;
}

wxString pgServer::GetTypeName() const
{
    return wxString("Server");
}

int pgServer::Connect(bool bLockFields) {

    wxLogInfo(wxT("Getting connection details..."));
    frmConnect *winConnect = new frmConnect(this, szServer, szDatabase, szUsername, iPort);
    if (bLockFields) winConnect->LockFields();

    if (winConnect->ShowModal() != 0) {
        delete winConnect;
        return PGCONN_ABORTED;
    }

    wxLogInfo(wxT("Attempting to create a connection object..."));
    StartMsg(wxT("Connecting to database"));
    cnMaster = new pgConn(szServer, szDatabase, szUsername, szPassword, iPort);

    delete winConnect;
    EndMsg();
    int iStatus = cnMaster->GetStatus();
    if (iStatus == PGCONN_OK) {
        bConnected = TRUE;
    } else {
        bConnected = FALSE;
    }

    return iStatus;
}

wxString pgServer::GetIdentifier() const
{
    wxString szID;
    szID.Printf(wxT("%s:%d"), szServer.c_str(), iPort);
    return wxString(szID);
}

wxString pgServer::GetVersionString()
{
    if (bConnected) {
      if (szVer.IsEmpty()) {
          szVer = wxString(cnMaster->GetVersionString());
      }
      return szVer;
    } else {
        return wxString("");
    }
}

float pgServer::GetVersionNumber()
{
    if (bConnected) {
      if (fVer == 0) {
          fVer = cnMaster->GetVersionNumber();
      }
      return fVer;
    } else {
        return 0.0;
    }
}

long pgServer::GetLastSystemOID()
{
    if (bConnected) {
      if (lLastSystemOID == 0) {
          lLastSystemOID = cnMaster->GetLastSystemOID();
      }
      return lLastSystemOID;
    } else {
        return 0;
    }
}

wxString pgServer::GetServer() const
{
    return szServer;
}
void pgServer::iSetServer(const wxString& szNewVal)
{
    szServer = szNewVal;
}

wxString pgServer::GetDatabase() const
{
    return szDatabase;
}
void pgServer::iSetDatabase(const wxString& szNewVal)
{
    szDatabase = szNewVal;
}

wxString pgServer::GetUsername() const
{
    return szUsername;
}
void pgServer::iSetUsername(const wxString& szNewVal)
{
    szUsername = szNewVal;
}

wxString pgServer::GetPassword() const
{
    return szPassword;
}
void pgServer::iSetPassword(const wxString& szNewVal)
{
    szPassword = szNewVal;
}

int pgServer::GetPort()
{
    return iPort;
}
void pgServer::iSetPort(int iNewVal)
{
    iPort = iNewVal;
}

wxString pgServer::GetLastError() const
{
    if (bConnected) {
        return cnMaster->GetLastError();
    } else {
        return wxString("");
    }
}

bool pgServer::GetConnected()
{
    return bConnected;
}

int pgServer::ExecuteVoid(const wxString& szSQL)
{
    return cnMaster->ExecuteVoid(szSQL);
}

wxString pgServer::ExecuteScalar(const wxString& szSQL) const
{
    return cnMaster->ExecuteScalar(szSQL);
}

pgSet pgServer::ExecuteSet(const wxString& szSQL)
{
    return *cnMaster->ExecuteSet(szSQL);
}