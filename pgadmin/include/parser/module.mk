#######################################################################
#
# pgAdmin III - PostgreSQL Tools
# $Id$
# Copyright (C) 2002 - 2009, The pgAdmin Development Team
# This software is released under the BSD Licence
#
# module.mk - pgadmin/include/parser/ Makefile fragment
#
#######################################################################

pgadmin3_SOURCES += \
	$(srcdir)/include/parser/keywords.h

EXTRA_DIST += \
        $(srcdir)/include/parser/module.mk
