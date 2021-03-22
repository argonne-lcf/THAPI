bin_SCRIPTS = iprof 

iprof: $(top_builddir)/utils/xprof.sh.erb
	$(ERB) -T 1 languages=["opencl","ze"] $(top_builddir)/utils/xprof.sh.erb  > $@
	chmod a+x $@

CLEANFILES = iprof

EXTRA_DIST = \
	interval.c.erb \
	interval.h.erb \
	interval_model.yaml
