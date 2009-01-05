//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2009, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgFunction.cpp - function class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "frm/menu.h"
#include "utils/pgfeatures.h"
#include "utils/misc.h"
#include "schema/pgFunction.h"
#include "frm/frmReport.h"
#include "frm/frmHint.h"

pgFunction::pgFunction(pgSchema *newSchema, const wxString& newName)
: pgSchemaObject(newSchema, functionFactory, newName)
{
}


pgFunction::pgFunction(pgSchema *newSchema, pgaFactory &factory, const wxString& newName)
: pgSchemaObject(newSchema, factory, newName)
{
}

pgTriggerFunction::pgTriggerFunction(pgSchema *newSchema, const wxString& newName)
: pgFunction(newSchema, triggerFunctionFactory, newName)
{
}

pgProcedure::pgProcedure(pgSchema *newSchema, const wxString& newName)
: pgFunction(newSchema, procedureFactory, newName)
{
}

bool pgFunction::IsUpToDate()
{
    wxString sql = wxT("SELECT xmin FROM pg_proc WHERE oid = ") + this->GetOidStr();
	if (!this->GetDatabase()->GetConnection() || this->GetDatabase()->ExecuteScalar(sql) != NumToStr(GetXid()))
		return false;
	else
		return true;
}

bool pgFunction::DropObject(wxFrame *frame, ctlTree *browser, bool cascaded)
{
    wxString sql=wxT("DROP FUNCTION ")  + this->GetSchema()->GetQuotedIdentifier() + wxT(".") + this->GetQuotedIdentifier() + wxT("(") + GetArgSigList() + wxT(")");
    if (cascaded)
        sql += wxT(" CASCADE");
    return GetDatabase()->ExecuteVoid(sql);
}

wxString pgFunction::GetFullName()
{ 
    return GetName() + wxT("(") + GetArgSigList() + wxT(")");
}

wxString pgProcedure::GetFullName()
{ 
    if (GetArgSigList().IsEmpty())
        return GetName();
    else
        return GetName() + wxT("(") + GetArgSigList() + wxT(")");
}


wxString pgFunction::GetSql(ctlTree *browser)
{
    if (sql.IsNull())
    {
        wxString qtName = GetQuotedFullIdentifier()  + wxT("(") + GetArgListWithNames() + wxT(")");
        wxString qtSig = GetQuotedFullIdentifier()  + wxT("(") + GetArgSigList() + wxT(")");

        sql = wxT("-- Function: ") + qtSig + wxT("\n\n")
            + wxT("-- DROP FUNCTION ") + qtSig + wxT(";")
            + wxT("\n\nCREATE OR REPLACE FUNCTION ") + qtName;

        // Use Oracle style syntax for edb-spl functions
        if (GetLanguage() == wxT("edbspl"))
        {
            sql += wxT("\nRETURN ");
            sql += GetReturnType();

            sql += wxT(" AS");
            if (GetSource().StartsWith(wxT("\n")))
                sql += GetSource(); 
            else
                sql += wxT("\n") + GetSource();
        }
        else
        {
            sql += wxT("\n  RETURNS ");
            if (GetReturnAsSet())
                sql += wxT("SETOF ");
            sql += GetReturnType();

            sql += wxT(" AS\n");
            
            if (GetLanguage().IsSameAs(wxT("C"), false))
            {
                sql += qtDbString(GetBin()) + wxT(", ") + qtDbString(GetSource());
            }
            else
            {
                if (GetConnection()->BackendMinimumVersion(7, 5))
                    sql += qtDbStringDollar(GetSource());
                else
                    sql += qtDbString(GetSource());
            }
            sql += wxT("\n  LANGUAGE '") + GetLanguage() + wxT("' ") + GetVolatility();

            if (GetIsStrict())
                sql += wxT(" STRICT");
            if (GetSecureDefiner())
                sql += wxT(" SECURITY DEFINER");

            // PostgreSQL 8.3+ cost/row estimations
            if (GetConnection()->BackendMinimumVersion(8, 3))
            {
                sql += wxT("\n  COST ") + NumToStr(GetCost());

                if (GetReturnAsSet())
                    sql += wxT("\n  ROWS ") + NumToStr(GetRows());
            }
        }

        if (!sql.Strip(wxString::both).EndsWith(wxT(";")))
            sql += wxT(";");

        size_t i;
        for (i=0 ; i < configList.GetCount() ; i++)
            sql += wxT("\nALTER FUNCTION ") + qtSig
                +  wxT(" SET ") + configList.Item(i) + wxT(";");

        sql += wxT("\n")
            +  GetOwnerSql(8, 0, wxT("FUNCTION ") + qtSig)
            +  GetGrant(wxT("X"), wxT("FUNCTION ") + qtSig);

        if (!GetComment().IsNull())
        {
            sql += wxT("COMMENT ON FUNCTION ") + qtSig
                + wxT(" IS ") + qtDbString(GetComment()) + wxT(";\n");
        }
    }

    return sql;
}


void pgFunction::ShowTreeDetail(ctlTree *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
    if (properties)
    {
        CreateListColumns(properties);

        properties->AppendItem(_("Name"), GetName());
        properties->AppendItem(_("OID"), GetOid());
        properties->AppendItem(_("Owner"), GetOwner());
        properties->AppendItem(_("Argument count"), GetArgCount());
        properties->AppendItem(_("Arguments"), GetArgListWithNames());
        properties->AppendItem(_("Signature arguments"), GetArgSigList());
        if (!GetIsProcedure())
            properties->AppendItem(_("Return type"), GetReturnType());
        properties->AppendItem(_("Language"), GetLanguage());
        properties->AppendItem(_("Returns a set?"), GetReturnAsSet());
        if (GetLanguage().IsSameAs(wxT("C"), false))
        {
            properties->AppendItem(_("Object file"), GetBin());
            properties->AppendItem(_("Link symbol"), GetSource());
        }
        else
            properties->AppendItem(_("Source"), firstLineOnly(GetSource()));

        if (GetConnection()->BackendMinimumVersion(8, 3))
        {
            properties->AppendItem(_("Estimated cost"), GetCost());
            if (GetReturnAsSet())
                properties->AppendItem(_("Estimated rows"), GetRows());
        }

        properties->AppendItem(_("Volatility"), GetVolatility());
        properties->AppendItem(_("Security of definer?"), GetSecureDefiner());
        properties->AppendItem(_("Strict?"), GetIsStrict());

        size_t i;
        for (i=0 ; i < configList.GetCount() ; i++)
        {
            wxString item=configList.Item(i);
            properties->AppendItem(item.BeforeFirst('='), item.AfterFirst('='));
        }

        properties->AppendItem(_("ACL"), GetAcl());
        properties->AppendItem(_("System function?"), GetSystemObject());
        properties->AppendItem(_("Comment"), firstLineOnly(GetComment()));
    }
}

void pgFunction::ShowHint(frmMain *form, bool force)
{
	wxArrayString hints;
	hints.Add(HINT_OBJECT_EDITING);
    frmHint::ShowHint((wxWindow *)form, hints, GetFullIdentifier(), force);
}

wxString pgProcedure::GetSql(ctlTree *browser)
{
    if (!GetConnection()->EdbMinimumVersion(8, 0))
        return pgFunction::GetSql(browser);

    if (sql.IsNull())
    {
        wxString qtName, qtSig;

        if (GetArgListWithNames().IsEmpty())
        {
            qtName = GetQuotedFullIdentifier();
            qtSig = GetQuotedFullIdentifier();
        }
        else
        {
            qtName = GetQuotedFullIdentifier() + wxT("(") + GetArgListWithNames() + wxT(")");
            qtSig = GetQuotedFullIdentifier()  + wxT("(") + GetArgSigList() + wxT(")");
        }

        sql = wxT("-- Procedure: ") + qtSig + wxT("\n\n")
            + wxT("-- DROP PROCEDURE ") + qtSig + wxT(";")
            + wxT("\n\nCREATE OR REPLACE PROCEDURE ") + qtName;


        sql += wxT(" AS")
            + GetSource()
            + wxT("\n\n")
            + GetGrant(wxT("X"), wxT("PROCEDURE ") + qtSig);
    }

    return sql;
}


bool pgProcedure::DropObject(wxFrame *frame, ctlTree *browser, bool cascaded)
{
    if (!GetConnection()->EdbMinimumVersion(8, 0))
        return pgFunction::DropObject(frame, browser, cascaded);

    wxString sql=wxT("DROP PROCEDURE ") + this->GetSchema()->GetQuotedIdentifier() + wxT(".") + this->GetQuotedIdentifier();
    return GetDatabase()->ExecuteVoid(sql);
}

wxString pgFunction::GetArgListWithNames()
{
    wxString args;

    for (unsigned int i=0; i < argTypesArray.Count(); i++)
    {
        if (args.Length() > 0)
            args += wxT(", ");

        wxString arg;

        if (GetIsProcedure())
        {
            if (!argNamesArray.Item(i).IsEmpty())
                arg += qtIdent(argNamesArray.Item(i));

            if (!argModesArray.Item(i).IsEmpty())
                if (arg.IsEmpty())
                    arg += argModesArray.Item(i);
                else
                    arg += wxT(" ") + argModesArray.Item(i);
        }
        else
        {
            if (!argModesArray.Item(i).IsEmpty())
                arg += argModesArray.Item(i);

            if (!argNamesArray.Item(i).IsEmpty())
                if (arg.IsEmpty())
                    arg += qtIdent(argNamesArray.Item(i));
                else
                    arg += wxT(" ") + qtIdent(argNamesArray.Item(i));
        }

        if (!arg.IsEmpty())
            arg += wxT(" ") + argTypesArray.Item(i);
        else
            arg += argTypesArray.Item(i);

        // Parameter default value
        if (GetConnection()->HasFeature(FEATURE_FUNCTION_DEFAULTS))
        {
            if (!argDefsArray.Item(i).IsEmpty())
                arg += wxT(" DEFAULT ") + argDefsArray.Item(i);
        }

        args += arg;
    }
    return args;
}

wxString pgFunction::GetArgSigList()
{
    wxString args;

    for (unsigned int i=0; i < argTypesArray.Count(); i++)
    {
        // OUT parameters are not considered part of the signature, except for EDB-SPL
        if (argModesArray.Item(i) != wxT("OUT"))
        {
            if (args.Length() > 0)
                args += wxT(", ");

            args += argTypesArray.Item(i);
        }
        else
        {
            if (GetLanguage() == wxT("edbspl"))
            {
                if (args.Length() > 0)
                    args += wxT(", ");

                args += argTypesArray.Item(i);
            }
        }
    }
    return args;
}

pgFunction *pgFunctionFactory::AppendFunctions(pgObject *obj, pgSchema *schema, ctlTree *browser, const wxString &restriction)
{
    // Caches
    cacheMap typeCache, exprCache;

    pgFunction *function=0;
    wxString argNamesCol, argDefsCol, proConfigCol;
    if (obj->GetConnection()->BackendMinimumVersion(8, 0))
        argNamesCol = wxT("proargnames, ");
    if (obj->GetConnection()->HasFeature(FEATURE_FUNCTION_DEFAULTS))
        argDefsCol = wxT("proargdefvals, ");
    if (obj->GetConnection()->BackendMinimumVersion(8, 3))
        proConfigCol = wxT("proconfig, ");

    pgSet *functions = obj->GetDatabase()->ExecuteSet(
            wxT("SELECT pr.oid, pr.xmin, pr.*, format_type(TYP.oid, NULL) AS typname, typns.nspname AS typnsp, lanname, ") + argNamesCol  + argDefsCol + proConfigCol + 
            wxT("       pg_get_userbyid(proowner) as funcowner, description\n")
            wxT("  FROM pg_proc pr\n")
            wxT("  JOIN pg_type typ ON typ.oid=prorettype\n")
            wxT("  JOIN pg_namespace typns ON typns.oid=typ.typnamespace\n")
            wxT("  JOIN pg_language lng ON lng.oid=prolang\n")
            wxT("  LEFT OUTER JOIN pg_description des ON des.objoid=pr.oid\n")
            + restriction +
            wxT(" ORDER BY proname"));

    pgSet *types = obj->GetDatabase()->ExecuteSet(wxT(
                    "SELECT oid, format_type(oid, NULL) AS typname FROM pg_type"));

	if (types)
	{
        while(!types->Eof())
        {
            typeCache[types->GetVal(wxT("oid"))] = types->GetVal(wxT("typname"));
            types->MoveNext();
        }
	}

    if (functions)
    {
        while (!functions->Eof())
        {
            bool isProcedure=false;
            wxString lanname=functions->GetVal(wxT("lanname"));
            wxString typname=functions->GetVal(wxT("typname"));

            // Is this an EDB Stored Procedure?
            if (obj->GetConnection()->EdbMinimumVersion(8, 0) && lanname == wxT("edbspl") && typname == wxT("void"))
                isProcedure = true;

            // Create the new object
            if (isProcedure)
                function = new pgProcedure(schema, functions->GetVal(wxT("proname")));
            else if (typname == wxT("\"trigger\""))
                function = new pgTriggerFunction(schema, functions->GetVal(wxT("proname")));
            else
                function = new pgFunction(schema, functions->GetVal(wxT("proname")));

            
            // Tokenize the arguments
            wxStringTokenizer argTypesTkz(wxEmptyString), argModesTkz(wxEmptyString);
            queryTokenizer argNamesTkz(wxEmptyString, (wxChar)','),  argDefsTkz(wxEmptyString, (wxChar)',');
            wxString tmp;

            // We always have types
            argTypesTkz.SetString(functions->GetVal(wxT("proargtypes")));

            // We only have names in 8.0+
            if (obj->GetConnection()->BackendMinimumVersion(8, 0))
            {
                tmp = functions->GetVal(wxT("proargnames"));
                if (!tmp.IsEmpty())
                    argNamesTkz.SetString(tmp.Mid(1, tmp.Length()-2), wxT(","));
            }

            // EDB 8.0 had modes in pg_proc.proargdirs
            if (!obj->GetConnection()->EdbMinimumVersion(8, 1) && isProcedure)
                argModesTkz.SetString(functions->GetVal(wxT("proargdirs")));

            // EDB 8.1 and PostgreSQL 8.1 have modes in pg_proc.proargmodes
            if (obj->GetConnection()->BackendMinimumVersion(8, 1))
            {
                tmp = functions->GetVal(wxT("proallargtypes"));
                if (!tmp.IsEmpty())
                    argTypesTkz.SetString(tmp.Mid(1, tmp.Length()-2), wxT(","));

                tmp = functions->GetVal(wxT("proargmodes"));
                if (!tmp.IsEmpty())
                    argModesTkz.SetString(tmp.Mid(1, tmp.Length()-2), wxT(","));
            }

            // Function defaults
            if (obj->GetConnection()->HasFeature(FEATURE_FUNCTION_DEFAULTS))
            {
                tmp = functions->GetVal(wxT("proargdefvals"));
                if (!tmp.IsEmpty())
                    argDefsTkz.SetString(tmp.Mid(1, tmp.Length()-2), wxT(","));
            }

            // Now iterate the arguments and build the arrays
            wxString type, name, mode, def;
            
            while (argTypesTkz.HasMoreTokens())
            {
                // Add the arg type. This is a type oid, so 
                // look it up in the hashmap
                type = argTypesTkz.GetNextToken();
                function->iAddArgType(typeCache[type]);

                // Now add the name, stripping the quotes and \" if
                // necessary. 
                name = argNamesTkz.GetNextToken();
                if (!name.IsEmpty())
                {
                    if (name[0] == '"')
                        name = name.Mid(1, name.Length()-2);
                    name.Replace(wxT("\\\""), wxT("\""));
                    function->iAddArgName(name);
                }
                else
                    function->iAddArgName(wxEmptyString);

                // Now the mode
                mode = argModesTkz.GetNextToken();
                if (!mode.IsEmpty())
                {
                    if (mode == wxT('o') || mode == wxT("2"))
                        mode = wxT("OUT");
                    else if (mode == wxT("b"))
                        if (isProcedure)
                            mode = wxT("IN OUT");
                        else
                            mode = wxT("INOUT");
                    else if (mode == wxT("3"))
                        mode = wxT("IN OUT");
                    else
                        mode = wxT("IN");

                    function->iAddArgMode(mode);
                }
                else
                    function->iAddArgMode(wxEmptyString);

                // Finally the defaults, as we got them. 
                def = argDefsTkz.GetNextToken();
                if (!def.IsEmpty() && !def.IsSameAs(wxT("-")))
                {
                    if (def[0] == '"')
                        def = def.Mid(1, def.Length()-2);

                    // Check the cache first - if we don't have a value, get it and cache for next time
                    wxString val = exprCache[def];
                    if (val == wxEmptyString)
                    {
                        val = obj->GetDatabase()->ExecuteScalar(wxT("SELECT pg_get_expr('") + def + wxT("', 'pg_catalog.pg_class'::regclass)"));
                        exprCache[def] = val;
                    }
                    function->iAddArgDef(val);
                }
                else
                    function->iAddArgDef(wxEmptyString);
            }

            function->iSetOid(functions->GetOid(wxT("oid")));
			function->iSetXid(functions->GetOid(wxT("xmin")));
            function->UpdateSchema(browser, functions->GetOid(wxT("pronamespace")));
            function->iSetOwner(functions->GetVal(wxT("funcowner")));
            function->iSetAcl(functions->GetVal(wxT("proacl")));
            function->iSetArgCount(functions->GetLong(wxT("pronargs")));
            function->iSetReturnType(functions->GetVal(wxT("typname")));
            function->iSetComment(functions->GetVal(wxT("description")));

            function->iSetLanguage(lanname);
            function->iSetSecureDefiner(functions->GetBool(wxT("prosecdef")));
            function->iSetReturnAsSet(functions->GetBool(wxT("proretset")));
            function->iSetIsStrict(functions->GetBool(wxT("proisstrict")));
            function->iSetSource(functions->GetVal(wxT("prosrc")));
            function->iSetBin(functions->GetVal(wxT("probin")));

            wxString vol=functions->GetVal(wxT("provolatile"));
            function->iSetVolatility(
                vol.IsSameAs(wxT("i")) ? wxT("IMMUTABLE") : 
                vol.IsSameAs(wxT("s")) ? wxT("STABLE") :
                vol.IsSameAs(wxT("v")) ? wxT("VOLATILE") : wxT("unknown"));

            // PostgreSQL 8.3 cost/row estimations
            if (obj->GetConnection()->BackendMinimumVersion(8, 3))
            {
                function->iSetCost(functions->GetLong(wxT("procost")));
                function->iSetRows(functions->GetLong(wxT("prorows")));
                wxString cfg=functions->GetVal(wxT("proconfig"));
                if (!cfg.IsEmpty())
                    FillArray(function->GetConfigList(), cfg.Mid(1, cfg.Length()-2));
            }

            if (browser)
            {
                browser->AppendObject(obj, function);
			    functions->MoveNext();
            }
            else
                break;
        }

		delete functions;
        delete types;
    }
    return function;
}



pgObject *pgFunction::Refresh(ctlTree *browser, const wxTreeItemId item)
{
    pgObject *function=0;
    pgCollection *coll=browser->GetParentCollection(item);
    if (coll)
        function = functionFactory.AppendFunctions(coll, GetSchema(), 0, wxT(" WHERE pr.oid=") + GetOidStr() + wxT("\n"));

    // We might be linked to trigger....
    pgObject *trigger=browser->GetParentObject(item);
    if (trigger->GetMetaType() == PGM_TRIGGER)
        function = functionFactory.AppendFunctions(trigger, GetSchema(), 0, wxT(" WHERE pr.oid=") + GetOidStr() + wxT("\n"));

    return function;
}



pgObject *pgFunctionFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restr)
{
    wxString funcRestriction=wxT(
        " WHERE proisagg = FALSE AND pronamespace = ") + NumToStr(collection->GetSchema()->GetOid()) 
        + wxT("::oid\n   AND typname <> 'trigger'\n");

    if (collection->GetConnection()->EdbMinimumVersion(8, 0))
        funcRestriction += wxT("   AND NOT (lanname = 'edbspl' AND typname = 'void')\n");

    // Get the Functions
    return AppendFunctions(collection, collection->GetSchema(), browser, funcRestriction);
}


pgObject *pgTriggerFunctionFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restr)
{
    wxString funcRestriction=wxT(
        " WHERE proisagg = FALSE AND pronamespace = ") + NumToStr(collection->GetSchema()->GetOid()) 
        + wxT("::oid\n   AND typname = 'trigger'\n")
        + wxT("   AND lanname != 'edbspl'\n");

    // Get the Functions
    return AppendFunctions(collection, collection->GetSchema(), browser, funcRestriction);
}


pgObject *pgProcedureFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restr)
{
    wxString funcRestriction=wxT(
        " WHERE proisagg = FALSE AND pronamespace = ") + NumToStr(collection->GetSchema()->GetOid()) 
        + wxT("::oid AND lanname = 'edbspl' AND typname = 'void'\n");

    // Get the Functions
    return AppendFunctions(collection, collection->GetSchema(), browser, funcRestriction);
}


#include "images/function.xpm"
#include "images/functions.xpm"

pgFunctionFactory::pgFunctionFactory(const wxChar *tn, const wxChar *ns, const wxChar *nls, char **img) 
: pgSchemaObjFactory(tn, ns, nls, img)
{
    metaType = PGM_FUNCTION;
}

pgFunctionFactory functionFactory(__("Function"), __("New Function..."), __("Create a new Function."), function_xpm);
static pgaCollectionFactory cf(&functionFactory, __("Functions"), functions_xpm);


#include "images/triggerfunction.xpm"
#include "images/triggerfunctions.xpm"

pgTriggerFunctionFactory::pgTriggerFunctionFactory() 
: pgFunctionFactory(__("Trigger Function"), __("New Trigger Function..."), __("Create a new Trigger Function."), triggerfunction_xpm)
{
}

pgTriggerFunctionFactory triggerFunctionFactory;
static pgaCollectionFactory cft(&triggerFunctionFactory, __("Trigger Functions"), triggerfunctions_xpm);

#include "images/procedure.xpm"
#include "images/procedures.xpm"

pgProcedureFactory::pgProcedureFactory() 
: pgFunctionFactory(__("Procedure"), __("New Procedure"), __("Create a new Procedure."), procedure_xpm)
{
}

pgProcedureFactory procedureFactory;
static pgaCollectionFactory cfp(&procedureFactory, __("Procedures"), procedures_xpm);
