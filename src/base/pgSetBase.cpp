//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2006, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgSetBase.cpp - PostgreSQL ResultSet class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// PostgreSQL headers
#include <libpq-fe.h>


// App headers
#include "base/pgSetBase.h"
#include "base/pgConnBase.h"
#include "base/sysLogger.h"
#include "base/pgDefs.h"

pgSetBase::pgSetBase(PGresult *newRes, pgConnBase *newConn, wxMBConv &cnv, bool needColQt)
: conv(cnv)
{
    needColQuoting = needColQt;

    wxLogInfo(wxT("Creating pgSetBase object"));
    conn = newConn;
    res = newRes;

    // Make sure we have tuples
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        nCols = 0;
        nRows = 0;
        pos = 0;
    }
    else
    {
        nCols = PQnfields(res);
        for (int x = 0; x < nCols+1; x++)
        {
            colTypes.Add(wxT(""));
            colFullTypes.Add(wxT(""));
        }

        nRows = PQntuples(res);
        MoveFirst();
    }
}


pgSetBase::~pgSetBase()
{
    wxLogInfo(wxT("Destroying pgSetBase object"));
    PQclear(res);
}



OID pgSetBase::ColTypeOid(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    return PQftype(res, col);
}

long pgSetBase::ColTypeMod(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    return PQfmod(res, col);
}


long pgSetBase::GetInsertedCount() const
{
    char *cnt=PQcmdTuples(res);
    if (!*cnt)
        return -1;
    else
        return atol(cnt);
}


pgTypClass pgSetBase::ColTypClass(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

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


wxString pgSetBase::ColType(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    if (!colTypes[col].IsEmpty())
        return colTypes[col];

    wxString szSQL, szResult;
    szSQL.Printf(wxT("SELECT format_type(oid,NULL) as typname FROM pg_type WHERE oid = %d"), ColTypeOid(col));
    szResult = ExecuteScalar(szSQL);
    colTypes[col] = szResult;

    return szResult;
}

wxString pgSetBase::ColFullType(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    if (!colFullTypes[col].IsEmpty())
        return colFullTypes[col];

    wxString szSQL, szResult;
    szSQL.Printf(wxT("SELECT format_type(oid,%d) as typname FROM pg_type WHERE oid = %d"), ColTypeMod(col), ColTypeOid(col));
    szResult = ExecuteScalar(szSQL);
    colFullTypes[col] = szResult;

    return szResult;
}

int pgSetBase::ColScale(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    // TODO
    return 0;
}
wxString pgSetBase::ColName(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    return wxString(PQfname(res, col), conv);
}


int pgSetBase::ColNumber(const wxString &colname) const
{
    int col;
    
    if (needColQuoting)
    {
        col = PQfnumber(res, (wxT("\"") + colname + wxT("\"")).mb_str(conv));
    }
    else
        col = PQfnumber(res, colname.mb_str(conv));

    if (col < 0)
        wxLogError(__("Column not found in pgSetBase: ") + colname);
    return col;
}



char *pgSetBase::GetCharPtr(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    return PQgetvalue(res, pos -1, col);
}


char *pgSetBase::GetCharPtr(const wxString &col) const
{
    return PQgetvalue(res, pos -1, ColNumber(col));
}


wxString pgSetBase::GetVal(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    return wxString(GetCharPtr(col), conv);
}


wxString pgSetBase::GetVal(const wxString& colname) const
{
    return GetVal(ColNumber(colname));
}


long pgSetBase::GetLong(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    char *c=PQgetvalue(res, pos-1, col);
    if (c)
        return atol(c);
    else
        return 0;
}


long pgSetBase::GetLong(const wxString &col) const
{
    char *c=PQgetvalue(res, pos-1, ColNumber(col));
    if (c)
        return atol(c);
    else
        return 0;
}


bool pgSetBase::GetBool(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    char *c=PQgetvalue(res, pos-1, col);
    if (c)
    {
        if (*c == 't' || *c == '1' || !strcmp(c, "on"))
            return true;
    }
    return false;
}


bool pgSetBase::GetBool(const wxString &col) const
{
    return GetBool(ColNumber(col));
}


wxDateTime pgSetBase::GetDateTime(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    wxDateTime dt;
    wxString str=GetVal(col);
    /* This hasn't just been used. ( Is not infinity ) */
    if (!str.IsEmpty())
        dt.ParseDateTime(str);
    return dt;
}


wxDateTime pgSetBase::GetDateTime(const wxString &col) const
{
    return GetDateTime(ColNumber(col));
}


wxDateTime pgSetBase::GetDate(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    wxDateTime dt;
    wxString str=GetVal(col);
    /* This hasn't just been used. ( Is not infinity ) */
    if (!str.IsEmpty())
        dt.ParseDate(str);
    return dt;
}


wxDateTime pgSetBase::GetDate(const wxString &col) const
{
    return GetDate(ColNumber(col));
}


double pgSetBase::GetDouble(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    return StrToDouble(GetVal(col));
}


double pgSetBase::GetDouble(const wxString &col) const
{
    return GetDouble(ColNumber(col));
}


wxULongLong pgSetBase::GetLongLong(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    char *c=PQgetvalue(res, pos-1, col);
    if (c)
        return atolonglong(c);
    else
        return 0;
}

wxULongLong pgSetBase::GetLongLong(const wxString &col) const
{
    return GetLongLong(ColNumber(col));
}


OID pgSetBase::GetOid(const int col) const
{
    wxASSERT(col < nCols && col >= 0);

    char *c=PQgetvalue(res, pos-1, col);
    if (c)
        return (OID)strtoul(c, 0, 10);
    else
        return 0;
}


OID pgSetBase::GetOid(const wxString &col) const
{
    return GetOid(ColNumber(col));
}


wxString pgSetBase::ExecuteScalar(const wxString& sql) const
{
    return conn->ExecuteScalar(sql);
}



static void pgNoticeProcessor(void *arg, const char *message)
{
    wxString str(message, wxConvUTF8);
    
    wxLogNotice(wxT("%s"), str.c_str());
    ((pgQueryThreadBase*)arg)->appendMessage(str);
}




//////////////////////////////////////////////////////////////////

pgSetIterator::pgSetIterator(pgConnBase *conn, const wxString &qry)
{
    set=conn->ExecuteSet(qry);
    first=true;
}


pgSetIterator::pgSetIterator(pgSetBase *s)
{
    set=s;
    first=true;
}


pgSetIterator::~pgSetIterator()
{
    if (set)
        delete set;
}


bool pgSetIterator::RowsLeft()
{
    if (!set)
        return false;

    if (first)
    {
        if (!set->NumRows())
            return false;
        first=false;
    }
    else
        set->MoveNext();

    return !set->Eof();
}


bool pgSetIterator::MovePrev()
{
    if (!set)
        return false;

    set->MovePrevious();
    return true;
}


////////////////////////////////////////////////////

pgQueryThreadBase::pgQueryThreadBase(pgConnBase *_conn, const wxString &qry, int _resultToRetrieve) 
: wxThread(wxTHREAD_JOINABLE)
{
    query = qry;
    conn=_conn;
    dataSet=0;
    result=0;
    resultToRetrieve=_resultToRetrieve;
    rc=-1;
    insertedOid = (OID)-1;

    wxLogSql(wxT("Thread Query %s"), qry.c_str());

    conn->RegisterNoticeProcessor(pgNoticeProcessor, this);
    if (conn->conn)
        PQsetnonblocking(conn->conn, 1);
}


pgQueryThreadBase::~pgQueryThreadBase()
{
    conn->RegisterNoticeProcessor(0, 0);
    if (dataSet)
        delete dataSet;
}


wxString pgQueryThreadBase::GetMessagesAndClear()
{
    wxString msg;

    {
        wxCriticalSectionLocker cs(criticalSection);
        msg=messages;
        messages.Empty();
    }

    return msg;
}


void pgQueryThreadBase::appendMessage(const wxString &str)
{
    wxCriticalSectionLocker cs(criticalSection);
    messages.Append(str);
}


int pgQueryThreadBase::execute()
{
    rowsInserted = -1L;

    if (!conn->conn)
        return(0);

    if (!PQsendQuery(conn->conn, query.mb_str(*conn->conv)))
    {
        conn->IsAlive();
        return(0);
    }
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
            wxMilliSleep(10);
            continue;
        }

        // If resultToRetrieve is given, the nth result will be returned, 
        // otherwise the last result set will be returned.
        // all others are discarded
        PGresult *res=PQgetResult(conn->conn);

        if (!res)
            break;

        resultsRetrieved++;
        if (resultsRetrieved == resultToRetrieve)
        {
            result=res;
            insertedOid=PQoidValue(res);
            if (insertedOid && insertedOid != (OID)-1)
                appendMessage(wxString::Format(_("Query inserted one row with OID %d.\n"), insertedOid));
			else
                appendMessage(wxString::Format(_("Query result with %d rows will be returned.\n"), PQntuples(result)));
            continue;
        }
        if (lastResult)
        {
            if (PQntuples(lastResult))
                appendMessage(wxString::Format(_("Query result with %d rows discarded.\n"), PQntuples(lastResult)));
            PQclear(lastResult);
        }
        lastResult=res;
    }

    if (!result)
        result = lastResult;

    conn->SetLastResultError(result);

    appendMessage(wxT("\n"));
    rc=PQresultStatus(result);
    insertedOid=PQoidValue(result);
    if (insertedOid == (OID)-1)
        insertedOid=0;

    if (rc == PGRES_TUPLES_OK)
    {
        dataSet = new pgSetBase(result, conn, *conn->conv, conn->needColQuoting);
        dataSet->MoveFirst();
        dataSet->GetVal(0);
    }
    else if (rc == PGRES_COMMAND_OK)
    {
        char *s=PQcmdTuples(result);
        if (*s)
            rowsInserted = atol(s);
    }

    return(1);
}

void *pgQueryThreadBase::Entry()
{
    rc=-2;
    execute();

    return(NULL);
}
