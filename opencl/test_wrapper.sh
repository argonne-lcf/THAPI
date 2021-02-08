#!/usr/bin/env sh
name="$(basename $1 .dust)"
echo babeltrace2 --plugin-path=./.libs --component=source.dust.input --params="path=\"$1\"" --component=sink.clprof.dispatch_testing --params="test=\"${name}\""
babeltrace2 --plugin-path=./ --component=source.dust.input --params="path=\"$1\"" --component=sink.clprof.dispatch_testing --params="test=\"${name}\""
