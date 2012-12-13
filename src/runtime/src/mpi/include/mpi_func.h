#ifndef __MPI_FUNC_H_
#define __MPI_FUNC_H_

# include "mpi_struct.h"
# include "tmpi_name.h"

/* defined in utils/mbuffer.c */
int MPI_Buffer_attach( void*, int);
int MPI_Buffer_detach( void*, int*);

/* defined in pt2pt/b_sr.c */
int MPI_Send(void*, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
int MPI_Bsend(void*, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Ssend(void*, int, MPI_Datatype, int, int, MPI_Comm);
int MPI_Rsend(void*, int, MPI_Datatype, int, int, MPI_Comm);

/* defined in pt2pt/nb_sr.c */
int MPI_Isend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Ibsend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Issend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Irsend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Irecv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Wait(MPI_Request *, MPI_Status *);

int MPI_Test(MPI_Request *, int *, MPI_Status *);
int MPI_Request_free(MPI_Request *);
int MPI_Waitany(int, MPI_Request *, int *, MPI_Status *);
int MPI_Testany(int, MPI_Request *, int *, int *, MPI_Status *);
int MPI_Waitall(int, MPI_Request *, MPI_Status *);
int MPI_Testall(int, MPI_Request *, int *, MPI_Status *);
int MPI_Waitsome(int, MPI_Request *, int *, int *, MPI_Status *);
int MPI_Testsome(int, MPI_Request *, int *, int *, MPI_Status *);
int MPI_Iprobe(int, int, MPI_Comm, int *flag, MPI_Status *);
int MPI_Probe(int, int, MPI_Comm, MPI_Status *);
int MPI_Cancel(MPI_Request *);
int MPI_Test_cancelled(MPI_Status *, int *flag);
int MPI_Send_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
int MPI_Bsend_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
int MPI_Ssend_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
int MPI_Rsend_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
int MPI_Recv_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
int MPI_Start(MPI_Request *);
int MPI_Startall(int, MPI_Request *);

/* defined in pt2pt/sendrecv.c */
int MPI_Sendrecv(void *, int, MPI_Datatype,int, int, void *, int, 
		 MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
int MPI_Sendrecv_replace(void*, int, MPI_Datatype, 
			 int, int, int, int, MPI_Comm, MPI_Status *);

int MPI_Type_contiguous(int, MPI_Datatype, MPI_Datatype *);
int MPI_Type_vector(int, int, int, MPI_Datatype, MPI_Datatype *);
int MPI_Type_hvector(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
int MPI_Type_indexed(int, int *, int *, MPI_Datatype, MPI_Datatype *);
int MPI_Type_hindexed(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
int MPI_Type_struct(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
int MPI_Address(void*, MPI_Aint *);
int MPI_Type_extent(MPI_Datatype, MPI_Aint *);

/* See the 1.1 version of the Standard; I think that the standard is in 
   error; however, it is the standard */
/* int MPI_Type_size(MPI_Datatype, MPI_Aint *size); */
int MPI_Type_size(MPI_Datatype, int *);
int MPI_Type_count(MPI_Datatype, int *);
int MPI_Type_lb(MPI_Datatype, MPI_Aint*);
int MPI_Type_ub(MPI_Datatype, MPI_Aint*);
int MPI_Type_commit(MPI_Datatype *);
int MPI_Type_free(MPI_Datatype *);
int MPI_Get_elements(MPI_Status *, MPI_Datatype, int *);
int MPI_Pack(void* inbuf, int, MPI_Datatype, void *outbuf, 
	     int outsize, int *position,  MPI_Comm);
int MPI_Unpack(void* inbuf, int insize, int *position, void *outbuf, 
	       int , MPI_Datatype, MPI_Comm);
int MPI_Pack_size(int, MPI_Datatype, MPI_Comm, 
		  int *);

/* defined in context/barrier.c */
int MPI_Barrier(MPI_Comm );

int MPI_Bcast(void*, int, MPI_Datatype, int root, MPI_Comm );
int MPI_Gather(void*, int, MPI_Datatype, 
	       void*, int, MPI_Datatype, 
	       int root, MPI_Comm); 
int MPI_Gatherv(void* , int, MPI_Datatype, 
		void*, int *recvcounts, int *displs, 
		MPI_Datatype, int root, MPI_Comm); 
int MPI_Scatter(void* , int, MPI_Datatype, 
		void*, int, MPI_Datatype, 
		int root, MPI_Comm);
int MPI_Scatterv(void* , int *sendcounts, int *displs, 
		 MPI_Datatype, void*, int, 
		 MPI_Datatype, int root, MPI_Comm);
int MPI_Allgather(void* , int, MPI_Datatype, 
		  void*, int, MPI_Datatype, 
		  MPI_Comm);
int MPI_Allgatherv(void* , int, MPI_Datatype, 
		   void*, int *recvcounts, int *displs, 
		   MPI_Datatype, MPI_Comm);
int MPI_Alltoall(void* , int, MPI_Datatype, 
		 void*, int, MPI_Datatype, 
		 MPI_Comm);
int MPI_Alltoallv(void* , int *sendcounts, int *sdispls, 
		  MPI_Datatype, void*, int *recvcounts, 
		  int *rdispls, MPI_Datatype, MPI_Comm);
int MPI_Reduce(void* , void*, int, 
	       MPI_Datatype, MPI_Op, int, MPI_Comm);
int MPI_Op_create(MPI_User_function *, int, MPI_Op *);
int MPI_Op_free( MPI_Op *);
int MPI_Allreduce(void* , void*, int, 
		  MPI_Datatype, MPI_Op, MPI_Comm);
int MPI_Reduce_scatter(void* , void*, int *recvcounts, 
		       MPI_Datatype, MPI_Op op, MPI_Comm);
int MPI_Scan(void* , void*, int, MPI_Datatype, 
	     MPI_Op op, MPI_Comm );

/* defined in context/group.c */
int MPI_Group_size(MPI_Group group, int *);
int MPI_Group_rank(MPI_Group group, int *rank);
int MPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1, 
			       MPI_Group group2, int *ranks2);
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);
int MPI_Comm_group(MPI_Comm, MPI_Group *);
int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, 
			   MPI_Group *newgroup);
int MPI_Group_difference(MPI_Group group1, MPI_Group group2, 
			 MPI_Group *newgroup);
int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup);
int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup);
int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], 
			 MPI_Group *newgroup);
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], 
			 MPI_Group *newgroup);
int MPI_Group_free(MPI_Group *);

/* defined in context/comm.c */
int MPI_Comm_size(MPI_Comm, int *);
int MPI_Comm_rank(MPI_Comm, int *rank);
int MPI_Comm_compare(MPI_Comm, MPI_Comm, int *result);

int MPI_Comm_dup(MPI_Comm, MPI_Comm *newcomm);
int MPI_Comm_create(MPI_Comm, MPI_Group group, MPI_Comm *newcomm);
int MPI_Comm_split(MPI_Comm, int color, int key, MPI_Comm *newcomm);
int MPI_Comm_free(MPI_Comm *comm);
int MPI_Comm_test_inter(MPI_Comm, int *flag);
int MPI_Comm_remote_size(MPI_Comm, int *);
int MPI_Comm_remote_group(MPI_Comm, MPI_Group *);
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
			 MPI_Comm peer_comm, int remote_leader, 
			 int, MPI_Comm *newintercomm);
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
int MPI_Keyval_create(MPI_Copy_function *copy_fn, 
		      MPI_Delete_function *delete_fn, 
		      int *keyval, void* extra_state);
int MPI_Keyval_free(int *keyval);
int MPI_Attr_put(MPI_Comm, int keyval, void* attribute_val);
int MPI_Attr_get(MPI_Comm, int keyval, void *attribute_val, int *flag);
int MPI_Attr_delete(MPI_Comm, int keyval);
int MPI_Topo_test(MPI_Comm, int *);
int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods,
		    int reorder, MPI_Comm *comm_cart);
int MPI_Dims_create(int nnodes, int ndims, int *dims);
int MPI_Graph_create(MPI_Comm, int, int *, int *, int, MPI_Comm *);
int MPI_Graphdims_get(MPI_Comm, int *nnodes, int *nedges);
int MPI_Graph_get(MPI_Comm, int, int, int *, int *);
int MPI_Cartdim_get(MPI_Comm, int *ndims);
int MPI_Cart_get(MPI_Comm, int maxdims, int *dims, int *periods,
		 int *coords);
int MPI_Cart_rank(MPI_Comm, int *coords, int *rank);
int MPI_Cart_coords(MPI_Comm, int rank, int maxdims, int *coords);
int MPI_Graph_neighbors_count(MPI_Comm, int rank, int *nneighbors);
int MPI_Graph_neighbors(MPI_Comm, int rank, int maxneighbors,
			int *neighbors);
int MPI_Cart_shift(MPI_Comm, int direction, int disp, 
		   int *rank_source, int *rank_dest);
int MPI_Cart_sub(MPI_Comm, int *remain_dims, MPI_Comm *newcomm);
int MPI_Cart_map(MPI_Comm, int ndims, int *dims, int *periods, 
		 int *newrank);
int MPI_Graph_map(MPI_Comm, int, int *, int *, int *);
int MPI_Get_processor_name(char *name, int *result_len);
int MPI_Errhandler_create(MPI_Handler_function *function, 
			  MPI_Errhandler *errhandler);
int MPI_Errhandler_set(MPI_Comm, MPI_Errhandler errhandler);
int MPI_Errhandler_get(MPI_Comm, MPI_Errhandler *errhandler);
int MPI_Errhandler_free(MPI_Errhandler *errhandler);
int MPI_Error_string(int errorcode, char *string, int *result_len);
int MPI_Error_class(int errorcode, int *errorclass);

/* defined in utils/utils.c */
double MPI_Wtime(void);
double MPI_Wtick(void);
int MPI_Init(int *argc, char ***argv);
int MPI_Finalize(void);
int MPI_Initialized(int *flag);
int MPI_Abort(MPI_Comm, int errorcode);
int MPI_Get_count(MPI_Status *, MPI_Datatype, int *);

/* MPI-2 communicator naming functions */
int MPI_Comm_set_name(MPI_Comm, char *);
int MPI_Comm_get_name(MPI_Comm, char **);
#ifdef HAVE_NO_C_CONST
/* Default Solaris compiler does not accept const but does accept prototypes */
int MPI_Pcontrol(int level, ...);
#else
int MPI_Pcontrol(const int level, ...);
#endif

int MPI_NULL_COPY_FN ( MPI_Comm oldcomm, int keyval, void *extra_state, 
		       void *attr_in, void *attr_out, int *flag );
int MPI_NULL_DELETE_FN ( MPI_Comm, int keyval, void *attr, 
			 void *extra_state );
int MPI_DUP_FN ( MPI_Comm, int keyval, void *extra_state, void *attr_in,
		 void *attr_out, int *flag );

/* The following is not part of MPI */
/* wrapping functions for ukey lib */
int thrMPI_createkey(void);
void thrMPI_setval(int, void *);
void *thrMPI_getval(int);

#endif
