#######################################################################
#
# pgAdmin III - PostgreSQL Tools
# $Id$
# Copyright (C) 2002 - 2006, The pgAdmin Development Team
# This software is released under the Artistic Licence
#
# module.mk - src/base/ Makefile fragment
#
#######################################################################

pgadmin3_SOURCES += \
	$(srcdir)/base/base.cpp \
	$(srcdir)/base/appbase.cpp \
	$(srcdir)/base/sysLogger.cpp \
	$(srcdir)/base/pgConnBase.cpp \
	$(srcdir)/base/pgSetBase.cpp \
	$(srcdir)/base/factory.cpp 

EXTRA_DIST += \
        $(srcdir)/base/module.mk
