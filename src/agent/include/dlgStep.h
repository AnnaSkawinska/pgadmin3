//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// dlgStep.h - Job property
//
//////////////////////////////////////////////////////////////////////////


#ifndef __DLG_STEPPROP
#define __DLG_STEPPROP

#include "dlgProperty.h"

class pgaStep;
class pgaJob;

class dlgStep : public dlgAgentProperty
{
public:
    dlgStep(pgaFactory *factory, frmMain *frame, pgaStep *s, pgaJob *j);

    void CheckChange();
    int Go(bool modal);

    wxString GetUpdateSql();
    wxString GetInsertSql();
    wxString GetComment();
    pgObject *CreateObject(pgCollection *collection);
    pgObject *GetObject();
    void SetJobId(long id) { jobId = id; }

    wxString GetHelpPage(bool forCreate) const { return wxT("pgagent-steps"); }

private:
    long jobId;
    ctlSQLBox *sqlBox;
    pgaStep *step;
    pgaJob *job;

    DECLARE_EVENT_TABLE();
};


#endif
