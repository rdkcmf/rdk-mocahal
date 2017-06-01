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
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.69])
AC_INIT([RDK-MOCA-HAL], [1.0], [Gabriel.Gooslin@broadcom.com])

AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
#AC_CONFIG_HEADERS([cfg/config.h])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE

AC_CONFIG_FILES([Makefile rmh_interface/Makefile rmh_lib/Makefile rmh_app/Makefile])

AC_OUTPUT
