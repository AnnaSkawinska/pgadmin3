//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgConn.cpp - PostgreSQL Connection class
//
/////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>

// PostgreSQL headers
#include <libpq-fe.h>
#include "pgfeatures.h"

// Network  headers
#ifdef __WXMSW__
#include <winsock.h>
#else

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

#endif

// App headers
#include "pgConn.h"
#include "misc.h"
#include "pgSet.h"
#include "sysLogger.h"


extern double libpqVersion;

static void pgNoticeProcessor(void *arg, const char *message)
{
    ((pgConn*)arg)->Notice(message);
}


pgConn::pgConn(const wxString& server, const wxString& database, const wxString& username, const wxString& password, int port, int sslmode, OID oid)
{
    wxLogInfo(wxT("Creating pgConn object"));
    wxString msg, hostip;

    conv = &wxConvLibc;
    needColQuoting = false;

    // Check the hostname/ipaddress
    struct hostent *host;
    unsigned long addr;
    conn=0;
    majorVersion=0;
    noticeArg=0;
    connStatus = PGCONN_BAD;
    memset(features, 0, sizeof(features));
    
#ifdef __WXMSW__
    struct in_addr ipaddr;
#else
    unsigned long ipaddr;
#endif
    
    
    addr = inet_addr(server.ToAscii());
	if (addr == INADDR_NONE) // szServer is not an IP address
	{
		host = gethostbyname(server.ToAscii());
		if (host == NULL)
		{
            connStatus = PGCONN_DNSERR;
            wxLogError(__("Could not resolve hostname %s"), server.c_str());
			return;
		}

        memcpy(&(ipaddr),host->h_addr,host->h_length); 
	    hostip = wxString::FromAscii(inet_ntoa(*((struct in_addr*) host->h_addr_list[0])));

    }
    else
        hostip = server;

    wxLogInfo(wxT("Server name: %s (resolved to: %s)"), server.c_str(), hostip.c_str());

    // Create the connection string
    wxString connstr;
    if (!server.IsEmpty()) {
      connstr.Append(wxT(" hostaddr="));
      connstr.Append(qtString(hostip));
    }
    if (!database.IsEmpty()) {
      connstr.Append(wxT(" dbname="));
      connstr.Append(qtString(database));
    }
    if (!username.IsEmpty()) {
      connstr.Append(wxT(" user="));
      connstr.Append(qtString(username));
    }
    if (!password.IsEmpty()) {
      connstr.Append(wxT(" password="));
      connstr.Append(qtString(password));
    }
    if (port > 0) {
      connstr.Append(wxT(" port="));
      connstr.Append(NumToStr((long)port));
    }

    if (libpqVersion > 7.3)
    {
        switch (sslmode)
        {
            case 1: connstr.Append(wxT(" sslmode=require"));   break;
            case 2: connstr.Append(wxT(" sslmode=prefer"));    break;
            case 3: connstr.Append(wxT(" sslmode=allow"));     break;
            case 4: connstr.Append(wxT(" sslmode=disable"));   break;
        }
    }
    else
    {
        switch (sslmode)
        {
            case 1: connstr.Append(wxT(" requiressl=1"));   break;
            case 2: connstr.Append(wxT(" requiressl=0"));   break;
        }
    }
    connstr.Trim(FALSE);

    // Open the connection
    wxLogInfo(wxT("Opening connection with connection string: %s"), connstr.c_str());

#if wxUSE_UNICODE
    conn = PQconnectdb(connstr.mb_str(wxConvUTF8));
    if (PQstatus(conn) != CONNECTION_OK)
    {
        PQfinish(conn);
        conn = PQconnectdb(connstr.mb_str(wxConvLibc));
    }
#else
    conn = PQconnectdb(connstr.ToAscii());
#endif

    dbHost = server;

    // Set client encoding to Unicode/Ascii
    if (PQstatus(conn) == CONNECTION_OK)
    {
        connStatus = PGCONN_OK;
        PQsetNoticeProcessor(conn, pgNoticeProcessor, this);


        wxString sql=wxT("SELECT oid, pg_encoding_to_char(encoding) AS encoding, datlastsysoid\n")
                      wxT("  FROM pg_database WHERE ");
        if (oid)
            sql += wxT("oid = ") + NumToStr(oid);
        else
            sql += wxT("datname=") + qtString(database);

        pgSet *set = ExecuteSet(sql);


        if (set)
        {
            if (set->ColNumber(wxT("\"datlastsysoid\"")) >= 0)
                needColQuoting = true;

            lastSystemOID = set->GetOid(wxT("datlastsysoid"));
            dbOid = set->GetOid(wxT("oid"));
            wxString encoding = set->GetVal(wxT("encoding"));

#if wxUSE_UNICODE
            if (encoding != wxT("SQL_ASCII") && encoding != wxT("MULE_INTERNAL"))
            {
                encoding = wxT("UNICODE");
                conv = &wxConvUTF8;
            }
            else
                conv = &wxConvLibc;
#endif

            wxLogInfo(wxT("Setting client_encoding to '%s'"), encoding.c_str());
            if (PQsetClientEncoding(conn, encoding.ToAscii()))
				wxLogError(wxT("%s"), GetLastError().c_str());

            delete set;
        }
    }
}


pgConn::~pgConn()
{
    wxLogInfo(wxT("Destroying pgConn object"));
    Close();
}



void pgConn::Close()
{
    if (conn)
        PQfinish(conn);
    conn=0;
    connStatus=PGCONN_BAD;
}


#ifdef SSL
// we don't define USE_SSL so we don't get ssl.h included
extern "C"
{
extern void *PQgetssl(PGconn *conn);
}

bool pgConn::IsSSLconnected()
{
    return (conn && PQstatus(conn) == CONNECTION_OK && PQgetssl(conn) != NULL);
}
#endif


void pgConn::RegisterNoticeProcessor(PQnoticeProcessor proc, void *arg)
{
    noticeArg=arg;
    noticeProc=proc;
}


wxString pgConn::SystemNamespaceRestriction(const wxString &nsp)
{
    return wxT("(") + nsp + wxT(" NOT LIKE 'pg\\_%' AND ") + nsp + wxT(" NOT LIKE 'information_schema')");
}


void pgConn::Notice(const char *msg)
{
    wxString str(msg, *conv);
    wxLogNotice(wxT("%s"), str.c_str());

    if (noticeArg && noticeProc)
        (*noticeProc)(noticeArg, msg);
}


//////////////////////////////////////////////////////////////////////////
// Execute SQL
//////////////////////////////////////////////////////////////////////////

bool pgConn::ExecuteVoid(const wxString& sql)
{
    if (GetStatus() != PGCONN_OK)
        return false;

    // Execute the query and get the status.
    PGresult *qryRes;

    wxLogSql(wxT("Void query (%s:%d): %s"), this->GetHost().c_str(), this->GetPort(), sql.c_str());
    qryRes = PQexec(conn, sql.mb_str(*conv));
    lastResultStatus = PQresultStatus(qryRes);

    // Check for errors
    if (lastResultStatus != PGRES_TUPLES_OK &&
        lastResultStatus != PGRES_COMMAND_OK)
    {
        LogError();
        return false;
    }

    // Cleanup & exit
    PQclear(qryRes);
    return  true;
}


bool pgConn::HasPrivilege(const wxString &objTyp, const wxString &objName, const wxString &priv)
{
    wxString res=ExecuteScalar(
        wxT("SELECT has_") + objTyp.Lower() 
        + wxT("_privilege(") + qtString(objName)
        + wxT(", ") + qtString(priv) + wxT(")"));

    return StrToBool(res);
}


wxString pgConn::ExecuteScalar(const wxString& sql)
{
    wxString result;

    if (GetStatus() == PGCONN_OK)
    {
        // Execute the query and get the status.
        PGresult *qryRes;
        wxLogSql(wxT("Scalar query (%s:%d): %s"), this->GetHost().c_str(), this->GetPort(), sql.c_str());
        qryRes = PQexec(conn, sql.mb_str(*conv));
        lastResultStatus = PQresultStatus(qryRes);
        
        // Check for errors
        if (lastResultStatus != PGRES_TUPLES_OK)
        {
            LogError();
            PQclear(qryRes);
            return wxEmptyString;
        }

	    // Check for a returned row
        if (PQntuples(qryRes) < 1)
        {
		    wxLogInfo(wxT("Query returned no tuples"));
            PQclear(qryRes);
            return wxEmptyString;
	    }
	    
	    // Retrieve the query result and return it.
        result=wxString(PQgetvalue(qryRes, 0, 0), *conv);

        wxLogSql(wxT("Query result: %s"), result.c_str());

        // Cleanup & exit
        PQclear(qryRes);
    }

    return result;
}

pgSet *pgConn::ExecuteSet(const wxString& sql)
{
    // Execute the query and get the status.
    if (GetStatus() == PGCONN_OK)
    {
        PGresult *qryRes;
        wxLogSql(wxT("Set query (%s:%d): %s"), this->GetHost().c_str(), this->GetPort(), sql.c_str());
        qryRes = PQexec(conn, sql.mb_str(*conv));

        lastResultStatus= PQresultStatus(qryRes);

        if (lastResultStatus == PGRES_TUPLES_OK || lastResultStatus == PGRES_COMMAND_OK)
        {
            pgSet *set = new pgSet(qryRes, this, *conv, needColQuoting);
            if (!set)
            {
                wxLogError(__("Couldn't create a pgSet object!"));
                PQclear(qryRes);
            }
    	    return set;
        }
        else
        {
            LogError();
            PQclear(qryRes);
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Info
//////////////////////////////////////////////////////////////////////////

wxString pgConn::GetLastError() const
{ 
    wxString errmsg;
	char *pqErr;
    if (conn && (pqErr = PQerrorMessage(conn)) != 0)
        errmsg=wxString(pqErr, wxConvUTF8);
    else
    {
        if (connStatus == PGCONN_BROKEN)
            errmsg = _("Connection to database broken.");
        else
            errmsg = _("No connection to database.");
    }
    return errmsg;
}



void pgConn::LogError()
{
    if (conn)
    {
        wxLogError(wxT("%s"), GetLastError().c_str());

        IsAlive();
#if 0
        ConnStatusType status = PQstatus(conn);
        if (status == CONNECTION_BAD)
        {
            PQfinish(conn);
            conn=0;
            connStatus = PGCONN_BROKEN;
        }
#endif
    }
}



bool pgConn::IsAlive()
{
    if (GetStatus() != PGCONN_OK)
        return false;

    PGresult *qryRes = PQexec(conn, "SELECT 1;");
    lastResultStatus = PQresultStatus(qryRes);
    PQclear(qryRes);

    // Check for errors
    if (lastResultStatus != PGRES_TUPLES_OK)
    {
        PQfinish(conn);
        conn=0;
        connStatus = PGCONN_BROKEN;
        return false;
    }

    return true;
}


int pgConn::GetStatus() const
{
    if (conn)
        ((pgConn*)this)->connStatus = PQstatus(conn);

    return connStatus;
}


wxString pgConn::GetVersionString()
{
	return ExecuteScalar(wxT("SELECT version();"));
}


bool pgConn::BackendMinimumVersion(int major, int minor)
{
    if (!majorVersion)
    {
	    sscanf(GetVersionString().ToAscii(), "%*s %d.%d", &majorVersion, &minorVersion);
    }
	return majorVersion > major || (majorVersion == major && minorVersion >= minor);
}


bool pgConn::HasFeature(int featureNo)
{
    if (!features[FEATURE_INITIALIZED])
    {
        features[FEATURE_INITIALIZED] = true;

        pgSet *set=ExecuteSet(
            wxT("SELECT proname, pronargs, proargtypes[0] AS arg0, proargtypes[1] AS arg1, proargtypes[2] AS arg2\n")
            wxT("  FROM pg_proc\n")
            wxT(" WHERE proname IN ('pg_tablespace_size', 'pg_file_read', 'pg_rotate_log',")
            wxT(                  " 'pg_postmaster_starttime', 'pg_terminate_backend', 'pg_reload_conf')"));

        if (set)
        {
            while (!set->Eof())
            {
                wxString proname=set->GetVal(wxT("proname"));
                long pronargs = set->GetLong(wxT("pronargs"));

                if (proname == wxT("pg_tablespace_size") && pronargs == 1 && set->GetLong(wxT("arg0")) == 26)
                    features[FEATURE_SIZE]= true;
                else if (proname == wxT("pg_file_read") && pronargs == 3 && set->GetLong(wxT("arg0")) == 25
                    && set->GetLong(wxT("arg1")) == 20 && set->GetLong(wxT("arg2")) == 20)
                    features[FEATURE_FILEREAD] = true;
                else if (proname == wxT("pg_rotate_log") && pronargs == 0)
                    features[FEATURE_ROTATELOG] = true;
                else if (proname == wxT("pg_postmaster_starttime") && pronargs == 0)
                    features[FEATURE_POSTMASTER_STARTTIME] = true;
                else if (proname == wxT("pg_terminate_backend") && pronargs == 1 && set->GetLong(wxT("arg0")) == 23)
                    features[FEATURE_TERMINATE_BACKEND] = true;
                else if (proname == wxT("pg_reload_conf") && pronargs == 0)
                    features[FEATURE_RELOAD_CONF] = true;

                set->MoveNext();
            }
            delete set;
        }
    }

    if (featureNo <= FEATURE_INITIALIZED || featureNo >= FEATURE_LAST)
        return false;
    return features[featureNo];
}
