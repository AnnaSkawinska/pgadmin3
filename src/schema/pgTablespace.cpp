//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgTablespace.cpp - PostgreSQL Tablespace
//
//////////////////////////////////////////////////////////////////////////


// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "misc.h"
#include "pgfeatures.h"
#include "pgObject.h"
#include "pgTablespace.h"
#include "pgCollection.h"
#include "frmMain.h"


pgTablespace::pgTablespace(const wxString& newName)
: pgServerObject(PG_TABLESPACE, newName)
{
    wxLogInfo(wxT("Creating a pgTablespace object"));
}

pgTablespace::~pgTablespace()
{
    wxLogInfo(wxT("Destroying a pgTablespace object"));
}


void pgTablespace::ShowReferencedBy(frmMain *form, ctlListView *referencedBy, const wxString &where)
{
    form->StartMsg(_(" Retrieving tablespace usage"));

    referencedBy->ClearAll();
    referencedBy->AddColumn(_("Type"), 60);
    referencedBy->AddColumn(_("Database"), 80);
    referencedBy->AddColumn(_("Name"), 300);

    wxArrayString dblist;

    pgSet *set=GetConnection()->ExecuteSet(
        wxT("SELECT datname, datallowconn, dattablespace\n")
        wxT("  FROM pg_database db\n")
        wxT(" ORDER BY datname"));

    if (set)
    {
        while (!set->Eof())
        {
            wxString datname=set->GetVal(wxT("datname"));
            if (set->GetBool(wxT("datallowconn")))
                dblist.Add(datname);
            OID oid=set->GetOid(wxT("dattablespace"));
            if (oid == GetOid())
                referencedBy->AppendItem(PGICON_DATABASE, _("Database"), datname);

            set->MoveNext();
        }
        delete set;
    }

    FillOwned(form->GetBrowser(), referencedBy, dblist, 
        wxT("SELECT cl.relkind, COALESCE(cin.nspname, cln.nspname) as nspname, COALESCE(ci.relname, cl.relname) as relname, cl.relname as indname\n")
        wxT("  FROM pg_class cl\n")
        wxT("  JOIN pg_namespace cln ON cl.relnamespace=cln.oid\n")
        wxT("  LEFT OUTER JOIN pg_index ind ON ind.indexrelid=cl.oid\n")
        wxT("  LEFT OUTER JOIN pg_class ci ON ind.indrelid=ci.oid\n")
        wxT("  LEFT OUTER JOIN pg_namespace cin ON ci.relnamespace=cin.oid,\n")
        wxT("       pg_database\n")
        wxT(" WHERE datname = current_database()\n")
        wxT("   AND (cl.reltablespace = ") + GetOidStr() + wxT("\n")
        wxT("        OR (cl.reltablespace=0 AND dattablespace = ") + GetOidStr() + wxT("))\n")
        wxT(" ORDER BY 1,2,3"));

    form->EndMsg();
}


bool pgTablespace::DropObject(wxFrame *frame, wxTreeCtrl *browser)
{
    return GetConnection()->ExecuteVoid(wxT("DROP TABLESPACE ") + GetQuotedFullIdentifier() + wxT(";"));
}


wxString pgTablespace::GetSql(wxTreeCtrl *browser)
{
    if (sql.IsNull())
    {
        sql = wxT("-- Tablespace: \"") + GetName() + wxT("\"\n\n");
        if (location.IsEmpty())
            sql += wxT("-- System Tablespace\n");
        else
            sql += wxT("\n\nCREATE TABLESPACE ") + GetQuotedIdentifier()
                +  wxT("\n  OWNER ") + qtIdent(GetOwner())
                +  wxT("\n  LOCATION ") + qtString(location)
                +  wxT(";\n");

    }
    return sql;
}


void pgTablespace::ShowTreeDetail(wxTreeCtrl *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
    if (!expandedKids)
    {
        expandedKids=true;

    }
    if (properties)
    {
        wxLogInfo(wxT("Displaying properties for Tablespace ") + GetIdentifier());

        CreateListColumns(properties);

        properties->AppendItem(_("Name"), GetName());
        properties->AppendItem(_("OID"), GetOid());
        properties->AppendItem(_("Owner"), GetOwner());
        properties->AppendItem(_("Location"), GetLocation());
        properties->AppendItem(_("ACL"), GetAcl());
    }
}


void pgTablespace::ShowStatistics(frmMain *form, ctlListView *statistics)
{
    if (statistics)
    {
        if (GetConnection()->HasFeature(FEATURE_SIZE))
        {
            wxLogInfo(wxT("Displaying statistics for %s"), GetTypeName().c_str());

        // Add the statistics view columns
            CreateListColumns(statistics, _("Statistic"), _("Value"));

            pgSet *stats = GetConnection()->ExecuteSet(
                wxT("SELECT pg_size_pretty(pg_tablespace_size(") + GetOidStr() + wxT(")) AS ") + qtIdent(_("Tablespace Size")));
    
            if (stats)
            {
                int col;
                for (col=0 ; col < stats->NumCols() ; col++)
                {
                    if (!stats->ColName(col).IsEmpty())
                        statistics->AppendItem(stats->ColName(col), stats->GetVal(col));
                }
                delete stats;
            }
        }
    }
}


pgObject *pgTablespace::Refresh(wxTreeCtrl *browser, const wxTreeItemId item)
{
    pgObject *tablespace=0;
    wxTreeItemId parentItem=browser->GetItemParent(item);
    if (parentItem)
    {
        pgObject *obj=(pgObject*)browser->GetItemData(parentItem);
        if (obj->GetType() == PG_TABLESPACES)
            tablespace = ReadObjects((pgCollection*)obj, 0, wxT("\n WHERE ts.oid=") + GetOidStr());
    }
    return tablespace;
}



pgObject *pgTablespace::ReadObjects(pgCollection *collection, wxTreeCtrl *browser, const wxString &restriction)
{
    pgTablespace *tablespace=0;

    wxString tabname;


    pgSet *tablespaces = collection->GetServer()->ExecuteSet(
        wxT("SELECT ts.oid, spcname, spclocation, pg_get_userbyid(spcowner) as spcuser, spcacl FROM pg_tablespace ts\n")
        + restriction + wxT(" ORDER BY spcname"));

    if (tablespaces)
    {
        while (!tablespaces->Eof())
        {

            tablespace = new pgTablespace(tablespaces->GetVal(wxT("spcname")));
            tablespace->iSetServer(collection->GetServer());
            tablespace->iSetOid(tablespaces->GetOid(wxT("oid")));
            tablespace->iSetOwner(tablespaces->GetVal(wxT("spcuser")));
            tablespace->iSetLocation(tablespaces->GetVal(wxT("spclocation")));
            tablespace->iSetAcl(tablespaces->GetVal(wxT("spcacl")));


            if (browser)
            {
                collection->AppendBrowserItem(browser, tablespace);
				tablespaces->MoveNext();
            }
            else
                break;
        }

		delete tablespaces;
    }
    return tablespace;
}


void pgTablespace::ShowStatistics(pgCollection *collection, ctlListView *statistics)
{
    if (collection->GetConnection()->HasFeature(FEATURE_SIZE))
    {
        wxLogInfo(wxT("Displaying statistics for tablespaces"));

        // Add the statistics view columns
        statistics->ClearAll();
        statistics->AddColumn(_("Tablespace"), 100);
        statistics->AddColumn(_("Size"), 60);

        pgSet *stats = collection->GetConnection()->ExecuteSet(
            wxT("SELECT spcname, pg_size_pretty(pg_tablespace_size(oid)) AS size FROM pg_tablespace ORDER BY spcname"));

        if (stats)
        {
            long pos=0;
            while (!stats->Eof())
            {
                statistics->InsertItem(pos, stats->GetVal(wxT("spcname")), PGICON_STATISTICS);
                statistics->SetItem(pos, 1, stats->GetVal(wxT("size")));
                stats->MoveNext();
                pos++;
            }

	        delete stats;
        }
    }
}

