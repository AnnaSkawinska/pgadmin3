//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// pgOperator.h PostgreSQL Operator
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGOperator_H
#define PGOperator_H

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "pgObject.h"
#include "pgServer.h"
#include "pgDatabase.h"


class pgOperator : public pgSchemaObject
{
public:
    pgOperator(pgSchema *newSchema, const wxString& newName = wxT(""));
    ~pgOperator();

    int GetIcon() { return PGICON_OPERATOR; }
    void ShowTreeDetail(wxTreeCtrl *browser, frmMain *form=0, ctlListView *properties=0, ctlSQLBox *sqlPane=0);
    static pgObject *ReadObjects(pgCollection *collection, wxTreeCtrl *browser, const wxString &restriction=wxT(""));
    virtual wxString GetQuotedIdentifier() const { return GetName(); }
    wxString GetFullName() const;
    wxString GetOperands() const;
    wxString GetLeftType() const { return leftType; }
    void iSetLeftType(const wxString& s) { leftType=s; }
    wxString GetRightType() const { return rightType; }
    void iSetRightType(const wxString& s) { rightType=s; }
    OID GetLeftTypeOid() const { return leftTypeOid; }
    void iSetLeftTypeOid(const OID o) { leftTypeOid=o; }
    OID GetRightTypeOid() const { return rightTypeOid; }
    void iSetRightTypeOid(const OID o) { rightTypeOid=o; }
    wxString GetResultType() { return resultType; }
    void iSetResultType(const wxString& s) { resultType=s; }
    wxString GetOperatorFunction() const { return operatorFunction; }
    void iSetOperatorFunction(const wxString& s) { operatorFunction=s; }
    wxString GetJoinFunction() const { return joinFunction; }
    void iSetJoinFunction(const wxString& s) { joinFunction=s; }
    wxString GetRestrictFunction() const { return restrictFunction; }
    void iSetRestrictFunction(const wxString& s) { restrictFunction=s; }
    wxString GetCommutator() const { return commutator; }
    void iSetCommutator(const wxString& s) { commutator=s; }
    wxString GetNegator() const { return negator; }
    void iSetNegator(const wxString& s) { negator=s; }
    wxString GetKind() const { return kind; }
    void iSetKind(const wxString& s) { kind=s; }
    wxString GetLeftSortOperator() const { return leftSortOperator; }
    void iSetLeftSortOperator(const wxString& s) { leftSortOperator=s; }
    wxString GetRightSortOperator() const { return  rightSortOperator; }
    void iSetRightSortOperator(const wxString& s) {  rightSortOperator=s; }
    wxString GetLessOperator() const { return lessOperator; }
    void iSetLessOperator(const wxString& s) { lessOperator=s; }
    wxString GetGreaterOperator() const { return  greaterOperator; }
    void iSetGreaterOperator(const wxString& s) {  greaterOperator=s; }
    bool GetHashJoins() const { return hashJoins; }
    void iSetHashJoins(bool b) {  hashJoins=b; }

    bool DropObject(wxFrame *frame, wxTreeCtrl *browser);
    wxString GetSql(wxTreeCtrl *browser);
    pgObject *Refresh(wxTreeCtrl *browser, const wxTreeItemId item);

private:
    wxString leftType, rightType, resultType,
             operatorFunction, joinFunction, restrictFunction,
             commutator, negator, kind, 
             leftSortOperator, rightSortOperator, lessOperator, greaterOperator;
    OID leftTypeOid, rightTypeOid;
    bool hashJoins;
};

#endif
