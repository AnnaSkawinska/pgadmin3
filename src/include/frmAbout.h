//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2007, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// frmAbout.h - About Box
//
//////////////////////////////////////////////////////////////////////////

#ifndef FRMABOUT_H
#define FRMABOUT_H

#include "dlgClasses.h"
#include "base/factory.h"

// Class declarations
class frmAbout : public wxDialog
{
public:
    frmAbout(wxFrame *parent);
    ~frmAbout();
    void OnPaint(wxPaintEvent&);
    
private:
    wxBitmap about;
    DECLARE_EVENT_TABLE()
};

class aboutFactory : public actionFactory
{
public:
    aboutFactory(menuFactoryList *list, wxMenu *mnu, wxToolBar *toolbar);
    wxWindow *StartDialog(frmMain *form, pgObject *obj);
};


#endif
