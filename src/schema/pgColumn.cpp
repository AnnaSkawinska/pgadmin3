//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgColumn.cpp - Column class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "misc.h"
#include "pgDefs.h"
#include "pgDatatype.h"

#include "pgObject.h"
#include "pgColumn.h"
#include "pgCollection.h"


pgColumn::pgColumn(pgSchema *newSchema, const wxString& newName)
: pgSchemaObject(newSchema, PG_COLUMN, newName)
{
    isFK=false;
    isPK=false;
    isReferenced = -1;
}

pgColumn::~pgColumn()
{
}


bool pgColumn::IsReferenced()
{
    if (isReferenced < 0)
    {
        isReferenced = (int)StrToLong(GetConnection()->ExecuteScalar(
            wxT("SELECT COUNT(1) FROM pg_depend dep\n")
            wxT("  JOIN pg_class cl ON dep.classid=cl.oid AND relname='pg_rewrite'\n")
            wxT(" WHERE refobjid=") + GetTableOidStr()
            + wxT(" AND refobjsubid=") + NumToStr(GetColNumber())));
    }

    return (isReferenced != 0);
}


bool pgColumn::DropObject(wxFrame *frame, wxTreeCtrl *browser)
{
    wxString sql = wxT("ALTER TABLE ") + GetQuotedFullTable();
             sql += wxT(" DROP COLUMN ") + GetQuotedIdentifier();
    
    return GetDatabase()->ExecuteVoid(sql);
}


void pgColumn::ShowDependsOn(frmMain *form, ctlListView *dependsOn, const wxString &where)
{
    pgObject::ShowDependsOn(form, dependsOn, 
        wxT("\n WHERE dep.objid=") + NumToStr(tableOid) +
        wxT(" AND dep.objsubid=") + NumToStr(colNumber));
}


void pgColumn::ShowReferencedBy(frmMain *form, ctlListView *referencedBy, const wxString &where)
{
    pgObject::ShowReferencedBy(form, referencedBy, 
        wxT("\n WHERE dep.refobjid=") + NumToStr(tableOid) +
        wxT(" AND dep.refobjsubid=") + NumToStr(colNumber));
}

wxString pgColumn::GetSql(wxTreeCtrl *browser)
{
    if (sql.IsNull() && !GetSystemObject())
    {
        if (GetInheritedCount())
            sql = wxT("-- Column inherited; cannot be changed");
        else
        {
            sql = wxT("-- Column: ") + GetQuotedFullIdentifier() + wxT("\n\n")
                + wxT("-- ALTER TABLE ") + GetQuotedFullTable()
                + wxT(" DROP COLUMN ") + GetQuotedIdentifier()
                
                + wxT("\n\nALTER TABLE ") + GetQuotedFullTable()
                + wxT(" ADD COLUMN ") + GetQuotedIdentifier() + wxT(" ") + GetQuotedTypename()
                
                + wxT(";\nALTER TABLE ")+ GetQuotedFullTable()
                + wxT(" ALTER COLUMN ") + GetQuotedIdentifier()
                + wxT(" SET STORAGE ") + GetStorage() + wxT(";\n");

            if (GetNotNull())
                sql += wxT("ALTER TABLE ") + GetQuotedFullTable()
                    + wxT(" ALTER COLUMN ") + GetQuotedIdentifier()
                    + wxT(" SET NOT NULL;\n");
            if (!GetDefault().IsEmpty())
                sql += wxT("ALTER TABLE ") + GetQuotedFullTable()
                    + wxT(" ALTER COLUMN ") + GetQuotedIdentifier()
                    + wxT(" SET DEFAULT ") + GetDefault() + wxT(";\n");
            if (GetAttstattarget() >= 0)
                sql += wxT("ALTER TABLE ") + GetQuotedFullTable()
                    + wxT(" ALTER COLUMN ") + GetQuotedIdentifier()
                    + wxT(" SET STATISTICS ") + NumToStr(GetAttstattarget()) + wxT(";\n");

			sql += GetCommentSql();
        }
    }

    return sql;
}

wxString pgColumn::GetCommentSql()
{
	wxString commentSql;

	if (!GetComment().IsEmpty())
		commentSql = wxT("COMMENT ON COLUMN ") + GetQuotedFullTable() + wxT(".") + GetQuotedIdentifier()
        +  wxT(" IS ") + qtString(GetComment()) + wxT(";\n");
	
	return commentSql;
}

wxString pgColumn::GetDefinition()
{
    wxString sql = GetQuotedTypename();

    if ((sql == wxT("int4") || sql == wxT("int8") || 
         sql == wxT("pg_catalog.int4") || sql == wxT("pg_catalog.int8"))
        && GetDefault() == wxT("nextval('") 
                        + schema->GetName() + wxT(".") + GetTableName() 
                        + wxT("_") + GetName() + wxT("_seq'::text)"))
    {
        if (sql == wxT("int8"))
            sql = wxT("bigserial");
        else
            sql = wxT("serial");

        if (GetNotNull())
            sql += wxT(" NOT NULL");
    }
    else
    {
        if (GetNotNull())
            sql += wxT(" NOT NULL");
        AppendIfFilled(sql, wxT(" DEFAULT "), GetDefault());
    }
    return sql;
}


void pgColumn::ShowTreeDetail(wxTreeCtrl *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
    if (!expandedKids)
    {
        expandedKids = true;
        // append type here
        // fk, pk lesen
        pgSet *set = ExecuteSet(
            wxT("SELECT indkey FROM pg_index\n")
            wxT(" WHERE indrelid=") + GetTableOidStr() +
			wxT(" AND indisprimary"));
        if (set)
        {
            wxString indkey, str;
            while (!isPK && !set->Eof())
            {
                wxStringTokenizer indkey(set->GetVal(0));
                while (indkey.HasMoreTokens())
                {
                    str=indkey.GetNextToken();
                    if (StrToLong(str) == GetColNumber())
                    {
                        isPK = true;
                        break;
                    }
                }

                set->MoveNext();
            }
            delete set;
        }

        set=ExecuteSet(
            wxT("SELECT conkey\n")
            wxT("  FROM pg_constraint ct\n")
            wxT("  JOIN pg_class cl on cl.oid=confrelid\n")
            wxT(" WHERE contype='f' AND conrelid = ") + GetTableOidStr() + wxT("\n")
            wxT(" ORDER BY conname"));
        if (set)
        {
            wxString conkey, str;
            while (!isFK && !set->Eof())
            {
                wxStringTokenizer conkey(set->GetVal(0));

                while (conkey.HasMoreTokens())
                {
                    str=conkey.GetNextToken();
                    if (StrToLong(str.Mid(1)) == GetColNumber())
                    {
                        isFK = true;
                        break;
                    }
                }

                set->MoveNext();
            }
            delete set;
        }
    }

    if (properties)
    {
        CreateListColumns(properties);

        properties->AppendItem(_("Name"), GetName());
        properties->AppendItem(_("Position"), GetColNumber());
        properties->AppendItem(_("Data type"), GetVarTypename());
        properties->AppendItem(_("Default"), GetDefault());
        properties->AppendItem(_("Sequence"), database->GetSchemaPrefix(GetSerialSchema()) + GetSerialSequence());
        properties->AppendItem(_("Not NULL?"), GetNotNull());
        properties->AppendItem(_("Primary key?"), GetIsPK());
        properties->AppendItem(_("Foreign key?"), GetIsFK());
        properties->AppendItem(_("Storage"), GetStorage());
        properties->AppendItem(_("Inherited"), GetInheritedCount() != 0);
        properties->AppendItem(_("Statistics"), GetAttstattarget());


        properties->AppendItem(_("System column?"), GetSystemObject());
        properties->AppendItem(_("Comment"), GetComment());
    }
}


void pgColumn::ShowStatistics(frmMain *form, ctlListView *statistics)
{
    DisplayStatistics(statistics,
        wxT("SELECT null_frac AS ") + qtIdent(_("Null Fraction")) +
             wxT(", avg_width AS ") + qtIdent(_("Average Width")) +
             wxT(", n_distinct AS ") + qtIdent(_("Distinct Values")) +
             wxT(", most_common_vals AS ") + qtIdent(_("Most Common Values")) +
             wxT(", most_common_freqs AS ") + qtIdent(_("Most Common Frequencies")) +
             wxT(", histogram_bounds AS ") + qtIdent(_("Histogram Bounds")) +
             wxT(", correlation AS ") + qtIdent(_("Correlation")) + wxT("\n")
        wxT("  FROM pg_stats\n")
        wxT(" WHERE schemaname = ") + qtString(schema->GetName()) + wxT("\n")
        wxT("   AND tablename = ") + qtString(GetTableName()) + wxT("\n")
        wxT("   AND attname = ") + qtString(GetName()));
}


pgObject *pgColumn::Refresh(wxTreeCtrl *browser, const wxTreeItemId item)
{
    pgObject *column=0;
    wxTreeItemId parentItem=browser->GetItemParent(item);
    if (parentItem)
    {
        pgObject *obj=(pgObject*)browser->GetItemData(parentItem);
        if (obj->GetType() == PG_COLUMNS)
            column = ReadObjects((pgCollection*)obj, 0, wxT("\n   AND attnum=") + NumToStr(GetColNumber()));
    }
    return column;
}



pgObject *pgColumn::ReadObjects(pgCollection *collection, wxTreeCtrl *browser, const wxString &restriction)
{
    pgColumn *column=0;
    pgDatabase *database=collection->GetDatabase();

    wxString systemRestriction;
    if (!settings->GetShowSystemObjects())
        systemRestriction = wxT("\n   AND attnum > 0");
        
    pgSet *columns= database->ExecuteSet(
        wxT("SELECT att.*, def.*, CASE WHEN attndims > 0 THEN 1 ELSE 0 END AS isarray, CASE WHEN ty.typname = 'bpchar' THEN 'char' WHEN ty.typname = '_bpchar' THEN '_char' ELSE ty.typname END AS typname, tn.nspname as typnspname, et.typname as elemtypname,\n")
        wxT("  cl.relname, na.nspname, att.attstattarget, description, cs.relname AS sername, ns.nspname AS serschema,\n")
        wxT("  (SELECT count(1) FROM pg_type t2 WHERE t2.typname=ty.typname) > 1 AS isdup\n")
        wxT("  FROM pg_attribute att\n")
        wxT("  JOIN pg_type ty ON ty.oid=atttypid\n")
        wxT("  JOIN pg_namespace tn ON tn.oid=ty.typnamespace\n")
        wxT("  JOIN pg_class cl ON cl.oid=attrelid\n")
        wxT("  JOIN pg_namespace na ON na.oid=cl.relnamespace\n")
        wxT("  LEFT OUTER JOIN pg_type et ON et.oid=ty.typelem\n")
        wxT("  LEFT OUTER JOIN pg_attrdef def ON adrelid=attrelid AND adnum=attnum\n")
        wxT("  LEFT OUTER JOIN pg_description des ON des.objoid=attrelid AND des.objsubid=attnum\n")
        wxT("  LEFT OUTER JOIN (pg_depend JOIN pg_class cs ON objid=cs.oid AND cs.relkind='S') ON refobjid=attrelid AND refobjsubid=attnum\n")
        wxT("  LEFT OUTER JOIN pg_namespace ns ON ns.oid=cs.relnamespace\n")
        wxT(" WHERE attrelid = ") + collection->GetOidStr()
        + restriction + systemRestriction + wxT("\n")
        wxT("   AND attisdropped IS FALSE\n")
        wxT(" ORDER BY attnum"));

    if (columns)
    {
        while (!columns->Eof())
        {
            column = new pgColumn(collection->GetSchema(), columns->GetVal(wxT("attname")));

            column->iSetTableOid(collection->GetOid());
            column->iSetAttTypId(columns->GetOid(wxT("atttypid")));
            column->iSetColNumber(columns->GetLong(wxT("attnum")));
            column->iSetIsArray(columns->GetBool(wxT("isarray")));
            column->iSetComment(columns->GetVal(wxT("description")));
            column->iSetSerialSequence(columns->GetVal(wxT("sername")));
            column->iSetSerialSchema(columns->GetVal(wxT("serschema")));

            if (columns->GetBool(wxT("atthasdef")))
                column->iSetDefault(columns->GetVal(wxT("adsrc")));
            column->iSetStatistics(columns->GetLong(wxT("attstattarget")));

            wxString storage=columns->GetVal(wxT("attstorage"));
            column->iSetStorage(
                storage == wxT("p") ? wxT("PLAIN") :
                storage == wxT("e") ? wxT("EXTERNAL") :
                storage == wxT("m") ? wxT("MAIN") :
                storage == wxT("x") ? wxT("EXTENDED") : wxT("Unknown"));

            column->iSetTyplen(columns->GetLong(wxT("attlen")));

            long typmod=columns->GetLong(wxT("atttypmod"));
            pgDatatype dt(columns->GetVal(wxT("typnspname")), columns->GetVal(wxT("typname")), 
                columns->GetBool(wxT("isdup")),
                columns->GetBool(wxT("isarray"))? 1 : 0, typmod);


            column->iSetTypmod(typmod);
            column->iSetLength(dt.Length());
            column->iSetPrecision(dt.Precision());
            column->iSetRawTypename(dt.Name());

            column->iSetVarTypename(dt.GetSchemaPrefix(database) + dt.FullName());
            column->iSetQuotedTypename(dt.GetQuotedSchemaPrefix(database) + dt.QuotedFullName());

            column->iSetNotNull(columns->GetBool(wxT("attnotnull")));
            column->iSetQuotedFullTable(database->GetQuotedSchemaPrefix(columns->GetVal(wxT("nspname")))
                + qtIdent(columns->GetVal(wxT("relname"))));
            column->iSetTableName(columns->GetVal(wxT("relname")));
            column->iSetInheritedCount(columns->GetLong(wxT("attinhcount")));
            column->iSetAttstattarget(columns->GetLong(wxT("attstattarget")));

            if (browser)
            {
                collection->AppendBrowserItem(browser, column);
				columns->MoveNext();
            }
            else
                break;
        }

		delete columns;
    }
    return column;
}
