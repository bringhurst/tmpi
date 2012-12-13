#ifndef _TMPI_NAME_H_
#define _TMPI_NAME_H_

/* set the name space */
# define MPI_Buffer_attach tmpi_Buffer_attach
# define MPI_Buffer_detach tmpi_Buffer_detach

# define MPI_Send tmpi_Send
# define MPI_Isend tmpi_Isend
# define MPI_Send_init tmpi_Send_init
# define MPI_Bsend tmpi_Bsend
# define MPI_Ibsend tmpi_Ibsend
# define MPI_Bsend_init tmpi_Bsend_init
# define MPI_Ssend tmpi_Ssend
# define MPI_Issend tmpi_Issend
# define MPI_Ssend_init tmpi_Ssend_init
# define MPI_Rsend tmpi_Rsend
# define MPI_Irsend tmpi_Irsend
# define MPI_Rsend_init tmpi_Rsend_init

# define MPI_Recv tmpi_Recv
# define MPI_Irecv tmpi_Irecv
# define MPI_Recv_init tmpi_Recv_init

# define MPI_Wait tmpi_Wait

# define MPI_Test tmpi_Test
# define MPI_Request_free tmpi_Request_free
# define MPI_Waitany tmpi_Waitany
# define MPI_Testany tmpi_Testany
# define MPI_Waitall tmpi_Waitall
# define MPI_Testall tmpi_Testall
# define MPI_Waitsome tmpi_Waitsome
# define MPI_Testsome tmpi_Testsome
# define MPI_Iprobe tmpi_Iprobe
# define MPI_Probe tmpi_Probe
# define MPI_Cancel tmpi_Cancel
# define MPI_Test_cancelled tmpi_Test_cancelled
# define MPI_Start tmpi_Start
# define MPI_Startall tmpi_Startall

# define MPI_Sendrecv tmpi_Sendrecv
# define MPI_Sendrecv_replace tmpi_Sendrecv_replace

# define MPI_Type_contiguous tmpi_Type_contiguous
# define MPI_Type_vector tmpi_Type_vector
# define MPI_Type_hvector tmpi_Type_hvector
# define MPI_Type_indexed tmpi_Type_indexed
# define MPI_Type_hindexed tmpi_Type_hindexed
# define MPI_Type_struct tmpi_Type_struct
# define MPI_Address tmpi_Address
# define MPI_Type_extent tmpi_Type_extent

# define MPI_Type_size tmpi_Type_size
# define MPI_Type_count tmpi_Type_count
# define MPI_Type_lb tmpi_Type_lb
# define MPI_Type_ub tmpi_Type_ub
# define MPI_Type_commit tmpi_Type_commit
# define MPI_Type_free tmpi_Type_free
# define MPI_Get_elements tmpi_Get_elements
# define MPI_Pack tmpi_Pack
# define MPI_Unpack tmpi_Unpack
# define MPI_Pack_size tmpi_Pack_size

# define MPI_Barrier tmpi_Barrier

# define MPI_Bcast tmpi_Bcast
# define MPI_Gather tmpi_Gather
# define MPI_Gatherv tmpi_Gatherv
# define MPI_Scatter tmpi_Scatter
# define MPI_Scatterv tmpi_Scatterv
# define MPI_Allgather tmpi_Allgather
# define MPI_Allgatherv tmpi_Allgatherv
# define MPI_Alltoall tmpi_Alltoall
# define MPI_Alltoallv tmpi_Alltoallv
# define MPI_Reduce tmpi_Reduce
# define MPI_Op_create tmpi_Op_create
# define MPI_Op_free tmpi_Op_free
# define MPI_Allreduce tmpi_Allreduce
# define MPI_Reduce_scatter tmpi_Reduce_scatter
# define MPI_Scan tmpi_Scan

# define MPI_Group_size tmpi_Group_size
# define MPI_Group_rank tmpi_Group_rank
# define MPI_Group_translate_ranks tmpi_Group_translate_ranks
# define MPI_Group_compare tmpi_Group_compare
# define MPI_Comm_group tmpi_Comm_group
# define MPI_Group_union tmpi_Group_union
# define MPI_Group_intersection tmpi_Group_intersection
# define MPI_Group_difference tmpi_Group_difference
# define MPI_Group_incl tmpi_Group_incl
# define MPI_Group_excl tmpi_Group_excl
# define MPI_Group_range_incl tmpi_Group_range_incl
# define MPI_Group_range_excl tmpi_Group_range_excl
# define MPI_Group_free tmpi_Group_free

# define MPI_Comm_size tmpi_Comm_size
# define MPI_Comm_rank tmpi_Comm_rank
# define MPI_Comm_compare tmpi_Comm_compare

# define MPI_Comm_dup tmpi_Comm_dup
# define MPI_Comm_create tmpi_Comm_create
# define MPI_Comm_split tmpi_Comm_split
# define MPI_Comm_free tmpi_Comm_free
# define MPI_Comm_test_inter tmpi_Comm_test_inter
# define MPI_Comm_remote_size tmpi_Comm_remote_size
# define MPI_Comm_remote_group tmpi_Comm_remote_group
# define MPI_Intercomm_create tmpi_Intercomm_create
# define MPI_Intercomm_merge tmpi_Intercomm_merge
# define MPI_Keyval_create tmpi_Keyval_create
# define MPI_Keyval_free tmpi_Keyval_free
# define MPI_Attr_put tmpi_Attr_put
# define MPI_Attr_get tmpi_Attr_get
# define MPI_Attr_delete tmpi_Attr_delete
# define MPI_Topo_test tmpi_Topo_test
# define MPI_Cart_create tmpi_Cart_create
# define MPI_Dims_create tmpi_Dims_create
# define MPI_Graph_create tmpi_Graph_create
# define MPI_Graphdims_get tmpi_Graphdims_get
# define MPI_Graph_get tmpi_Graph_get
# define MPI_Cartdim_get tmpi_Cartdim_get
# define MPI_Cart_get tmpi_Cart_get
# define MPI_Cart_rank tmpi_Cart_rank
# define MPI_Cart_coords tmpi_Cart_coords
# define MPI_Graph_neighbors_count tmpi_Graph_neighbors_count
# define MPI_Graph_neighbors tmpi_Graph_neighbors
# define MPI_Cart_shift tmpi_Cart_shift
# define MPI_Cart_sub tmpi_Cart_sub
# define MPI_Cart_map tmpi_Cart_map
# define MPI_Graph_map tmpi_Graph_map
# define MPI_Get_processor_name tmpi_Get_processor_name
# define MPI_Errhandler_create tmpi_Errhandler_create
# define MPI_Errhandler_set tmpi_Errhandler_set
# define MPI_Errhandler_get tmpi_Errhandler_get
# define MPI_Errhandler_free tmpi_Errhandler_free
# define MPI_Error_string tmpi_Error_string
# define MPI_Error_class tmpi_Error_class

# define MPI_Wtime tmpi_Wtime
# define MPI_Wtick tmpi_Wtick
# define MPI_Init tmpi_Init
# define MPI_Finalize tmpi_Finalize
# define MPI_Initialized tmpi_Initialized
# define MPI_Abort tmpi_Abort
# define MPI_Get_count tmpi_Get_count

# define MPI_Comm_set_name tmpi_Comm_set_name
# define MPI_Comm_get_name tmpi_Comm_get_name

# define MPI_Pcontrol tmpi_Pcontrol

# define MPI_Status tmpi_Status
# define MPI_Request tmpi_Request
# define MPI_Errhandler tmpi_Errhandler
# define MPI_Op tmpi_Op
# define MPI_Datatype tmpi_Datatype
# define MPI_Aint tmpi_Aint

# define main tmain
#endif
