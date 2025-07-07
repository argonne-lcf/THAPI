# ===========================================================================
#
# SYNOPSIS
#
#   AX_MPI_SESSION([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro tries to find out if the MPI compiler supports MPI sessions.

AU_ALIAS([ACX_MPI_SESSION], [AX_MPI_SESSION])
AC_DEFUN([AX_MPI_SESSION], [
AC_PREREQ(2.50) dnl for AC_LANG_CASE

AC_LANG_CASE([C], [
  AC_REQUIRE([AC_PROG_CC])
  AC_ARG_VAR(MPICC,[MPI C compiler command])
  AC_CHECK_PROGS(MPICC, mpicc hcc mpxlc_r mpxlc mpcc cmpicc, $CC)
  ax_mpi_session_save_CC="$CC"
  CC="$MPICC"
  AC_SUBST(MPICC)
],
[C++], [
  AC_REQUIRE([AC_PROG_CXX])
  AC_ARG_VAR(MPICXX,[MPI C++ compiler command])
  AC_CHECK_PROGS(MPICXX, mpic++ mpicxx mpiCC hcp mpxlC_r mpxlC mpCC cmpic++, $CXX)
  ax_mpi_session_save_CXX="$CXX"
  CXX="$MPICXX"
  AC_SUBST(MPICXX)
])

dnl We have to use AC_TRY_COMPILE and not AC_CHECK_HEADER because the
dnl latter uses $CPP, not $CC (which may be mpicc).
AC_LANG_CASE([C], [
  AC_MSG_CHECKING([for MPI_Session_init])
  AC_TRY_COMPILE([
    #include <mpi.h>
  ],[
    MPI_Session s;
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &s);
  ],[
    MPI_SESSION="yes"
    AC_MSG_RESULT(yes)
  ],[
    MPI_SESSION=""
    AC_MSG_RESULT(no)
  ])
],
[C++], [
  AC_MSG_CHECKING([for MPI_Session_init])
  AC_TRY_COMPILE([
    #include <mpi.h>
  ],[
    MPI_Session s;
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &s);
  ],[
    MPI_SESSION="yes"
    AC_MSG_RESULT(yes)
  ],[
    MPI_SESSION=""
    AC_MSG_RESULT(no)
  ])
])

AC_LANG_CASE([C], [CC="$ax_mpi_session_save_CC"],
  [C++], [CXX="$ax_mpi_session_save_CXX"])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x = x"$MPI_SESSION"; then
  $2
  :
else
  ifelse([$1],,[AC_DEFINE(HAVE_MPI_SESSION,1,[Define if you have the MPI sessions support.])],[$1])
  :
fi
])dnl AX_MPI_SESSION
