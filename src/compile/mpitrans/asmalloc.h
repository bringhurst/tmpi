# ifndef AS_MALLOC_H
# define AS_MALLOC_H
# define as_malloc(size) AS_Malloc1(size, __LINE__, __FILE__)
# define as_free(mem) AS_Free1(mem, __LINE__, __FILE__)
# define as_init() AS_InitMem()
void AS_InitMem();
void * AS_Malloc1(const size_t size, const int line, char  *file);
void AS_Free1(void *ptr, const int line, const char *file);
void AS_DumpMem();
void AS_CheckMem();
# endif
