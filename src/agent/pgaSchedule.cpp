//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgaSchedule.h - PostgreSQL Agent Schedule
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/arrimpl.cpp>

// App headers
#include "misc.h"
#include "pgObject.h"
#include "pgDatabase.h"
#include "pgCollection.h"
#include "pgaSchedule.h"
#include "pgaStep.h"
#include "pgaSchedule.h"


WX_DEFINE_OBJARRAY(wxArrayTimeSpan);


pgaSchedule::pgaSchedule(pgaJob *_job, const wxString& newName)
: pgaJobObject(_job, PGA_SCHEDULE, newName)
{
    wxLogInfo(wxT("Creating a pgaSchedule object"));
}

pgaSchedule::~pgaSchedule()
{
    wxLogInfo(wxT("Destroying a pgaSchedule object"));
}


void pgaSchedule::iSetIntervalList(const wxString &s)
{
    if (s[0] == '{')
        intervalListString = s.Mid(1, s.Length()-2);
    else
        intervalListString = s;

    wxStringTokenizer tk(intervalListString, wxT(","));

    while (tk.HasMoreTokens())
    {
        wxString str=tk.GetNextToken();

        int h, m, s;
        wxSscanf(str, wxT("%d:%d:%d"), &h, &m, &s);
        intervalList.Add(wxTimeSpan(h, m, s));
    }
}


bool pgaSchedule::DropObject(wxFrame *frame, wxTreeCtrl *browser)
{
    return GetDatabase()->ExecuteVoid(wxT("DELETE FROM pg_admin.pga_jobschedule WHERE oid=") + GetOidStr());
}


void pgaSchedule::ShowTreeDetail(wxTreeCtrl *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
    if (!expandedKids)
    {
        expandedKids=true;
    }

    if (properties)
    {
        CreateListColumns(properties);

        properties->AppendItem(_("Name"), GetName());
        properties->AppendItem(_("OID"), GetOid());
        properties->AppendItem(_("Enabled"), GetEnabled());
        properties->AppendItem(_("Type"), GetKind());
        if (kindChar == 'n' || kindChar == 's')
            properties->AppendItem(_("Scheduled"), GetSchedule());
        if (kindChar != 's')
        {
            wxTimeSpan ts=intervalList.Item(0);
            if (ts.GetDays() > 0)
                properties->AppendItem(_("Interval"), ts.Format(wxT("%D:%H:%M:%S")));
            else
                properties->AppendItem(_("Interval"), ts.Format(wxT("%H:%M:%S")));
        }
        properties->AppendItem(_("Start date"), GetStart());
        properties->AppendItem(_("End date"), GetEnd());

        properties->AppendItem(_("Comment"), GetComment());
    }
}



pgObject *pgaSchedule::Refresh(wxTreeCtrl *browser, const wxTreeItemId item)
{
    pgObject *schedule=0;
    wxTreeItemId parentItem=browser->GetItemParent(item);
    if (parentItem)
    {
        pgaJob *obj=(pgaJob*)browser->GetItemData(parentItem);
        if (obj->GetType() == PGA_JOB)
            schedule = ReadObjects(obj, 0);
    }
    return schedule;
}



pgObject *pgaSchedule::ReadObjects(pgaJob *job, wxTreeCtrl *browser, const wxString &restriction)
{
    pgaSchedule *schedule=0;

    pgSet *schedules= job->GetDatabase()->ExecuteSet(
       wxT("SELECT sc.oid, * FROM pg_admin.pga_schedule sc\n")
       wxT(" WHERE sc.jscjoboid=") + job->GetOidStr() + wxT("\n")
       + restriction +
       wxT(" ORDER BY sc.oid"));

    if (schedules)
    {
        while (!schedules->Eof())
        {

            schedule = new pgaSchedule(job, schedules->GetVal(wxT("jscname")));
            schedule->iSetOid(schedules->GetOid(wxT("oid")));
            schedule->iSetDatabase(job->GetDatabase());
            schedule->iSetStart(schedules->GetDateTime(wxT("jscstart")));
            schedule->iSetEnd(schedules->GetDateTime(wxT("jscend")));
            schedule->iSetSchedule(schedules->GetDateTime(wxT("jscsched")));
            schedule->iSetIntervalList(schedules->GetVal(wxT("jsclist")));

            wxChar kindc = *schedules->GetVal(wxT("jsckind")).c_str();
            wxString kinds;
            switch (kindc)
            {
                case 'n':   kinds = _("repeat");            break;
                case 's':   kinds = _("single");            break;
                case 'd':   kinds = _("daily") ;            break;
                case 'w':   kinds = _("weekly");            break;
                case 'm':   kinds = _("monthly");           break;
                case 'y':   kinds = _("yearly");            break;
            }
            schedule->iSetKind(kinds);
            schedule->iSetKindChar(kindc);

            schedule->iSetComment(schedules->GetVal(wxT("jscdesc")));


            if (browser)
            {
                job->AppendBrowserItem(browser, schedule);
				schedules->MoveNext();
            }
            else
                break;
        }

		delete schedules;
    }
    return schedule;
}
