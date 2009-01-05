#######################################################################
#
# pgAdmin III - PostgreSQL Tools
# $Id$
# Copyright (C) 2002 - 2009, The pgAdmin Development Team
# This software is released under the Artistic Licence
#
# module.mk - pgadmin/agent/ Makefile fragment
#
#######################################################################

pgadmin3_SOURCES += \
    $(srcdir)/agent/dlgJob.cpp \
	  $(srcdir)/agent/dlgSchedule.cpp \
	  $(srcdir)/agent/dlgStep.cpp \
	  $(srcdir)/agent/pgaJob.cpp \
	  $(srcdir)/agent/pgaSchedule.cpp \
	  $(srcdir)/agent/pgaStep.cpp

EXTRA_DIST += \
    $(srcdir)/agent/module.mk


