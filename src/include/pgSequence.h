//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgSequence.h PostgreSQL Sequence
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGSequence_H
#define PGSequence_H

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "pgObject.h"
#include "pgServer.h"
#include "pgDatabase.h"

class pgCollection;

class pgSequence : public pgSchemaObject
{
public:
    pgSequence(pgSchema *newSchema, const wxString& newName = wxT(""));
    ~pgSequence();

    int GetIcon() { return PGICON_SEQUENCE; }
    void ShowTreeDetail(wxTreeCtrl *browser, frmMain *form=0, ctlListView *properties=0, ctlSQLBox *sqlPane=0);
    void ShowStatistics(frmMain *form, ctlListView *statistics);
    static pgObject *ReadObjects(pgCollection *collection, wxTreeCtrl *browser, const wxString &restriction=wxT(""));

    wxString GetTablespace() const { return tablespace; };
    void iSetTablespace(const wxString& newVal) { tablespace = newVal; }
    void UpdateValues();
    wxULongLong GetLastValue() const { return lastValue; }
    wxULongLong GetMinValue() const { return minValue; }
    wxULongLong GetMaxValue() const { return maxValue; }
    wxULongLong GetCacheValue() const { return cacheValue; }
    wxULongLong GetIncrement() const { return increment; }
    bool GetCycled() const { return cycled; }

    bool DropObject(wxFrame *frame, wxTreeCtrl *browser);
    wxString GetSql(wxTreeCtrl *browser);
    pgObject *Refresh(wxTreeCtrl *browser, const wxTreeItemId item);

private:
    wxULongLong lastValue, minValue, maxValue, cacheValue, increment;
    bool cycled;
    wxString tablespace;
};

#endif
