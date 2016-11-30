##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
SUBDIRS =

# additional include pathes necessary to compile the C/C++ library
AM_CFLAGS = -I$(top_srcdir)/include 

###############################################################################
# THE LIBRARIES TO BUILD
###############################################################################

# the list of header files that belong to the library (to be installed later)
include_HEADERS = $(top_srcdir)/include/moca_hal.h


# the library names to build (note we are building dynamic libs only)
lib_LTLIBRARIES = librdkmocahal.la

# the sources to add to the library and to add to the source distribution
librdkmocahal_la_SOURCES = moca_hal.c
