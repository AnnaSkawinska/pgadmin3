//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2005, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// xh_calb.cpp - wxCalendarBox handler
//
//////////////////////////////////////////////////////////////////////////
 
#include "wx/wx.h"
#include "xh_calb.h"
#include "calbox.h"


IMPLEMENT_DYNAMIC_CLASS(wxCalendarBoxXmlHandler, wxXmlResourceHandler)

wxCalendarBoxXmlHandler::wxCalendarBoxXmlHandler() 
: wxXmlResourceHandler() 
{
    XRC_ADD_STYLE(wxCAL_SUNDAY_FIRST);
    XRC_ADD_STYLE(wxCAL_MONDAY_FIRST);
    XRC_ADD_STYLE(wxCAL_SHOW_HOLIDAYS);

    AddWindowStyles();
}


wxObject *wxCalendarBoxXmlHandler::DoCreateResource()
{ 
    XRC_MAKE_INSTANCE(calendar, wxCalendarBox);

    calendar->Create(m_parentAsWindow,
                     GetID(),
                     wxDefaultDateTime,
                     GetPosition(), GetSize(),
                     GetStyle(),
                     GetName());
    
    SetupWindow(calendar);
    
    return calendar;
}

bool wxCalendarBoxXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxCalendarBox"));
}
