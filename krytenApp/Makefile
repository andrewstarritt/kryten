# Kryten is a EPICS PV monitoring program that calls a system command
# when the value of the PV matches/cease to match specified criteria.
#
# Copyright (C) 2011-2018  Andrew C. Starritt
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact details:
# starritt@netspace.net.au
# PO Box 3118, Prahran East, Victoria 3181, Australia.
#
# Makefile for kyrten extension application.
#
TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================

#========================================
# Build the application
#
PROD_HOST += kryten

kryten_SRCS += buffered_callbacks.c
kryten_SRCS += filter.c
kryten_SRCS += information.c
kryten_SRCS += kryten.c
kryten_SRCS += pv_client.c
kryten_SRCS += read_configuration.c
kryten_SRCS += utilities.c
kryten_SRCS += gnu_public_licence.c

kryten_LIBS += $(EPICS_BASE_IOC_LIBS)

#========================================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end
