# Kryten is a EPICS PV monitoring program that calls a system command
# when the value of the PV matches/cease to match specified criteria.
#
# Copyright (C) 2011  Andrew C. Starritt
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
# Makefile for kyrten extension top level.
#
TOP = ../..
include $(TOP)/configure/CONFIG
DIRS = kryten
include $(TOP)/configure/RULES_DIRS

# end
