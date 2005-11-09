//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgFunction.h PostgreSQL Function
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGFunction_H
#define PGFunction_H

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "pgObject.h"
#include "pgServer.h"
#include "pgDatabase.h"

class pgCollection;

class pgFunction : public pgSchemaObject
{
public:
    pgFunction(pgSchema *newSchema, const wxString& newName = wxT(""));
    ~pgFunction();


    int GetIcon() { return PGICON_FUNCTION; }
    void ShowTreeDetail(wxTreeCtrl *browser, frmMain *form=0, ctlListView *properties=0, ctlSQLBox *sqlPane=0);
    static pgObject *ReadObjects(pgCollection *collection, wxTreeCtrl *browser);
    static pgFunction *pgFunction::AppendFunctions(pgObject *obj, pgSchema *schema, wxTreeCtrl *browser, const wxString &restriction);

    wxString GetFullName() const {return GetName()+wxT("(")+GetArgTypes()+wxT(")"); }
    wxString GetArgTypeNames() const { return argTypeNames; }
    void iSetArgTypeNames(const wxString& s) { argTypeNames=s; }
    wxString GetQuotedArgTypeNames() const { return quotedArgTypeNames; }
    void iSetQuotedArgTypeNames(const wxString& s) { quotedArgTypeNames=s; }
    wxString GetArgTypes() const { return argTypes; }
    void iSetArgTypes(const wxString& s) { argTypes=s; }
    wxString GetQuotedArgTypes() const { return quotedArgTypes; }
    void iSetQuotedArgTypes(const wxString& s) { quotedArgTypes=s; }
    wxString GetArgTypeOids() const { return argTypeOids; }
    wxArrayString &GetArgNames() { return argNames; }
    void iAddArgName(const wxString &s) { argNames.Add(s); }
    void iSetArgTypeOids(const wxString& s) { argTypeOids = s; }
    wxString GetReturnType() const { return returnType; }
	wxString GetQuotedReturnType() const { return quotedReturnType; }
    void iSetReturnType(const wxString& s) { returnType = s; }
	void iSetQuotedReturnType(const wxString& s) { quotedReturnType = s; }
    wxString GetLanguage() const { return language; }
    void iSetLanguage(const wxString& s) { language = s; }
    wxString GetVolatility() const { return volatility; }
    void iSetVolatility(const wxString& s) { volatility = s; }
    wxString GetSource() const { return source; }
    void iSetSource(const wxString& s) { source=s; }
    wxString GetBin() const { return bin; }
    void iSetBin(const wxString& s) { bin=s; }
    long GetArgCount() const { return argCount; }
    void iSetArgCount(long ac) { argCount = ac; }
    bool GetReturnAsSet() const { return returnAsSet; }
    void iSetReturnAsSet(bool b) { returnAsSet = b; }
    bool GetSecureDefiner() const { return secureDefiner; }
    void iSetSecureDefiner(bool b) { secureDefiner = b; }
    bool GetIsStrict() const { return isStrict; }
    void iSetIsStrict(bool b) { isStrict = b; }

    bool CanRestore() { return true; }
    bool DropObject(wxFrame *frame, wxTreeCtrl *browser);
    wxString GetSql(wxTreeCtrl *browser);
    wxString GetHelpPage(bool forCreate) const { return wxT("sql-createfunction"); }
    pgObject *Refresh(wxTreeCtrl *browser, const wxTreeItemId item);

protected:
    pgFunction(pgSchema *newSchema, int newType, const wxString& newName = wxT(""));

private:
    wxString argTypeOids, argTypes, quotedArgTypes,
        argTypeNames, quotedArgTypeNames,
        returnType, quotedReturnType, language, volatility, source, bin;
    wxArrayString argNames;
    bool returnAsSet, secureDefiner, isStrict;
    long argCount;
};

class pgTriggerFunction : public pgFunction
{
public:
    pgTriggerFunction(pgSchema *newSchema, const wxString& newName = wxT(""));
    static pgObject *ReadObjects(pgCollection *collection, wxTreeCtrl *browser);
    int GetIcon() { return PGICON_TRIGGERFUNCTION; }
};

#endif
