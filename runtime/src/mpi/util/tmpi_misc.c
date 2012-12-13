# include <stdio.h>
# include "mpi.h"
# include "netq.h"
# include "sock_conn.h"
# include "machine.h"

int tmpi_Get_version(int *version, int *subversion)
{
	if (version)
		*version=MPI_VERSION;
	if (*subversion)
		*subversion=MPI_SUBVERSION;

	return MPI_SUCCESS;
}

int tmpi_Tag_ub(int *ub)
{
	if (ub)
		*ub=MPI_TAG_UB;
	return MPI_SUCCESS;
}

int tmpi_Host(int *host)
{
	if (host)
		*host=MPI_PROC_NULL;

	return MPI_SUCCESS;
}

int tmpi_Io(int *io)
{
	ID_ENTRY id_entry;
	int grank;

	id_entry.macID=0;
	id_entry.localID=0;

	if (netq_findGlobalRank(&id_entry, &grank)<0)
		grank=MPI_PROC_NULL;
	
	if (io)
		*io=grank;

	return MPI_SUCCESS;
}

int tmpi_Wtime_isglobal(int *flag)
{
	if (flag) {
		if (netq_getMacSize()==1)
			*flag=1;
		else
			*flag=0;
	}

	return MPI_SUCCESS;
}

/* I simply assume the host name won't be too long */
int tmpi_Get_processor_name(char *name, int *buflen)
{
	int ncpu;
	int mycpu;
	char strncpu[100]="(arch <unknown>)";
	char strmycpu[100]="<unknown>";

	if ((!name)||(!buflen))
		return MPI_ERR_PARAM;

	ncpu=cpu_total();
	mycpu=cpu_current();

	if (mycpu!=-1)
		sprintf(strncpu, "%d", mycpu);

	if (ncpu!=-1) {
		if (ncpu>1)
			sprintf(strncpu, "(smp-%d) on PROCID %s", ncpu, strmycpu);
		else
			sprintf(strncpu, "(uni-proc)");
	}

	sprintf(name, "%s%s", sock_hostname(), strncpu);
	*buflen=strlen(name)+1;

	return MPI_SUCCESS;
}
