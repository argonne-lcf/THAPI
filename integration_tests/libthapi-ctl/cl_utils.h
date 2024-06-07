extern int read_from_binary(unsigned char **output, size_t *size, const char *name);
extern int write_to_binary(unsigned char *input, size_t size, const char *name);

extern char *read_from_file(const char *filename);

extern const char *getErrorString(cl_int error);

extern void check_error(cl_int error, char const *name);

extern void exit_msg(char const *str);
