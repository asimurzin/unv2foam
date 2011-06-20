dnl confFlu - pythonFlu configuration package
dnl Copyright (C) 2010- Alexey Petrov
dnl Copyright (C) 2009-2010 Pebble Bed Modular Reactor (Pty) Limited (PBMR)
dnl 
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl 
dnl See http://sourceforge.net/projects/pythonflu
dnl
dnl Author : Alexey PETROV
dnl


dnl --------------------------------------------------------------------------------
AC_DEFUN([UNV2FOAM_CHECK_UNV2FOAM],
[
AC_CHECKING(for unv2foam package)

AC_REQUIRE([CONFFLU_CHECK_OPENFOAM])

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

UNV2FOAM_CPPFLAGS=""
AC_SUBST(UNV2FOAM_CPPFLAGS)

UNV2FOAM_CXXFLAGS=""
AC_SUBST(UNV2FOAM_CXXFLAGS)

UNV2FOAM_LDFLAGS=""
AC_SUBST(UNV2FOAM_LDFLAGS)

AC_SUBST(ENABLE_UNV2FOAM)

unv2foam_ok=no

dnl --------------------------------------------------------------------------------
AC_ARG_WITH( [unv2foam-includes],
             AC_HELP_STRING( [--with-unv2foam-includes=<path>],
                             [use <path> to look for unv2foam installation] ),
             [],
             [])
   
dnl --------------------------------------------------------------------------------
unv2foam_headers_dir=${with_unv2foam_includes}

if test "x${with_unv2foam_includes}" = "x" ; then
   if test ! "x${UNV2FOAM_ROOT_DIR}" = "x" && test -d ${UNV2FOAM_ROOT_DIR} ; then
      unv2foam_headers_dir=${UNV2FOAM_ROOT_DIR}/lib
   fi
fi

AC_CHECK_FILE( [${unv2foam_headers_dir}/unv2foam.H], [ unv2foam_includes=yes ], [ unv2foam_includes=no ] )

if test "x${unv2foam_includes}" = "xno" ; then
   unv2foam_headers_dir=/usr/local/include/unv2foam
   AC_CHECK_FILE( [${unv2foam_headers_dir}/unv2foam.H], [ unv2foam_includes=yes ], [ unv2foam_includes=no ] )
fi

if test "x${unv2foam_includes}" = "xyes"; then
   UNV2FOAM_CPPFLAGS="-I${unv2foam_headers_dir}"
   dnl AC_CHECK_HEADERS( [unv2foam.H], [ unv2foam_ok=yes ], [ unv2foam_ok=no ] )
fi

if test "x${unv2foam_includes}" = "xno" ; then
   AC_MSG_WARN( [use --with-unv2foam-includes=<path> to define path to unv2foam headers] )
fi


dnl --------------------------------------------------------------------------------
AC_ARG_WITH( [unv2foam-libraries],
             AC_HELP_STRING( [--with-unv2foam-libraries=<path>],
                             [use <path> to look for unv2foam installation] ),
             [],
             [])
   
dnl --------------------------------------------------------------------------------
unv2foam_libraries_dir=${with_unv2foam_libraries}

if test "x${with_unv2foam_libraries}" = "x" ; then
   if test ! "x${UNV2FOAM_ROOT_DIR}" = "x" && test -d ${UNV2FOAM_ROOT_DIR} ; then
      unv2foam_libraries_dir=${UNV2FOAM_ROOT_DIR}/lib
   fi
fi

AC_CHECK_FILE( [${unv2foam_libraries_dir}/libunv2foam.so], [ unv2foam_libraries=yes ], [ unv2foam_libraries=no ] )

if test "x${unv2foam_libraries}" = "xno" ; then
   unv2foam_libraries_dir=/usr/local/lib
   AC_CHECK_FILE( [${unv2foam_libraries_dir}/libunv2foam.so], [ unv2foam_libraries=yes ], [ unv2foam_libraries=no ] )
fi

if test "x${unv2foam_libraries}" = "xyes"; then
   UNV2FOAM_CXXFLAGS=""

   UNV2FOAM_LDFLAGS="-L${unv2foam_root_dir} -lunv2foam"

   dnl AC_MSG_CHECKING( for linking to unv2foam library )
   dnl AC_LINK_IFELSE( [ AC_LANG_PROGRAM( [ #include <unv2foam.H> ], [ Foam::unv2foam( "dummy", Foam::Time() ) ] ) ],
   dnl                 [ unv2foam_ok=yes ],
   dnl                 [ unv2foam_ok=no ] )
   dnl AC_MSG_RESULT( ${unv2foam_ok} )

fi

if test "x${unv2foam_libraries}" = "xno" ; then
   AC_MSG_WARN( [use --with-unv2foam-libraries=<path> to define path to unv2foam libraries] )
fi


dnl --------------------------------------------------------------------------------
AC_CHECK_PROG( [unv2foam_exe], [unv2foam], [yes], [no] )


dnl --------------------------------------------------------------------------------
unv2foam_wrapper=no
unv2foam_wrapper=[`python -c "import unv2foam; print \"yes\"" 2>/dev/null`]


dnl --------------------------------------------------------------------------------
if test "${unv2foam_libraries}" = "yes" && test "${unv2foam_includes}" = "yes" && test "${unv2foam_wrapper}" = "yes" && test "${unv2foam_exe}" = "yes"; then 
   unv2foam_ok=yes
fi

if test "x${unv2foam_ok}" = "xno" ; then
   AC_MSG_WARN([use either \${UNV2FOAM_ROOT_DIR} or install it ])
fi


dnl --------------------------------------------------------------------------------
ENABLE_UNV2FOAM=${unv2foam_ok}
])


dnl --------------------------------------------------------------------------------
