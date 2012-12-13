#ifndef _MPI_STRUCT_H_
#define _MPI_STRUCT_H_

# include "tmpi_name.h"
/** size large enough to hold an address */
typedef long MPI_Aint;

/* Data types
 * A more aggressive yet homogeneous implementation might want to 
 * make the values here the number of bytes in the basic type, with
 * a simple test against a max limit (e.g., 16 for long double), and
 * non-contiguous structures with indices greater than that.
 */
typedef int MPI_Datatype;
#define MPI_CHAR           ((MPI_Datatype)1)
#define MPI_UNSIGNED_CHAR  ((MPI_Datatype)2)
#define MPI_BYTE           ((MPI_Datatype)3)
#define MPI_SHORT          ((MPI_Datatype)4)
#define MPI_UNSIGNED_SHORT ((MPI_Datatype)5)
#define MPI_INT            ((MPI_Datatype)6)
#define MPI_UNSIGNED       ((MPI_Datatype)7)
#define MPI_LONG           ((MPI_Datatype)8)
#define MPI_UNSIGNED_LONG  ((MPI_Datatype)9)
#define MPI_FLOAT          ((MPI_Datatype)10)
#define MPI_DOUBLE         ((MPI_Datatype)11)
#define MPI_LONG_DOUBLE    ((MPI_Datatype)12)
#define MPI_LONG_LONG_INT  ((MPI_Datatype)13)

#define MPI_PACKED         ((MPI_Datatype)14)
#define MPI_LB             ((MPI_Datatype)15)
#define MPI_UB             ((MPI_Datatype)16)

#define MPI_FLOAT_INT      ((MPI_Datatype)17)
#define MPI_DOUBLE_INT     ((MPI_Datatype)18)
#define MPI_LONG_INT       ((MPI_Datatype)19)
#define MPI_SHORT_INT      ((MPI_Datatype)20)
#define MPI_2INT           ((MPI_Datatype)21)
#define MPI_LONG_DOUBLE_INT ((MPI_Datatype)22)

/* Fortran types */
#define MPI_COMPLEX           ((MPI_Datatype)23)
#define MPI_DOUBLE_COMPLEX    ((MPI_Datatype)24)
#define MPI_LOGICAL           ((MPI_Datatype)25)
#define MPI_REAL              ((MPI_Datatype)26)
#define MPI_DOUBLE_PRECISION  ((MPI_Datatype)27)
#define MPI_INTEGER           ((MPI_Datatype)28)
#define MPI_2INTEGER          ((MPI_Datatype)29)
#define MPI_2COMPLEX          ((MPI_Datatype)30)
#define MPI_2DOUBLE_COMPLEX   ((MPI_Datatype)31)
#define MPI_2REAL             ((MPI_Datatype)32)
#define MPI_2DOUBLE_PRECISION ((MPI_Datatype)33)
#define MPI_CHARACTER         ((MPI_Datatype)1)

/* Communicators */
typedef struct tmpi_Comm *MPI_Comm;
#define MPI_COMM_WORLD ((MPI_Comm)1)
#define MPI_COMM_SELF ((MPI_Comm)2)

/* function types */
typedef void (MPI_User_function) (void *, void *, int *, MPI_Datatype *);
typedef void (MPI_Copy_function) (MPI_Comm, int, void *, void *, void *, int *);
typedef void (MPI_Delete_function) (MPI_Comm, int, void *, void *);
typedef void (MPI_Handler_function) (MPI_Comm *, int *, ...);

/* Groups */
typedef struct tmpi_Group *MPI_Group;
#define MPI_GROUP_EMPTY ((MPI_Group)1)

/* Collective operations */
typedef int MPI_Op;

#define MPI_MAX    ((MPI_Op)1)
#define MPI_MIN    ((MPI_Op)2)
#define MPI_SUM    ((MPI_Op)3)
#define MPI_PROD   ((MPI_Op)4)
#define MPI_LAND   ((MPI_Op)5)
#define MPI_BAND   ((MPI_Op)6)
#define MPI_LOR    ((MPI_Op)7)
#define MPI_BOR    ((MPI_Op)8)
#define MPI_LXOR   ((MPI_Op)9)
#define MPI_BXOR   ((MPI_Op)10)
#define MPI_MINLOC ((MPI_Op)11)
#define MPI_MAXLOC ((MPI_Op)12)

/* Define some null objects */
#define MPI_COMM_NULL       ((MPI_Comm)0)
#define MPI_OP_NULL         ((MPI_Op)0)
#define MPI_GROUP_NULL      ((MPI_Group)0)
#define MPI_DATATYPE_NULL   ((MPI_Datatype)0)
#define MPI_REQUEST_NULL    ((MPI_Request)0)
#define MPI_ERRHANDLER_NULL ((MPI_Errhandler )0)

/* 
   Status object.  It is the only user-visible MPI data-structure 
   The "count" field is PRIVATE; use MPI_Get_count to access it. 
 */
typedef struct { 
    /* int count; */
    int buflen;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
} MPI_Status;

typedef int MPI_Errhandler;
#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler)119)
#define MPI_ERRORS_RETURN    ((MPI_Errhandler)120)
#define MPIR_ERRORS_WARN     ((MPI_Errhandler)121)

/* MPI request opjects */
typedef union tmpi_Handle *MPI_Request;

#endif
