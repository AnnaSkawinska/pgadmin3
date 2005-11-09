//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgSet.h - PostgreSQL ResultSet class
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGSET_H
#define PGSET_H

// wxWindows headers
#include <wx/wx.h>

// PostgreSQL headers
#include <libpq-fe.h>

// App headers
#include "pgAdmin3.h"
#include "misc.h"


typedef enum
{
    PGTYPCLASS_NUMERIC,
    PGTYPCLASS_BOOL,
    PGTYPCLASS_STRING,
    PGTYPCLASS_DATE,
    PGTYPCLASS_OTHER
} pgTypClass;


class pgConn;

// Class declarations
class pgSet
{
public:
    pgSet(PGresult *newRes, pgConn *newConn, wxMBConv &cnv, bool needColQt);
    ~pgSet();
    long NumRows() const { return nRows; }
    long NumCols() const { return PQnfields(res); }

    void MoveNext() { if (pos <= nRows) pos++; }
    void MovePrevious() { if (pos > 0) pos--; }
    void MoveFirst() { if (nRows) pos=1; else pos=0; }
    void MoveLast() { pos=nRows; }
    void Locate(long l) { pos=l; }
    long CurrentPos() const { return pos; }
    bool Bof() const { return (!nRows || pos < 1); }
    bool Eof() const { return (!nRows || pos > nRows); }
    wxString ColName(int col) const;
    OID ColTypeOid(int col) const;
    wxString ColType(int col) const;
    pgTypClass ColTypClass(int col) const;

    OID GetInsertedOid() const { return PQoidValue(res); }
    long GetInsertedCount() const;
    int ColSize(int col) const { return PQfsize(res, col); }
    bool IsNull(int col) const { return (PQgetisnull(res, pos-1, col) != 0); }
    int ColScale(int col) const;
    int ColNumber(const wxString &colName) const;


    wxString GetVal(const int col) const;
    wxString GetVal(const wxString& col) const;
    long GetLong(const int col) const;
    long GetLong(const wxString &col);
    bool GetBool(const int col) const;
    bool GetBool(const wxString &col) const;
    double GetDouble(const int col) const;
    double GetDouble(const wxString &col) const;
    wxDateTime GetDateTime(const int col) const;
    wxDateTime GetDateTime(const wxString &col) const;
    wxULongLong GetLongLong(const int col) const;
    wxULongLong GetLongLong(const wxString &col) const;
    OID GetOid(const int col) const;
    OID GetOid(const wxString &col) const;

    char *GetCharPtr(const int col) const;
    char *GetCharPtr(const wxString &col) const;

    wxMBConv &GetConversion() const { return conv; }


private:
    pgConn *conn;
    PGresult *res;
    long pos, nRows;
    wxString ExecuteScalar(const wxString& sql) const;
    wxMBConv &conv;
    bool needColQuoting;
};



class pgQueryThread : public wxThread
{
public:
    pgQueryThread(pgConn *_conn, const wxString &qry, int resultToRetrieve=-1);
    ~pgQueryThread();

    virtual void *Entry();
    bool DataValid() const { return dataSet != NULL; }
    pgSet *DataSet() { return dataSet; }
    int ReturnCode() const { return rc; }
    long RowsInserted() const { return rowsInserted; }
    OID InsertedOid() const { return insertedOid; }
    wxString GetMessagesAndClear();
    bool IsRunning() const;
    void appendMessage(const wxString &str);

private:
    int rc;
    int resultToRetrieve;
    long rowsInserted;
    OID insertedOid;

    wxString query;
    pgConn *conn;
    PGresult *result;
    wxString messages;
    pgSet *dataSet;
    wxCriticalSection criticalSection;

    int execute();
};

#endif

