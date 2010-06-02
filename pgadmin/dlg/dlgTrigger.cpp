//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2010, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// dlgTrigger.cpp - PostgreSQL Trigger Property
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "frm/frmMain.h"
#include "utils/pgDefs.h"

#include "dlg/dlgTrigger.h"
#include "schema/pgTrigger.h"
#include "schema/pgTable.h"
#include "schema/pgSchema.h"


// pointer to controls
#define chkRow          CTRL_CHECKBOX("chkRow")
#define cbFunction      CTRL_COMBOBOX2("cbFunction")
#define txtArguments    CTRL_TEXT("txtArguments")
#define rdbFires        CTRL_RADIOBOX("rdbFires")
#define chkInsert       CTRL_CHECKBOX("chkInsert")
#define chkUpdate       CTRL_CHECKBOX("chkUpdate")
#define chkDelete       CTRL_CHECKBOX("chkDelete")
#define chkTruncate     CTRL_CHECKBOX("chkTruncate")
#define txtBody         CTRL_SQLBOX("txtBody")

BEGIN_EVENT_TABLE(dlgTrigger, dlgProperty)
    EVT_RADIOBOX(XRCID("rdbFires"),                 dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkRow"),                   dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkInsert"),                dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkUpdate"),                dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkDelete"),                dlgProperty::OnChange)
    EVT_CHECKBOX(XRCID("chkTruncate"),              dlgProperty::OnChange)
    EVT_TEXT(XRCID("cbFunction"),                   dlgTrigger::OnChangeFunc)
    EVT_COMBOBOX(XRCID("cbFunction"),               dlgProperty::OnChange)
    EVT_TEXT(XRCID("txtArguments"),                 dlgProperty::OnChange)
    EVT_STC_MODIFIED(XRCID("txtBody"),              dlgProperty::OnChangeStc)
    EVT_BUTTON(wxID_APPLY,                          dlgTrigger::OnApply)
END_EVENT_TABLE();


dlgProperty *pgTriggerFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
{
    return new dlgTrigger(this, frame, (pgTrigger*)node, (pgTable*)parent);
}




dlgTrigger::dlgTrigger(pgaFactory *f, frmMain *frame, pgTrigger *node, pgTable *parentNode)
: dlgProperty(f, frame, wxT("dlgTrigger"))
{
    trigger=node;
    table=parentNode;
    wxASSERT(!table || table->GetMetaType() == PGM_TABLE || table->GetMetaType() == GP_PARTITION);

    txtBody->SetMarginType(1, wxSTC_MARGIN_NUMBER);
    txtBody->SetMarginWidth(1, ConvertDialogToPixels(wxPoint(16, 0)).x);
}


pgObject *dlgTrigger::GetObject()
{
    return trigger;
}


int dlgTrigger::Go(bool modal)
{
    if (trigger)
    {
        // edit mode
        chkRow->SetValue((trigger->GetTriggerType() & TRIGGER_TYPE_ROW) != 0);
        chkInsert->SetValue((trigger->GetTriggerType() & TRIGGER_TYPE_INSERT) != 0);
        chkUpdate->SetValue((trigger->GetTriggerType() & TRIGGER_TYPE_UPDATE) != 0);
        chkDelete->SetValue((trigger->GetTriggerType() & TRIGGER_TYPE_DELETE) != 0);
		chkTruncate->SetValue((trigger->GetTriggerType() & TRIGGER_TYPE_TRUNCATE) != 0);
        rdbFires->SetSelection(trigger->GetTriggerType() & TRIGGER_TYPE_BEFORE ? 0 : 1);
        txtArguments->SetValue(trigger->GetArguments());
        if (!connection->BackendMinimumVersion(7, 4))
            txtName->Disable();

        if (trigger->GetLanguage() == wxT("edbspl"))
        {
            cbFunction->Append(wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")));
            txtBody->SetText(trigger->GetSource());
        }
        else
        {
            cbFunction->Append(trigger->GetFunction());
            txtBody->Disable();
        }

        cbFunction->SetSelection(0);
        txtArguments->Disable();
        cbFunction->Disable();

        if (!connection->EdbMinimumVersion(8,0))
        {
            chkRow->Disable();
            rdbFires->Disable();
            chkInsert->Disable();
            chkUpdate->Disable();
            chkDelete->Disable();
		    chkTruncate->Disable();
        }
        else if (!connection->BackendMinimumVersion(8, 4))
			chkTruncate->Disable();
    }
    else
    {
        // create mode
        if (connection->EdbMinimumVersion(8,0))
            cbFunction->Append(wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")));

        wxString sysRestr;
        if (!settings->GetShowSystemObjects())
            sysRestr = wxT("   AND ") + connection->SystemNamespaceRestriction(wxT("nspname"));

        pgSet *set=connection->ExecuteSet(
            wxT("SELECT quote_ident(nspname) || '.' || quote_ident(proname)\n")
            wxT("  FROM pg_proc p, pg_namespace n, pg_language l\n")
            wxT(" WHERE p.pronamespace = n.oid AND p.prolang = l.oid AND l.lanname != 'edbspl' AND prorettype=") + NumToStr(PGOID_TYPE_TRIGGER) + sysRestr + 
            wxT(" ORDER BY nspname ASC, proname ASC "));
        if (set)
        {
            while (!set->Eof())
            {
                cbFunction->Append(set->GetVal(0));
                set->MoveNext();
            }
            delete set;
        }
        if (!connection->BackendMinimumVersion(7, 4))
        {
            chkRow->SetValue(true);
            chkRow->Disable();
        }
        
        txtBody->Disable();

		if (!connection->BackendMinimumVersion(8, 4))
			chkTruncate->Disable();

    }

    return dlgProperty::Go(modal);
}


wxString dlgTrigger::GetSql()
{
    wxString sql;
    wxString name=GetName();

    if (trigger)
    {
        if (name != trigger->GetName())
            sql = wxT("ALTER TRIGGER ") + trigger->GetQuotedIdentifier() + wxT(" ON ") + table->GetQuotedFullIdentifier()
                + wxT(" RENAME TO ") + qtIdent(name) + wxT(";\n\n");
    }

    if (!trigger ||
        (cbFunction->GetValue() == wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")) && txtBody->GetText() != trigger->GetSource()) ||
        chkRow->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_ROW) ||
        chkInsert->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_INSERT ? true : false) ||
        chkUpdate->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_UPDATE ? true : false) ||
        chkDelete->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_DELETE ? true : false) ||
		chkTruncate->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_TRUNCATE ? true : false) ||
        rdbFires->GetSelection() != (trigger->GetTriggerType() & TRIGGER_TYPE_BEFORE ? 0 : 1))
    {
        if (cbFunction->GetValue() == wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")))
            sql += wxT("CREATE OR REPLACE TRIGGER ") + qtIdent(name);
        else
            sql += wxT("CREATE TRIGGER ") + qtIdent(name);

        if (rdbFires->GetSelection())
            sql += wxT(" AFTER");
        else
            sql += wxT(" BEFORE");
        int actionCount=0;
        if (chkInsert->GetValue())
        {
            if (actionCount++)
                sql += wxT(" OR");
            sql += wxT(" INSERT");
        }
        if (chkUpdate->GetValue())
        {
            if (actionCount++)
                sql += wxT(" OR");
            sql += wxT(" UPDATE");
        }
        if (chkDelete->GetValue())
        {
            if (actionCount++)
                sql += wxT(" OR");
            sql += wxT(" DELETE");
        }
        if (chkTruncate->GetValue())
        {
            if (actionCount++)
                sql += wxT(" OR");
            sql += wxT(" TRUNCATE");
        }
        sql += wxT("\n   ON ") + table->GetQuotedFullIdentifier()
            + wxT(" FOR EACH ");
        if (chkRow->GetValue())
            sql += wxT("ROW");
        else
            sql += wxT("STATEMENT");

        if (cbFunction->GetValue() != wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")))
        {
            sql += wxT("\n   EXECUTE PROCEDURE ") + cbFunction->GetValue()
                + wxT("(") + txtArguments->GetValue()
                + wxT(");\n");
        }
        else
        {
            sql += wxT("\n") + txtBody->GetText();
            if (!sql.Trim().EndsWith(wxT(";")))
                sql = sql.Trim() + wxT(";");
            sql += wxT("\n");
        }
    }
    AppendComment(sql, wxT("TRIGGER ") + qtIdent(GetName()) 
        + wxT(" ON ") + table->GetQuotedFullIdentifier(), trigger);

    return sql;
}


pgObject *dlgTrigger::CreateObject(pgCollection *collection)
{
    pgObject *obj=triggerFactory.CreateObjects(collection, 0, 
        wxT("\n   AND tgname=") + qtDbString(GetName()) +
        wxT("\n   AND tgrelid=") + table->GetOidStr() +
        wxT("\n   AND relnamespace=") + table->GetSchema()->GetOidStr());
    return obj;
}


void dlgTrigger::OnChangeFunc(wxCommandEvent &ev)
{
    cbFunction->GuessSelection(ev);

    if (cbFunction->GetValue() == wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")))
    {
        txtArguments->Disable();
        txtBody->Enable();
    }
    else
    {
        txtArguments->Enable();
        txtBody->Disable();
    }

    CheckChange();
}


void dlgTrigger::CheckChange()
{
    bool enable=true;

    wxString function=cbFunction->GetValue();
    wxString name=GetName();

	// We can only have per-statement TRUNCATE triggers
	if (connection->BackendMinimumVersion(8, 4))
	{
		if (chkRow->GetValue())
		{
			chkTruncate->Disable();
			chkTruncate->SetValue(false);
		}
		else
			chkTruncate->Enable();
	}

    CheckValid(enable, !name.IsEmpty(), _("Please specify name."));
    CheckValid(enable, !function.IsEmpty(), _("Please specify trigger function."));

    CheckValid(enable, chkInsert->GetValue() || chkUpdate->GetValue() ||chkDelete->GetValue() ||chkTruncate->GetValue(),
        _("Please specify at least one action."));

    if (cbFunction->GetValue() == wxString::Format(wxT("<%s>"), _("Inline EDB-SPL")))
        CheckValid(enable, !txtBody->GetText().IsEmpty(), _("Please specify trigger body."));

    if (trigger)
    {
        EnableOK(enable &&
                 (txtComment->GetValue() != trigger->GetComment() ||
                 txtName->GetValue() != trigger->GetName() ||
                 (txtBody->GetText() != trigger->GetSource() && cbFunction->GetValue() == wxString::Format(wxT("<%s>"), _("Inline EDB-SPL"))) ||
                 chkRow->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_ROW ? true : false) ||
                 chkInsert->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_INSERT ? true : false) ||
                 chkUpdate->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_UPDATE ? true : false) ||
                 chkDelete->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_DELETE ? true : false) ||
				 chkTruncate->GetValue() != (trigger->GetTriggerType() & TRIGGER_TYPE_TRUNCATE ? true : false) ||
                 rdbFires->GetSelection() != (trigger->GetTriggerType() & TRIGGER_TYPE_BEFORE ? 0 : 1)));
    }
    else
    {
        EnableOK(enable);
    }
}

bool dlgTrigger::IsUpToDate()
{
	if (trigger && !trigger->IsUpToDate())
		return false;
	else
		return true;
}
  
void dlgTrigger::OnApply(wxCommandEvent &ev)
{
    dlgProperty::OnApply(ev);

    wxString sql = wxT("SELECT xmin FROM pg_trigger WHERE oid = ") + trigger->GetOidStr();
	trigger->iSetXid(StrToOid(connection->ExecuteScalar(sql)));
}
