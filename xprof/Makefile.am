bin_SCRIPTS = iprof 

iprof:
	$(ERB) -T 1 languages=["opencl","ze"] $(top_builddir)/utils/xprof.sh.erb  > $@
	chmod a+x $@
