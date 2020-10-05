#!/usr/bin/env sh
name="$(basename $1 .dust)"
echo babeltrace2 --plugin-path=./.libs --component=source.dust.input --params="path=\"$1\"" --component=sink.testing_clprof.dispatch --params="test=\"${name}\""
babeltrace2 --plugin-path=./ --component=source.dust.input --params="path=\"$1\"" --component=sink.testing_clprof.dispatch --params="test=\"${name}\""
