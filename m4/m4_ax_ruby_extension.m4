# SYNOPSIS
#
#   AX_RUBY_EXTENSION(extname[, fatal, ruby])
#
# DESCRIPTION
#
#   Checks for Ruby extension.
#
#   If fatal is non-empty then absence of a the extension will trigger an error.
#   The third parameter can be used to specify the ruby to use.
#
# LICENSE
#
#   Copyright (c) 2021 Brice Videau
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AU_ALIAS([AC_RUBY_EXTENSION], [AX_RUBY_EXTENSION])
AC_DEFUN([AX_RUBY_EXTENSION],[
    if test -z $RUBY;
    then
        if test -z "$3";
        then
            RUBY="ruby"
        else
            RUBY="$3"
        fi
    fi
    RUBY_NAME=`basename $RUBY`
    AC_MSG_CHECKING($RUBY_NAME extension: $1)
    $RUBY -e "gem *'$1'.split(' ', 2)" 2>/dev/null
    if test $? -eq 0;
    then
        AC_MSG_RESULT(yes)
        eval AS_TR_CPP(HAVE_RBEXT_$1)=yes
    else
        AC_MSG_RESULT(no)
        eval AS_TR_CPP(HAVE_RBEXT_$1)=no
        #
        if test -n "$2"
        then
            AC_MSG_ERROR(failed to find required extension $1)
            exit 1
        fi
    fi
])
