//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2010, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// frmSplash.h - Splash Screen
//
//////////////////////////////////////////////////////////////////////////

#ifndef SPLASH_H
#define SPLASH_H

#include "dlgClasses.h"
#include "base/factory.h"

// Class declarations
class frmSplash : public pgFrame
{
public:
    frmSplash(wxFrame *parent);
    ~frmSplash();
    void OnPaint(wxPaintEvent&);
    
private:
    wxBitmap splash;
    DECLARE_EVENT_TABLE()
};

#endif
