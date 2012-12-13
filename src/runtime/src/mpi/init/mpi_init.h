# ifndef _MPI_INIT_H_
# define _MPI_INIT_H_

int tmpi_main_init(int *argc, char ***argv);
int tmpi_main_end();

int tmpi_thr_init(int *argc, char ***argv, int myid);
int tmpi_thr_end(int myid);

int tmain(int argc, char **argv);

void tmpi_prog_main_init();
void tmpi_prog_user_init();

# endif
