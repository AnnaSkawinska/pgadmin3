//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// Copyright (C) 2002 - 2003, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgSet.cpp - PostgreSQL ResultSet class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// PostgreSQL headers
#include <libpq-fe.h>


// App headers
#include "pgSet.h"
#include "pgConn.h"
#include "pgAdmin3.h"
#include "sysLogger.h"
#include "pgDefs.h"

pgSet::pgSet(PGresult *newRes, PGconn *newConn, wxMBConv &cnv, bool needColQt)
: conv(cnv)
{
    needColQuoting = needColQt;

    wxLogInfo(wxT("Creating pgSet object"));
    conn = newConn;
    res = newRes;

    // Make sure we have tuples
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        nRows = 0;
        pos = 0;
    }
    else
    {
        nRows = PQntuples(res);
        MoveFirst();
    }
}


pgSet::~pgSet()
{
    wxLogInfo(wxT("Destroying pgSet object"));
    PQclear(res);
}



OID pgSet::ColTypeOid(int col) const
{
    return PQftype(res, col);
}


pgTypClass pgSet::ColTypClass(int col) const
{
    wxString typoid=ExecuteScalar(
        wxT("SELECT CASE WHEN typbasetype=0 THEN oid else typbasetype END AS basetype\n")
        wxT("  FROM pg_type WHERE oid=") + NumToStr(ColTypeOid(col)));

    switch (StrToLong(typoid))
    {
        case PGOID_TYPE_BOOL:
            return PGTYPCLASS_BOOL;

        case PGOID_TYPE_INT8:
        case PGOID_TYPE_INT2:
        case PGOID_TYPE_INT4:
        case PGOID_TYPE_OID:
        case PGOID_TYPE_XID:
        case PGOID_TYPE_TID:
        case PGOID_TYPE_CID:
        case PGOID_TYPE_FLOAT4:
        case PGOID_TYPE_FLOAT8:
        case PGOID_TYPE_MONEY:
        case PGOID_TYPE_BIT:
        case PGOID_TYPE_NUMERIC:
            return PGTYPCLASS_NUMERIC;
        case PGOID_TYPE_BYTEA:
        case PGOID_TYPE_CHAR:
        case PGOID_TYPE_NAME:
        case PGOID_TYPE_TEXT:
        case PGOID_TYPE_VARCHAR:
            return PGTYPCLASS_STRING;
        case PGOID_TYPE_TIMESTAMP:
        case PGOID_TYPE_TIMESTAMPTZ:
        case PGOID_TYPE_TIME:
        case PGOID_TYPE_TIMETZ:
        case PGOID_TYPE_INTERVAL:
            return PGTYPCLASS_DATE;
        default:
            return PGTYPCLASS_OTHER;
    }
}


wxString pgSet::ColType(int col) const
{
    wxString szSQL, szResult;
    szSQL.Printf(wxT("SELECT typname FROM pg_type WHERE oid = %d"), ColTypeOid(col));
    szResult = ExecuteScalar(szSQL);
    return szResult;
}

int pgSet::ColScale(int col) const
{
    // TODO
    return 0;
}
wxString pgSet::ColName(int col) const
{
    return wxString(PQfname(res, col), conv);
}


int pgSet::ColNumber(const wxString &colname) const
{
    int col;
    
    if (needColQuoting)
        col = PQfnumber(res, qtIdent(colname).mb_str(conv));
    else
        col = PQfnumber(res, colname.mb_str(conv));

    if (col < 0)
        wxLogError(__("Column not found in pgSet: ") + colname);
    return col;
}


wxString pgSet::GetVal(const int col) const
{
    return wxString(PQgetvalue(res, pos -1, col), conv);
}


wxString pgSet::GetVal(const wxString& colname) const
{
    return GetVal(ColNumber(colname));
}


long pgSet::GetLong(const int col) const
{
    char *c=PQgetvalue(res, pos-1, col);
    if (c)
        return atol(c);
    else
        return 0;
}


long pgSet::GetLong(const wxString &col)
{
    char *c=PQgetvalue(res, pos-1, ColNumber(col));
    if (c)
        return atol(c);
    else
        return 0;
}


bool pgSet::GetBool(const int col) const
{
    char *c=PQgetvalue(res, pos-1, col);
    if (c)
    {
        if (*c == 't' || *c == '1')
            return true;
    }
    return false;
}


bool pgSet::GetBool(const wxString &col) const
{
    return GetBool(ColNumber(col));
}


wxDateTime pgSet::GetDateTime(const int col) const
{
    wxDateTime dt;
    wxString str=GetVal(col);
    dt.ParseDateTime(str);
    return dt;
}


wxDateTime pgSet::GetDateTime(const wxString &col) const
{
    return GetDateTime(ColNumber(col));
}


double pgSet::GetDouble(const int col) const
{
    char *c=PQgetvalue(res, pos-1, col);
    if (c)
        return strtod(c, 0);
    else
        return 0;
}


double pgSet::GetDouble(const wxString &col) const
{
    return GetDouble(ColNumber(col));
}


wxULongLong pgSet::GetLongLong(const int col) const
{
    char *c=PQgetvalue(res, pos-1, col);
    if (c)
        return atolonglong(c);
    else
        return 0;
}

wxULongLong pgSet::GetLongLong(const wxString &col) const
{
    return GetLongLong(ColNumber(col));
}


OID pgSet::GetOid(const int col) const
{
    return (OID)GetLong(col);
}


OID pgSet::GetOid(const wxString &col) const
{
    return GetOid(ColNumber(col));
}


wxString pgSet::ExecuteScalar(const wxString& sql) const
{
    // Execute the query and get the status.
    PGresult *qryRes;

    wxLogSql(wxT("Set sub-query: %s"), sql.c_str());

    qryRes = PQexec(conn, sql.mb_str(conv));
    if (PQresultStatus(qryRes) != PGRES_TUPLES_OK) {
        return wxEmptyString;
    }

    // Retrieve the query result and return it.
    wxString result=wxString(PQgetvalue(qryRes, 0, 0), conv);
    wxLogInfo(wxT("Query result: %s"), result.c_str());

    // Cleanup & exit
    PQclear(qryRes);
    return result;
}


static void pgNoticeProcessor(void *arg, const char *message)
{
    wxString str(message, wxConvUTF8);
    
    wxLogNotice(wxT("%s"), str.c_str());
    ((pgQueryThread*)arg)->appendMessage(str);
}


pgQueryThread::pgQueryThread(pgConn *_conn, const wxString &qry, int _resultToRetrieve) 
: wxThread(wxTHREAD_JOINABLE)
{
    query = qry;
    conn=_conn;
    dataSet=0;
    result=0;
    resultToRetrieve=_resultToRetrieve;
    rc=-1;
    PQsetNoticeProcessor(conn->conn, pgNoticeProcessor, this);
    PQsetnonblocking(conn->conn, 1);
}


pgQueryThread::~pgQueryThread()
{
    if (dataSet)
        delete dataSet;
}


wxString pgQueryThread::GetMessagesAndClear()
{
    wxString msg;

    {
        wxCriticalSectionLocker cs(criticalSection);
        msg=messages;
        messages.Empty();
    }

    return msg;
}


void pgQueryThread::appendMessage(const wxString &str)
{
    wxCriticalSectionLocker cs(criticalSection);
    messages += str;
}


int pgQueryThread::execute()
{
    wxLongLong startTime=wxGetLocalTimeMillis();

    wxLogSql(wxT("Thread Query %s"), query.c_str());


    if (!PQsendQuery(conn->conn, query.mb_str(*conn->conv)))
        return(0);

    int resultsRetrieved=0;
    PGresult *lastResult=0;
    while (true)
    {
        if (TestDestroy())
        {
            if (rc != -3)
            {
                if (!PQrequestCancel(conn->conn)) // could not abort; abort failed.
                    return(-1);

                rc = -3;
            }
        }
        if (!PQconsumeInput(conn->conn))
            return(0);
        if (PQisBusy(conn->conn))
        {
            Yield();
            wxUsleep(10);
            continue;
        }

        // If resultToRetrieve is given, the nth result will be returned, 
        // otherwise the last result set will be returned.
        // all others are discarded
        PGresult *res=PQgetResult(conn->conn);

        startTime = wxGetLocalTimeMillis();
        if (!res)
            break;

        resultsRetrieved++;
        if (resultsRetrieved == resultToRetrieve)
        {
            result=res;
            appendMessage(wxString::Format(_("Query result with %d rows will be returned.\n"), PQntuples(result)));
            continue;
        }
        if (lastResult)
        {
            appendMessage(wxString::Format(_("Query result with %d rows discarded.\n"), PQntuples(lastResult)));
            PQclear(lastResult);
        }
        lastResult=res;
    }

    if (!result)
        result = lastResult;

    appendMessage(wxT("\n"));
    rc=PQresultStatus(result);

    if (rc == PGRES_TUPLES_OK)
    {
        dataSet = new pgSet(result, conn->conn, *conn->conv, conn->needColQuoting);
        dataSet->MoveFirst();
        dataSet->GetVal(0);
    }
    return(1);
}


bool pgQueryThread::IsRunning() const
{
    if (rc <  0)
        return wxThread::IsRunning();
    return false;
}


void *pgQueryThread::Entry()
{
    rc=-2;
    wxLogInfo(wxT("Running query %s"), query.c_str());

    execute();

    return(NULL);
}
