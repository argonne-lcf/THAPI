extern void
thapi_register_sampling(
	void (*pfn_run)(void) /*Running*/,
	struct timespec *interval,
	void (*pfn_final)(void) /*Finalization*/);
