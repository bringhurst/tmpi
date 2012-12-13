/*
 * comm.c -- operations on group
 */

# include <stdlib.h>
# include "mpi_def.h"
# include "mpi_struct.h"
# include "tmpi_struct.h"
# include "comm.h"
# include "ukey.h"

static tmpi_Group *group_verify(MPI_Group group) {
	if (group==MPI_GROUP_EMPTY)
		return NULL;
	if (group->cookie==MPI_GROUP_COOKIE)
		return group;
	return NULL;
}

/* get the number of processes in the group */
int MPI_Group_size(MPI_Group group, int *size) {
	tmpi_Group *grp;
	if ((grp=group_verify(group))==NULL) return MPI_ERR_GROUP;
	*size=grp->np;
	return MPI_SUCCESS;
}

/* get the rank of calling process in this group */
int MPI_Group_rank(MPI_Group group, int *rank) {
	tmpi_Group *grp;
	if ((grp=group_verify(group))==NULL)
		return MPI_ERR_GROUP;
	*rank=grp->local_rank;
	return MPI_SUCCESS;
}

/* determing the relative ranks of two groups */
int MPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1, 
MPI_Group group2, int *ranks2) {
	tmpi_Group *grp1, *grp2;
	int i, j, grank;
	if ((grp1=group_verify(group1))==NULL) return MPI_ERR_GROUP;
	if ((grp2=group_verify(group2))==NULL) return MPI_ERR_GROUP;
	for (i=0; i<n; i++) {
		grank=grp1->lrank2grank[ranks1[i]];
		ranks2[i]=MPI_UNDEFINED;
		for (j=0; j<grp2->np; j++)
			if (grp2->lrank2grank[j]==grank) {
				ranks2[i]=j;
				break;
			}
	}

	return MPI_SUCCESS;
}

/* compare two MPI groups */
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result) {
	tmpi_Group *grp1, *grp2;
	int i, j, lrank, grank;
	if ((grp1=group_verify(group1))==NULL) return MPI_ERR_GROUP;
	if ((grp2=group_verify(group2))==NULL) return MPI_ERR_GROUP;
	if (grp1->np!=grp2->np) {
		*result=MPI_UNEQUAL;
		return MPI_SUCCESS;
	}
	*result=MPI_IDENT;
	for (i=0; i<grp1->np; i++)
		if (grp1->lrank2grank[i]!=grp2->lrank2grank[i]) {
			*result=MPI_SIMILAR;
			break;
		}
	if (*result==MPI_IDENT) return MPI_SUCCESS;
	for (i=0; i<grp1->np; i++) {
		grank=grp1->lrank2grank[i];
		lrank=MPI_UNDEFINED;
		for (j=0; j<grp2->np; j++)
			if (grank==grp2->lrank2grank[j]) {
				lrank=j;
				break;
			}
		if (lrank==MPI_UNDEFINED) {
			*result=MPI_UNEQUAL;
			break;
		}
	}

	return MPI_SUCCESS;
}

/* get a group of a communicator */
int MPI_Comm_group(MPI_Comm communicator, MPI_Group *group_ptr) {
	tmpi_Comm *comm;
	int myid=ukey_getid();

	if ((comm=tmpi_comm_verify(communicator, myid))==NULL) return MPI_ERR_COMM;
	*group_ptr=comm->group;
	comm->group->ref_count++;
	return MPI_SUCCESS;
}

/* get the union of two groups */
int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
	tmpi_Group *grp1, *grp2, *newgrp;
	int i, j, lrank, grank;

	if ((grp1=group_verify(group1))==NULL) return MPI_ERR_GROUP;
	if ((grp2=group_verify(group2))==NULL) return MPI_ERR_GROUP;

	newgrp=(tmpi_Group*)malloc(sizeof(tmpi_Group));
	newgrp->cookie=MPI_GROUP_COOKIE;
	newgrp->ref_count=1;
	newgrp->isPermanent=0;

	/* lrank to grank table */
	newgrp->lrank2grank=(int*)malloc(grp1->np+grp2->np);
	for (i=0; i<grp1->np; i++)
		newgrp->lrank2grank[i]=grp1->lrank2grank[i];
	newgrp->np=grp1->np;
	for (i=0; i<grp2->np; i++) {
		grank=grp2->lrank2grank[i];
		lrank=MPI_UNDEFINED;
		for (j=0; j<grp1->np; j++)
			if (grank==grp1->lrank2grank[j]) {
				lrank=j;
				break;
			}
		if (lrank==MPI_UNDEFINED)
			newgrp->lrank2grank[newgrp->np++]=grank;
	}

	/* local rank */
	if (grp1->local_rank!=MPI_UNDEFINED)
		newgrp->local_rank=grp1->local_rank;
	else 
		if (grp2->local_rank!=MPI_UNDEFINED) {
			grank=grp2->lrank2grank[grp2->local_rank];
			for (i=grp1->np; i<newgrp->np; i++)
				if (grank==newgrp->lrank2grank[i]) {
					newgrp->local_rank=i;
					break;
				}
		}
		else
			newgrp->local_rank=MPI_UNDEFINED;

	*newgroup=newgrp;

	return MPI_SUCCESS;
}

/* get the intersection of two groups */
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
	tmpi_Group *grp1, *grp2, *newgrp;
	int i, j, lrank, grank;

	if ((grp1=group_verify(group1))==NULL) return MPI_ERR_GROUP;
	if ((grp2=group_verify(group2))==NULL) return MPI_ERR_GROUP;

	newgrp=(tmpi_Group*)malloc(sizeof(tmpi_Group));
	newgrp->cookie=MPI_GROUP_COOKIE;
	newgrp->ref_count=1;
	newgrp->isPermanent=0;

	/* lrank to grank table */
	newgrp->lrank2grank=(int*)malloc(grp1->np);
	newgrp->np=0;
	for (i=0; i<grp1->np; i++) {
		grank=grp1->lrank2grank[i];
		lrank=MPI_UNDEFINED;
		for (j=0; j<grp2->np; j++)
			if (grank==grp2->lrank2grank[j]) {
				lrank=j;
				break;
			}
		if (lrank!=MPI_UNDEFINED)
			newgrp->lrank2grank[newgrp->np++]=grank;
	}

	/* local rank */
	if (grp1->local_rank==MPI_UNDEFINED || grp2->local_rank==MPI_UNDEFINED)
		newgrp->local_rank=MPI_UNDEFINED;
	else {
		grank=grp1->lrank2grank[grp1->local_rank];
		for (i=0; i<newgrp->np; i++)
			if (grank==newgrp->lrank2grank[i]) {
				newgrp->local_rank=i;
				break;
			}
	}

	if (newgrp->np==0) {
		free(newgrp->lrank2grank);
		free(newgrp);
		*newgroup=MPI_GROUP_EMPTY;
	}
	else
		*newgroup=newgrp;

	return MPI_SUCCESS;
}

/* get the difference of two groups */
int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
	tmpi_Group *grp1, *grp2, *newgrp;
	int i, j, lrank, grank;

	if ((grp1=group_verify(group1))==NULL) return MPI_ERR_GROUP;
	if ((grp2=group_verify(group2))==NULL) return MPI_ERR_GROUP;

	newgrp=(tmpi_Group*)malloc(sizeof(tmpi_Group));
	newgrp->cookie=MPI_GROUP_COOKIE;
	newgrp->ref_count=1;
	newgrp->isPermanent=0;

	/* lrank to grank table */
	newgrp->lrank2grank=(int*)malloc(grp1->np);
	newgrp->np=0;
	for (i=0; i<grp1->np; i++) {
		grank=grp1->lrank2grank[i];
		lrank=MPI_UNDEFINED;
		for (j=0; j<grp2->np; j++)
			if (grank==grp2->lrank2grank[j]) {
				lrank=j;
				break;
			}
		if (lrank==MPI_UNDEFINED)
			newgrp->lrank2grank[newgrp->np++]=grank;
	}

	/* local rank */
	if (grp1->local_rank==MPI_UNDEFINED || grp2->local_rank!=MPI_UNDEFINED)
		newgrp->local_rank=MPI_UNDEFINED;
	else {
		grank=grp1->lrank2grank[grp1->local_rank];
		for (i=0; i<newgrp->np; i++)
			if (grank==newgrp->lrank2grank[i]) {
				newgrp->local_rank=i;
				break;
			}
	}

	if (newgrp->np==0) {
		free(newgrp->lrank2grank);
		free(newgrp);
		*newgroup=MPI_GROUP_EMPTY;
	}
	else
		*newgroup=newgrp;

	return MPI_SUCCESS;
}

int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup) {
	tmpi_Group *grp, *newgrp;
	int i;

	if ((grp=group_verify(group))==NULL) return MPI_ERR_GROUP;

	if (n==0) {
		*newgroup=MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}

	newgrp=(tmpi_Group*)malloc(sizeof(tmpi_Group));
	newgrp->cookie=MPI_GROUP_COOKIE;
	newgrp->ref_count=1;
	newgrp->isPermanent=0;
	newgrp->np=n;
	newgrp->local_rank=MPI_UNDEFINED;
	newgrp->lrank2grank=(int*)malloc(n);
	for (i=0; i<n; i++) {
		newgrp->lrank2grank[i]=grp->lrank2grank[ranks[i]];
		if (grp->local_rank==ranks[i]) newgrp->local_rank=i;
	}

	*newgroup=newgrp;

	return MPI_SUCCESS;
}

int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup) {
	tmpi_Group *grp, *newgrp;
	int i, j, rank, grank;

	if ((grp=group_verify(group))==NULL) return MPI_ERR_GROUP;

	if (n>=grp->np) {
		*newgroup=MPI_GROUP_EMPTY;
		return MPI_SUCCESS;
	}

	newgrp=(tmpi_Group*)malloc(sizeof(tmpi_Group));
	newgrp->cookie=MPI_GROUP_COOKIE;
	newgrp->ref_count=1;
	newgrp->isPermanent=0;
	newgrp->np=0;

	/* lrank to grank table */
	newgrp->lrank2grank=(int*)malloc(grp->np-n);
	for (i=0; i<grp->np; i++) {
		rank=MPI_UNDEFINED;
		for (j=0; j<n; j++)
			if (ranks[j]==i) {
				rank=j;
				break;
			}
		if (rank==MPI_UNDEFINED)
			newgrp->lrank2grank[newgrp->np++]=grp->lrank2grank[i];
	}

	/* local rank */
	newgrp->local_rank=MPI_UNDEFINED;
	if (grp->local_rank!=MPI_UNDEFINED) {
		grank=grp->lrank2grank[grp->local_rank];
		for (i=0; i<newgrp->np; i++)
			if (newgrp->lrank2grank[i]==grank) {
				newgrp->local_rank=i;
				break;
			}
	}

	*newgroup=newgrp;

	return MPI_SUCCESS;
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) {
	int i, j, index=0, size=0, *ranks;
	for (i=0; i<n; i++) size+=(ranges[i][1]-ranges[i][0])/ranges[i][2]+1;
	ranks=(int*)malloc(size);
	for (i=0; i<n; i++)
		for (j=ranges[i][0]; j<=ranges[i][1]; j+=ranges[i][2])
			ranks[index++]=j;
	return MPI_Group_incl(group, n, ranks, newgroup);
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) {
	int i, j, index=0, size=0, *ranks;
	for (i=0; i<n; i++) size+=(ranges[i][1]-ranges[i][0])/ranges[i][2]+1;
	ranks=(int*)malloc(size);
	for (i=0; i<n; i++)
		for (j=ranges[i][0]; j<=ranges[i][1]; j+=ranges[i][2])
			ranks[index++]=j;
	return MPI_Group_excl(group, n, ranks, newgroup);
}

/* group destructor */
int MPI_Group_free(MPI_Group *group_ptr) {
	tmpi_Group *grp;

	if ((grp=group_verify(*group_ptr))==NULL) return MPI_ERR_GROUP;

	/* We can't free permanent objects */
	if (grp->isPermanent==1) return MPI_ERR_PERM_GROUP;

	if (grp->ref_count<=1) {
		grp->cookie=0;
		free(grp->lrank2grank);
		free(grp);
	}
	else
		grp->ref_count--;

	*group_ptr=MPI_GROUP_NULL;

	return MPI_SUCCESS;
}
