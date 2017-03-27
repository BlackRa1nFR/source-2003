/*
 *  $Id: mpi.h,v 1.19 2002/03/11 10:30:17 joachim Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

/* user include file for MPI programs */

#ifndef _MPI_INCLUDE
#define _MPI_INCLUDE

/* 
   NEW_POINTERS enables using ints for the MPI objects instead of addresses.
   This is required by MPI 1.1, particularly for Fortran (also for C, 
   since you can now have "constant" values for things like MPI_INT, and
   they are stronger than "constant between MPI_INIT and MPI_FINALIZE".
   For example, we might want to allow MPI_INT as a case label.

   NEW_POINTERS is the default; I'm purging all code that doesn't use
   NEW_POINTERS from the source.
 */

/* The following definitions are used to support a single source tree for
   both Unix and Windows NT versions of MPICH.  For Unix users, the 
   definition is empty.
  Currently we cannot use __declspec(dllimport) since this will break the
  profiling interface. If used, the linker would always link the functions from the dll,
  not from the wrapper. So we leave IMPORT_MPI_API empty.
*/
#if defined(WIN32_IMPORT_DLL) && !defined(IN_MPICH_DLL) && !defined(MPID_NO_IMPORT)
#define IMPORT_MPI_API __declspec(dllimport)
#else
#define IMPORT_MPI_API
#endif
#define EXPORT_MPI_API

#define NEW_POINTERS

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

/* We require that the C compiler support prototypes */
#define MPIR_ARGS(a) a

 /* Results of the compare operations */
/* These should stay ordered */
#define MPI_IDENT     0
#define MPI_CONGRUENT 1
#define MPI_SIMILAR   2
#define MPI_UNEQUAL   3

/* Data types
 * A more aggressive yet homogeneous implementation might want to 
 * make the values here the number of bytes in the basic type, with
 * a simple test against a max limit (e.g., 16 for long double), and
 * non-contiguous structures with indices greater than that.
 * 
 * Note: Configure knows these values for providing the Fortran optional
 * types (like MPI_REAL8).  Any changes here must be matched by changes
 * in configure.in
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
/* MPI_LONG_LONG is in the complete ref 2nd edition, though not in the 
   standard.  Rather, MPI_LONG_LONG_INT is on page 40 in the HPCA version */
#define MPI_LONG_LONG      ((MPI_Datatype)13)

#define MPI_PACKED         ((MPI_Datatype)14)
#define MPI_LB             ((MPI_Datatype)15)
#define MPI_UB             ((MPI_Datatype)16)

/* 
   The layouts for the types MPI_DOUBLE_INT etc are simply
   struct { 
       double var;
       int    loc;
   }
   This is documented in the man pages on the various datatypes.   
 */
#define MPI_FLOAT_INT         ((MPI_Datatype)17)
#define MPI_DOUBLE_INT        ((MPI_Datatype)18)
#define MPI_LONG_INT          ((MPI_Datatype)19)
#define MPI_SHORT_INT         ((MPI_Datatype)20)
#define MPI_2INT              ((MPI_Datatype)21)
#define MPI_LONG_DOUBLE_INT   ((MPI_Datatype)22)

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
typedef int MPI_Comm;
  /* META */
#ifndef META
#define MPI_COMM_WORLD 91
#endif
#define MPI_COMM_SELF  92

#ifdef META
#define MPI_COMM_ALL 91
#define MPI_COMM_META 93
/* make sure that all use of MPI_COMM_WORLD in the MPI-Application
   use MPI_COMM_META instead */
#define MPI_COMM_WORLD 93

/* for the META-Environment, we need different communicators 
   on each host. This is done by giving them different indexes
   on each host - I hope this works! Just take care that
   META_MPI_MAX_HOSTS is not bigger than the available index
   space.*/
#define MPI_COMM_HOST_BASE 150
#define MPI_COMM_LOCAL_BASE 166
extern IMPORT_MPI_API int MPI_COMM_HOST;
extern IMPORT_MPI_API int MPI_COMM_LOCAL;
#endif
  /* /META */

/* Groups */
typedef int MPI_Group;
#define MPI_GROUP_EMPTY 90

/* Windows */
typedef int MPI_Win;		
#define MPI_WIN_NULL ((MPI_Win) 0)
#define MPI_WIN_BASE		1
#define MPI_WIN_SIZE		2
#define MPI_WIN_DISP_UNIT	3
/* winlocks */
#define MPI_LOCK_EXCLUSIVE	1
#define MPI_LOCK_SHARED		2

/* Collective operations */
typedef int MPI_Op;

#define MPI_MAX		(MPI_Op)(100)
#define MPI_MIN		(MPI_Op)(101)
#define MPI_SUM		(MPI_Op)(102)
#define MPI_PROD	(MPI_Op)(103)
#define MPI_LAND	(MPI_Op)(104)
#define MPI_BAND	(MPI_Op)(105)
#define MPI_LOR		(MPI_Op)(106)
#define MPI_BOR		(MPI_Op)(107)
#define MPI_LXOR	(MPI_Op)(108)
#define MPI_BXOR	(MPI_Op)(109)
#define MPI_MINLOC	(MPI_Op)(110)
#define MPI_MAXLOC	(MPI_Op)(111)
#define MPI_REPLACE (MPI_Op)(112)
#define MPI_DIV		(MPI_Op)(113)
#define MPI_MOD		(MPI_Op)(114)

/* Permanent key values */
/* C Versions (return pointer to value) */
#define MPI_TAG_UB           81
#define MPI_HOST             83
#define MPI_IO               85
#define MPI_WTIME_IS_GLOBAL  87
/* Fortran Versions (return integer value) */
#define MPIR_TAG_UB          80
#define MPIR_HOST            82
#define MPIR_IO              84
#define MPIR_WTIME_IS_GLOBAL 86

/* Define some null objects */
#define MPI_COMM_NULL      ((MPI_Comm)0)
#define MPI_OP_NULL        ((MPI_Op)0)
#define MPI_GROUP_NULL     ((MPI_Group)0)
#define MPI_DATATYPE_NULL  ((MPI_Datatype)0)
#define MPI_REQUEST_NULL   ((MPI_Request)0)
#define MPI_ERRHANDLER_NULL ((MPI_Errhandler )0)

/* These are only guesses; make sure you change them in mpif.h as well */
#define MPI_MAX_PROCESSOR_NAME 256
#define MPI_MAX_ERROR_STRING   512
#define MPI_MAX_NAME_STRING     63		/* How long a name do you need ? */

/* Pre-defined constants */
#define MPI_UNDEFINED      (-32766)
#define MPI_UNDEFINED_RANK MPI_UNDEFINED
#define MPI_KEYVAL_INVALID 0

/* Upper bound on the overhead in bsend for each message buffer */
#define MPI_BSEND_OVERHEAD 512

/* Topology types */
#define MPI_GRAPH  1
#define MPI_CART   2

#define MPI_BOTTOM      (void *)0

#define MPI_PROC_NULL   (-1)
#define MPI_ANY_SOURCE 	(-2)
#define MPI_ROOT        (-3)
#define MPI_ANY_TAG	(-1)

/* 
   Status object.  It is the only user-visible MPI data-structure 
   The "count" field is PRIVATE; use MPI_Get_count to access it. 
 */
typedef struct { 
    int count;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int private_count;
} MPI_Status;
/* MPI_STATUS_SIZE is not strictly required in C; however, it should match
   the value for Fortran */
#define MPI_STATUS_SIZE 5

/* Must be able to hold any valid address.  64 bit machines may need
   to change this */
/* #if (defined(_SX) && !defined(_LONG64)) */
/* NEC SX-4 in some modes needs this */
/* typedef long long MPI_Aint; */
/* #else */
/* typedef long MPI_Aint; */
/* #endif */


/* MPI Error handlers.  Systems that don't support stdargs can't use
   this definition
 */
#if defined(USE_STDARG) 
typedef void (MPI_Handler_function)  ( MPI_Comm *, int *, ... );
typedef void (MPI_Win_errhandler_fn) ( MPI_Win *, int *, ...);
#else
typedef void (MPI_Handler_function) ();
typedef void (MPI_Win_errhandler_fn) ();
#endif
/* For MPI 2 error handling */
typedef MPI_Handler_function MPI_Comm_errhandler_fn;

#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler)119)
#define MPI_ERRORS_RETURN    ((MPI_Errhandler)120)
#define MPIR_ERRORS_WARN     ((MPI_Errhandler)121)
typedef int MPI_Errhandler;
/* Make the C names for the null functions all upper-case.  Note that 
   this is required for systems that use all uppercase names for Fortran 
   externals.  */
/* MPI 1 names */
#define MPI_NULL_COPY_FN   MPIR_null_copy_fn
#define MPI_NULL_DELETE_FN MPIR_null_delete_fn
#define MPI_DUP_FN         MPIR_dup_fn
/* MPI 2 names */
#define MPI_COMM_NULL_COPY_FN MPI_NULL_COPY_FN
#define MPI_COMM_NULL_DELETE_FN MPI_NULL_DELETE_FN
#define MPI_COMM_DUP_FN MPI_DUP_FN

/* MPI request opjects */
typedef union MPIR_HANDLE *MPI_Request;

/* User combination function */
typedef void (MPI_User_function) ( void *, void *, int *, MPI_Datatype * ); 

/* MPI Attribute copy and delete functions */
typedef int (MPI_Copy_function) ( MPI_Comm, int, void *, void *, void *, int * );
typedef int (MPI_Delete_function) ( MPI_Comm, int, void *, void * );

#define MPI_VERSION    1
#define MPI_SUBVERSION 2
#define MPICH_NAME     1

/********************** MPI-2 FEATURES BEGIN HERE ***************************/
#define MPICH_HAS_C2F

/* for the datatype decoders */
#define MPI_COMBINER_NAMED         2312
#define MPI_COMBINER_CONTIGUOUS    2313
#define MPI_COMBINER_VECTOR        2314
#define MPI_COMBINER_HVECTOR       2315
#define MPI_COMBINER_INDEXED       2316
#define MPI_COMBINER_HINDEXED      2317
#define MPI_COMBINER_STRUCT        2318

/* for info */
typedef struct MPIR_Info *MPI_Info;
# define MPI_INFO_NULL         ((MPI_Info) 0)
# define MPI_MAX_INFO_KEY       255
# define MPI_MAX_INFO_VAL      1024

/* for subarray and darray constructors */
#define MPI_ORDER_C              56
#define MPI_ORDER_FORTRAN        57
#define MPI_DISTRIBUTE_BLOCK    121
#define MPI_DISTRIBUTE_CYCLIC   122
#define MPI_DISTRIBUTE_NONE     123
#define MPI_DISTRIBUTE_DFLT_DARG -49767

/* mpidefs.h includes configuration-specific information, such as the 
   type of MPI_Aint or MPI_Fint, also mpio.h, if it was built */
#include "mpidefs.h"

/* Handle conversion types/functions */

/* Programs that need to convert types used in MPICH should use these */
#define MPI_Comm_c2f(comm) (MPI_Fint)(comm)
#define MPI_Comm_f2c(comm) (MPI_Comm)(comm)
#define MPI_Type_c2f(datatype) (MPI_Fint)(datatype)
#define MPI_Type_f2c(datatype) (MPI_Datatype)(datatype)
#define MPI_Group_c2f(group) (MPI_Fint)(group)
#define MPI_Group_f2c(group) (MPI_Group)(group)
/* MPI_Request_c2f is a routine in src/misc2 */
#define MPI_Request_f2c(request) (MPI_Request)MPIR_ToPointer(request)
#define MPI_Op_c2f(op) (MPI_Fint)(op)
#define MPI_Op_f2c(op) (MPI_Op)(op)
#define MPI_Errhandler_c2f(errhandler) (MPI_Fint)(errhandler)
#define MPI_Errhandler_f2c(errhandler) (MPI_Errhandler)(errhandler)

/* For new MPI-2 types */
#define MPI_Win_c2f(win)   (MPI_Fint)(win)
#define MPI_Win_f2c(win)   (MPI_Win)(win)

#define MPI_STATUS_IGNORE (MPI_Status *)0
#define MPI_STATUSES_IGNORE (MPI_Status *)0

/* For supported thread levels */
#define MPI_THREAD_SINGLE 0
#define MPI_THREAD_FUNNELED 1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE 3

/********************** MPI-2 FEATURES END HERE ***************************/

/*************** Experimental MPICH FEATURES BEGIN HERE *******************/
/* Attribute keys.  These are set to MPI_KEYVAL_INVALID by default */
extern int MPICHX_QOS_BANDWIDTH;
/**************** Experimental MPICH FEATURES END HERE ********************/

/* MPI's error classes */
#include "mpi_errno.h"
/* End of MPI's error classes */




#if defined(WIN32_FORTRAN_STDCALL) &&!defined(FORTRAN_API)
/* This is used on Windows to distinguish between
  FORTRAN compilers that use STDCALL convention ond those
  that use C convention. */
#define FORTRAN_API __stdcall
#else
#define FORTRAN_API
#endif

#ifndef _NORETURN
#if (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#define _NORETURN   __declspec(noreturn)
#else
#define _NORETURN
#endif
#endif


/*
 * Normally, we provide prototypes for all MPI routines.  In a few wierd
 * cases, we need to suppress the prototypes.
 */
#ifndef MPICH_SUPPRESS_PROTOTYPES
/* We require that the C compiler support prototypes */
IMPORT_MPI_API int MPI_Send(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int MPI_Recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int MPI_Get_count(MPI_Status *, MPI_Datatype, int *);
IMPORT_MPI_API int MPI_Bsend(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int MPI_Ssend(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int MPI_Rsend(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int MPI_Buffer_attach( void*, int);
IMPORT_MPI_API int MPI_Buffer_detach( void*, int*);
IMPORT_MPI_API int MPI_Isend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Ibsend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Issend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Irsend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Irecv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Wait(MPI_Request *, MPI_Status *);
IMPORT_MPI_API int MPI_Test(MPI_Request *, int *, MPI_Status *);
IMPORT_MPI_API int MPI_Request_free(MPI_Request *);
IMPORT_MPI_API int MPI_Waitany(int, MPI_Request *, int *, MPI_Status *);
IMPORT_MPI_API int MPI_Testany(int, MPI_Request *, int *, int *, MPI_Status *);
IMPORT_MPI_API int MPI_Waitall(int, MPI_Request *, MPI_Status *);
IMPORT_MPI_API int MPI_Testall(int, MPI_Request *, int *, MPI_Status *);
IMPORT_MPI_API int MPI_Waitsome(int, MPI_Request *, int *, int *, MPI_Status *);
IMPORT_MPI_API int MPI_Testsome(int, MPI_Request *, int *, int *, MPI_Status *);
IMPORT_MPI_API int MPI_Iprobe(int, int, MPI_Comm, int *flag, MPI_Status *);
IMPORT_MPI_API int MPI_Probe(int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int MPI_Cancel(MPI_Request *);
IMPORT_MPI_API int MPI_Test_cancelled(MPI_Status *, int *);
IMPORT_MPI_API int MPI_Send_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Bsend_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Ssend_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Rsend_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Recv_init(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int MPI_Start(MPI_Request *);
IMPORT_MPI_API int MPI_Startall(int, MPI_Request *);
IMPORT_MPI_API int MPI_Sendrecv(void *, int, MPI_Datatype,int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int MPI_Sendrecv_replace(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int MPI_Type_contiguous(int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_vector(int, int, int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_hvector(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_indexed(int, int *, int *, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_hindexed(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_struct(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
IMPORT_MPI_API int MPI_Address(void*, MPI_Aint *);
IMPORT_MPI_API int MPI_Type_extent(MPI_Datatype, MPI_Aint *);
/* See the 1.1 version of the Standard; I think that the standard is in 
   error; however, it is the standard */
/* int MPI_Type_size(MPI_Datatype, MPI_Aint *size); */
IMPORT_MPI_API int MPI_Type_size(MPI_Datatype, int *);
/* This one has been removed from MPI.*/
/*IMPORT_MPI_API int MPI_Type_count(MPI_Datatype, int *);*/
IMPORT_MPI_API int MPI_Type_lb(MPI_Datatype, MPI_Aint*);
IMPORT_MPI_API int MPI_Type_ub(MPI_Datatype, MPI_Aint*);
IMPORT_MPI_API int MPI_Type_commit(MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_free(MPI_Datatype *);
IMPORT_MPI_API int MPI_Get_elements(MPI_Status *, MPI_Datatype, int *);
IMPORT_MPI_API int MPI_Pack(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm);
IMPORT_MPI_API int MPI_Unpack(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int MPI_Pack_size(int, MPI_Datatype, MPI_Comm, int *);
IMPORT_MPI_API int MPI_Barrier(MPI_Comm );
IMPORT_MPI_API int MPI_Bcast(void*, int, MPI_Datatype, int, MPI_Comm );
IMPORT_MPI_API int MPI_Gather(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm); 
IMPORT_MPI_API int MPI_Gatherv(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm); 
IMPORT_MPI_API int MPI_Scatter(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
IMPORT_MPI_API int MPI_Scatterv(void* , int *, int *,  MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
IMPORT_MPI_API int MPI_Allgather(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int MPI_Allgatherv(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int MPI_Alltoall(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int MPI_Alltoallv(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int MPI_Reduce(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
IMPORT_MPI_API int MPI_Op_create(MPI_User_function *, int, MPI_Op *);
IMPORT_MPI_API int MPI_Op_free( MPI_Op *);
IMPORT_MPI_API int MPI_Allreduce(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
IMPORT_MPI_API int MPI_Reduce_scatter(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm);
IMPORT_MPI_API int MPI_Scan(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm );
IMPORT_MPI_API int MPI_Group_size(MPI_Group group, int *);
IMPORT_MPI_API int MPI_Group_rank(MPI_Group group, int *);
IMPORT_MPI_API int MPI_Group_translate_ranks (MPI_Group, int, int *, MPI_Group, int *);
IMPORT_MPI_API int MPI_Group_compare(MPI_Group, MPI_Group, int *);
IMPORT_MPI_API int MPI_Comm_group(MPI_Comm, MPI_Group *);
IMPORT_MPI_API int MPI_Group_union(MPI_Group, MPI_Group, MPI_Group *);
IMPORT_MPI_API int MPI_Group_intersection(MPI_Group, MPI_Group, MPI_Group *);
IMPORT_MPI_API int MPI_Group_difference(MPI_Group, MPI_Group, MPI_Group *);
IMPORT_MPI_API int MPI_Group_incl(MPI_Group group, int, int *, MPI_Group *);
IMPORT_MPI_API int MPI_Group_excl(MPI_Group group, int, int *, MPI_Group *);
IMPORT_MPI_API int MPI_Group_range_incl(MPI_Group group, int, int [][3], MPI_Group *);
IMPORT_MPI_API int MPI_Group_range_excl(MPI_Group group, int, int [][3], MPI_Group *);
IMPORT_MPI_API int MPI_Group_free(MPI_Group *);
IMPORT_MPI_API int MPI_Comm_size(MPI_Comm, int *);
IMPORT_MPI_API int MPI_Comm_rank(MPI_Comm, int *);
IMPORT_MPI_API int MPI_Comm_compare(MPI_Comm, MPI_Comm, int *);
IMPORT_MPI_API int MPI_Comm_dup(MPI_Comm, MPI_Comm *);
IMPORT_MPI_API int MPI_Comm_create(MPI_Comm, MPI_Group, MPI_Comm *);
IMPORT_MPI_API int MPI_Comm_split(MPI_Comm, int, int, MPI_Comm *);
IMPORT_MPI_API int MPI_Comm_free(MPI_Comm *);
IMPORT_MPI_API int MPI_Comm_test_inter(MPI_Comm, int *);
IMPORT_MPI_API int MPI_Comm_remote_size(MPI_Comm, int *);
IMPORT_MPI_API int MPI_Comm_remote_group(MPI_Comm, MPI_Group *);
IMPORT_MPI_API int MPI_Intercomm_create(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm * );
IMPORT_MPI_API int MPI_Intercomm_merge(MPI_Comm, int, MPI_Comm *);
IMPORT_MPI_API int MPI_Keyval_create(MPI_Copy_function *, MPI_Delete_function *, int *, void*);
IMPORT_MPI_API int MPI_Keyval_free(int *);
IMPORT_MPI_API int MPI_Attr_put(MPI_Comm, int, void*);
IMPORT_MPI_API int MPI_Attr_get(MPI_Comm, int, void *, int *);
IMPORT_MPI_API int MPI_Attr_delete(MPI_Comm, int);
IMPORT_MPI_API int MPI_Topo_test(MPI_Comm, int *);
IMPORT_MPI_API int MPI_Cart_create(MPI_Comm, int, int *, int *, int, MPI_Comm *);
IMPORT_MPI_API int MPI_Dims_create(int, int, int *);
IMPORT_MPI_API int MPI_Graph_create(MPI_Comm, int, int *, int *, int, MPI_Comm *);
IMPORT_MPI_API int MPI_Graphdims_get(MPI_Comm, int *, int *);
IMPORT_MPI_API int MPI_Graph_get(MPI_Comm, int, int, int *, int *);
IMPORT_MPI_API int MPI_Cartdim_get(MPI_Comm, int *);
IMPORT_MPI_API int MPI_Cart_get(MPI_Comm, int, int *, int *, int *);
IMPORT_MPI_API int MPI_Cart_rank(MPI_Comm, int *, int *);
IMPORT_MPI_API int MPI_Cart_coords(MPI_Comm, int, int, int *);
IMPORT_MPI_API int MPI_Graph_neighbors_count(MPI_Comm, int, int *);
IMPORT_MPI_API int MPI_Graph_neighbors(MPI_Comm, int, int, int *);
IMPORT_MPI_API int MPI_Cart_shift(MPI_Comm, int, int, int *, int *);
IMPORT_MPI_API int MPI_Cart_sub(MPI_Comm, int *, MPI_Comm *);
IMPORT_MPI_API int MPI_Cart_map(MPI_Comm, int, int *, int *, int *);
IMPORT_MPI_API int MPI_Graph_map(MPI_Comm, int, int *, int *, int *);
IMPORT_MPI_API int MPI_Get_processor_name(char *, int *);
IMPORT_MPI_API int MPI_Get_version(int *, int *);
IMPORT_MPI_API int MPI_Errhandler_create(MPI_Handler_function *, MPI_Errhandler *);
IMPORT_MPI_API int MPI_Errhandler_set(MPI_Comm, MPI_Errhandler);
IMPORT_MPI_API int MPI_Errhandler_get(MPI_Comm, MPI_Errhandler *);
IMPORT_MPI_API int MPI_Errhandler_free(MPI_Errhandler *);
IMPORT_MPI_API int MPI_Error_string(int, char *, int *);
IMPORT_MPI_API int MPI_Error_class(int, int *);
IMPORT_MPI_API double MPI_Wtime(void);
IMPORT_MPI_API double MPI_Wtick(void);
#ifndef MPI_Wtime
IMPORT_MPI_API double PMPI_Wtime(void);
IMPORT_MPI_API double PMPI_Wtick(void);
#endif
IMPORT_MPI_API int MPI_Init(int *, char ***);
IMPORT_MPI_API int MPI_Init_thread( int *, char ***, int, int * );
IMPORT_MPI_API int MPI_Finalize(void);
IMPORT_MPI_API int MPI_Initialized(int *);
_NORETURN IMPORT_MPI_API int MPI_Abort(MPI_Comm, int);

/* MPI-2 communicator naming functions */
IMPORT_MPI_API int MPI_Comm_set_name(MPI_Comm, char *);
IMPORT_MPI_API int MPI_Comm_get_name(MPI_Comm, char *, int *);

#ifdef HAVE_NO_C_CONST
/* Default Solaris compiler does not accept const but does accept prototypes */
#if defined(USE_STDARG) 
IMPORT_MPI_API int MPI_Pcontrol(int, ...);
#else
IMPORT_MPI_API int MPI_Pcontrol(int);
#endif
#else
IMPORT_MPI_API int MPI_Pcontrol(const int, ...);
#endif

IMPORT_MPI_API int MPI_NULL_COPY_FN ( MPI_Comm, int, void *, void *, void *, int * );
IMPORT_MPI_API int MPI_NULL_DELETE_FN ( MPI_Comm, int, void *, void * );
IMPORT_MPI_API int MPI_DUP_FN ( MPI_Comm, int, void *, void *, void *, int * );

/* misc2 (MPI2) */
IMPORT_MPI_API int MPI_Status_f2c( MPI_Fint *, MPI_Status * );
IMPORT_MPI_API int MPI_Status_c2f( MPI_Status *, MPI_Fint * );
IMPORT_MPI_API int MPI_Finalized( int * );
IMPORT_MPI_API int MPI_Type_create_indexed_block(int, int, int *, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_get_envelope(MPI_Datatype, int *, int *, int *, int *); 
IMPORT_MPI_API int MPI_Type_get_contents(MPI_Datatype, int, int, int, int *, MPI_Aint *, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_create_subarray(int, int *, int *, int *, int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Type_create_darray(int, int, int, int *, int *, int *, int *, int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int MPI_Info_create(MPI_Info *);
IMPORT_MPI_API int MPI_Info_set(MPI_Info, char *, char *);
IMPORT_MPI_API int MPI_Info_delete(MPI_Info, char *);
IMPORT_MPI_API int MPI_Info_get(MPI_Info, char *, int, char *, int *);
IMPORT_MPI_API int MPI_Info_get_valuelen(MPI_Info, char *, int *, int *);
IMPORT_MPI_API int MPI_Info_get_nkeys(MPI_Info, int *);
IMPORT_MPI_API int MPI_Info_get_nthkey(MPI_Info, int, char *);
IMPORT_MPI_API int MPI_Info_dup(MPI_Info, MPI_Info *);
IMPORT_MPI_API int MPI_Info_free(MPI_Info *info);

MPI_Fint MPI_Info_c2f(MPI_Info);
MPI_Info MPI_Info_f2c(MPI_Fint);
MPI_Fint MPI_Request_c2f( MPI_Request );

/* external */
IMPORT_MPI_API int MPI_Status_set_cancelled( MPI_Status *, int );
IMPORT_MPI_API int MPI_Status_set_elements( MPI_Status *, MPI_Datatype, int );


/*
 * single sided communication
 */

/* Window calls */

IMPORT_MPI_API int MPI_Alloc_mem (MPI_Aint, MPI_Info, void *);
IMPORT_MPI_API int MPI_Free_mem (void *);

#define MPI_MALLOC_ALIGNMENT 64
/* malloc() / free() wrapper - but not inside the MPI library! */
#if (defined MPI_MALLOC_WRAP) && !(defined MPID_DEVICE_CODE) && !(defined MPIR_LIB_CODE)
void *_mpi_malloc(size_t len);
#define malloc(s) _mpi_malloc(s)
#define free(s) MPI_Free_mem(s)
/* default address alignment for wrapper functions */
#endif

IMPORT_MPI_API int MPI_Win_create (void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
IMPORT_MPI_API int MPI_Win_free (MPI_Win *);
IMPORT_MPI_API int MPI_Win_get_attr (MPI_Win, int, void *, int *);
IMPORT_MPI_API int MPI_Win_get_group (MPI_Win, MPI_Group *);

/* Communication Calls */
IMPORT_MPI_API int MPI_Put (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
IMPORT_MPI_API int MPI_Get (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
IMPORT_MPI_API int MPI_Accumulate (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win);

/* Synchronization Calls */
IMPORT_MPI_API int MPI_Win_fence (int, MPI_Win);
IMPORT_MPI_API int MPI_Win_start (MPI_Group, int, MPI_Win);
IMPORT_MPI_API int MPI_Win_complete (MPI_Win);
IMPORT_MPI_API int MPI_Win_post (MPI_Group, int, MPI_Win);
IMPORT_MPI_API int MPI_Win_wait (MPI_Win);
IMPORT_MPI_API int MPI_Win_test (MPI_Win, int *);
IMPORT_MPI_API int MPI_Win_lock (int, int, int, MPI_Win);
IMPORT_MPI_API int MPI_Win_unlock (int, MPI_Win);

/* Error handlers */
IMPORT_MPI_API int MPI_Win_create_errhandler (	MPI_Win_errhandler_fn *, 
						MPI_Errhandler *);
IMPORT_MPI_API int MPI_Win_set_errhandler (MPI_Win , MPI_Errhandler);
IMPORT_MPI_API int MPI_Win_get_errhandler (MPI_Win , MPI_Errhandler *);




#endif /* MPICH_SUPPRESS_PROTOTYPES */



/* Here are the bindings of the profiling routines */
#if !defined(MPI_BUILD_PROFILING)
IMPORT_MPI_API int PMPI_Send(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Recv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int PMPI_Get_count(MPI_Status *, MPI_Datatype, int *);
IMPORT_MPI_API int PMPI_Bsend(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Ssend(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Rsend(void*, int, MPI_Datatype, int, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Buffer_attach( void* buffer, int);
IMPORT_MPI_API int PMPI_Buffer_detach( void* buffer, int*);
IMPORT_MPI_API int PMPI_Isend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Ibsend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Issend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Irsend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Irecv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Wait(MPI_Request *, MPI_Status *);
IMPORT_MPI_API int PMPI_Test(MPI_Request *, int *flag, MPI_Status *);
IMPORT_MPI_API int PMPI_Request_free(MPI_Request *);
IMPORT_MPI_API int PMPI_Waitany(int, MPI_Request *, int *, MPI_Status *);
IMPORT_MPI_API int PMPI_Testany(int, MPI_Request *, int *, int *, MPI_Status *);
IMPORT_MPI_API int PMPI_Waitall(int, MPI_Request *, MPI_Status *);
IMPORT_MPI_API int PMPI_Testall(int, MPI_Request *, int *flag, MPI_Status *);
IMPORT_MPI_API int PMPI_Waitsome(int, MPI_Request *, int *, int *, MPI_Status *);
IMPORT_MPI_API int PMPI_Testsome(int, MPI_Request *, int *, int *, MPI_Status *);
IMPORT_MPI_API int PMPI_Iprobe(int, int, MPI_Comm, int *flag, MPI_Status *);
IMPORT_MPI_API int PMPI_Probe(int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int PMPI_Cancel(MPI_Request *);
IMPORT_MPI_API int PMPI_Test_cancelled(MPI_Status *, int *flag);
IMPORT_MPI_API int PMPI_Send_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Bsend_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Ssend_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Rsend_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Recv_init(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
IMPORT_MPI_API int PMPI_Start(MPI_Request *);
IMPORT_MPI_API int PMPI_Startall(int, MPI_Request *);
IMPORT_MPI_API int PMPI_Sendrecv(void *, int, MPI_Datatype, int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int PMPI_Sendrecv_replace(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *);
IMPORT_MPI_API int PMPI_Type_contiguous(int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_vector(int, int, int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_hvector(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_indexed(int, int *, int *, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_hindexed(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_struct(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Address(void*, MPI_Aint *);
IMPORT_MPI_API int PMPI_Type_extent(MPI_Datatype, MPI_Aint *);
/* See the 1.1 version of the Standard; I think that the standard is in 
   error; however, it is the standard */
/* IMPORT_MPI_API int PMPI_Type_size(MPI_Datatype, MPI_Aint *); */
IMPORT_MPI_API int PMPI_Type_size(MPI_Datatype, int *);
IMPORT_MPI_API int PMPI_Type_count(MPI_Datatype, int *);
IMPORT_MPI_API int PMPI_Type_lb(MPI_Datatype, MPI_Aint*);
IMPORT_MPI_API int PMPI_Type_ub(MPI_Datatype, MPI_Aint*);
IMPORT_MPI_API int PMPI_Type_commit(MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_free(MPI_Datatype *);
IMPORT_MPI_API int PMPI_Get_elements(MPI_Status *, MPI_Datatype, int *);
IMPORT_MPI_API int PMPI_Pack(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm);
IMPORT_MPI_API int PMPI_Unpack(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int PMPI_Pack_size(int, MPI_Datatype, MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Barrier(MPI_Comm );
IMPORT_MPI_API int PMPI_Bcast(void* buffer, int, MPI_Datatype, int, MPI_Comm );
IMPORT_MPI_API int PMPI_Gather(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm); 
IMPORT_MPI_API int PMPI_Gatherv(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm); 
IMPORT_MPI_API int PMPI_Scatter(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Scatterv(void* , int *, int *displs, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Allgather(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int PMPI_Allgatherv(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int PMPI_Alltoall(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int PMPI_Alltoallv(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
IMPORT_MPI_API int PMPI_Reduce(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
IMPORT_MPI_API int PMPI_Op_create(MPI_User_function *, int, MPI_Op *);
IMPORT_MPI_API int PMPI_Op_free( MPI_Op *);
IMPORT_MPI_API int PMPI_Allreduce(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
IMPORT_MPI_API int PMPI_Reduce_scatter(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm);
IMPORT_MPI_API int PMPI_Scan(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm );
IMPORT_MPI_API int PMPI_Group_size(MPI_Group group, int *);
IMPORT_MPI_API int PMPI_Group_rank(MPI_Group group, int *);
IMPORT_MPI_API int PMPI_Group_translate_ranks (MPI_Group, int, int *, MPI_Group, int *);
IMPORT_MPI_API int PMPI_Group_compare(MPI_Group, MPI_Group, int *);
IMPORT_MPI_API int PMPI_Comm_group(MPI_Comm, MPI_Group *);
IMPORT_MPI_API int PMPI_Group_union(MPI_Group, MPI_Group, MPI_Group *);
IMPORT_MPI_API int PMPI_Group_intersection(MPI_Group, MPI_Group, MPI_Group *);
IMPORT_MPI_API int PMPI_Group_difference(MPI_Group, MPI_Group, MPI_Group *);
IMPORT_MPI_API int PMPI_Group_incl(MPI_Group group, int, int *, MPI_Group *);
IMPORT_MPI_API int PMPI_Group_excl(MPI_Group group, int, int *, MPI_Group *);
IMPORT_MPI_API int PMPI_Group_range_incl(MPI_Group group, int, int [][3], MPI_Group *);
IMPORT_MPI_API int PMPI_Group_range_excl(MPI_Group group, int, int [][3], MPI_Group *);
IMPORT_MPI_API int PMPI_Group_free(MPI_Group *);
IMPORT_MPI_API int PMPI_Comm_size(MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Comm_rank(MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Comm_compare(MPI_Comm, MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Comm_dup(MPI_Comm, MPI_Comm *);
IMPORT_MPI_API int PMPI_Comm_create(MPI_Comm, MPI_Group, MPI_Comm *);
IMPORT_MPI_API int PMPI_Comm_split(MPI_Comm, int, int, MPI_Comm *);
IMPORT_MPI_API int PMPI_Comm_free(MPI_Comm *);
IMPORT_MPI_API int PMPI_Comm_test_inter(MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Comm_remote_size(MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Comm_remote_group(MPI_Comm, MPI_Group *);
IMPORT_MPI_API int PMPI_Intercomm_create(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm *);
IMPORT_MPI_API int PMPI_Intercomm_merge(MPI_Comm, int, MPI_Comm *);
IMPORT_MPI_API int PMPI_Keyval_create(MPI_Copy_function *, MPI_Delete_function *, int *, void*);
IMPORT_MPI_API int PMPI_Keyval_free(int *);
IMPORT_MPI_API int PMPI_Attr_put(MPI_Comm, int, void*);
IMPORT_MPI_API int PMPI_Attr_get(MPI_Comm, int, void *, int *);
IMPORT_MPI_API int PMPI_Attr_delete(MPI_Comm, int);
IMPORT_MPI_API int PMPI_Topo_test(MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Cart_create(MPI_Comm, int, int *, int *, int, MPI_Comm *);
IMPORT_MPI_API int PMPI_Dims_create(int, int, int *);
IMPORT_MPI_API int PMPI_Graph_create(MPI_Comm, int, int *, int *, int, MPI_Comm *);
IMPORT_MPI_API int PMPI_Graphdims_get(MPI_Comm, int *, int *);
IMPORT_MPI_API int PMPI_Graph_get(MPI_Comm, int, int, int *, int *);
IMPORT_MPI_API int PMPI_Cartdim_get(MPI_Comm, int *);
IMPORT_MPI_API int PMPI_Cart_get(MPI_Comm, int, int *, int *, int *);
IMPORT_MPI_API int PMPI_Cart_rank(MPI_Comm, int *, int *);
IMPORT_MPI_API int PMPI_Cart_coords(MPI_Comm, int, int, int *);
IMPORT_MPI_API int PMPI_Graph_neighbors_count(MPI_Comm, int, int *);
IMPORT_MPI_API int PMPI_Graph_neighbors(MPI_Comm, int, int, int *);
IMPORT_MPI_API int PMPI_Cart_shift(MPI_Comm, int, int, int *, int *);
IMPORT_MPI_API int PMPI_Cart_sub(MPI_Comm, int *, MPI_Comm *);
IMPORT_MPI_API int PMPI_Cart_map(MPI_Comm, int, int *, int *, int *);
IMPORT_MPI_API int PMPI_Graph_map(MPI_Comm, int, int *, int *, int *);
IMPORT_MPI_API int PMPI_Get_processor_name(char *, int *);
IMPORT_MPI_API int PMPI_Get_version(int *, int *);
IMPORT_MPI_API int PMPI_Errhandler_create(MPI_Handler_function *, MPI_Errhandler *);
IMPORT_MPI_API int PMPI_Errhandler_set(MPI_Comm, MPI_Errhandler);
IMPORT_MPI_API int PMPI_Errhandler_get(MPI_Comm, MPI_Errhandler *);
IMPORT_MPI_API int PMPI_Errhandler_free(MPI_Errhandler *);
IMPORT_MPI_API int PMPI_Error_string(int, char *, int *);
IMPORT_MPI_API int PMPI_Error_class(int, int *);

IMPORT_MPI_API int PMPI_Type_get_envelope(MPI_Datatype, int *, int *, int *, int *); 
IMPORT_MPI_API int PMPI_Type_get_contents(MPI_Datatype, int, int, int, int *, MPI_Aint *, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_create_subarray(int, int *, int *, int *, int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Type_create_darray(int, int, int, int *, int *, int *, int *, int, MPI_Datatype, MPI_Datatype *);
IMPORT_MPI_API int PMPI_Info_create(MPI_Info *);
IMPORT_MPI_API int PMPI_Info_set(MPI_Info, char *, char *);
IMPORT_MPI_API int PMPI_Info_delete(MPI_Info, char *);
IMPORT_MPI_API int PMPI_Info_get(MPI_Info, char *, int, char *, int *);
IMPORT_MPI_API int PMPI_Info_get_valuelen(MPI_Info, char *, int *, int *);
IMPORT_MPI_API int PMPI_Info_get_nkeys(MPI_Info, int *);
IMPORT_MPI_API int PMPI_Info_get_nthkey(MPI_Info, int, char *);
IMPORT_MPI_API int PMPI_Info_dup(MPI_Info, MPI_Info *);
IMPORT_MPI_API int PMPI_Info_free(MPI_Info *);

MPI_Fint PMPI_Info_c2f(MPI_Info);
MPI_Info PMPI_Info_f2c(MPI_Fint);

/* Wtime done above */
IMPORT_MPI_API int PMPI_Init(int *, char ***);
IMPORT_MPI_API int PMPI_Init_thread( int *, char ***, int, int * );
IMPORT_MPI_API int PMPI_Finalize(void);
IMPORT_MPI_API int PMPI_Initialized(int *);
IMPORT_MPI_API int PMPI_Abort(MPI_Comm, int);
/* MPI-2 communicator naming functions */
/* IMPORT_MPI_API int PMPI_Comm_set_name(MPI_Comm, char *); */
/* IMPORT_MPI_API int PMPI_Comm_get_name(MPI_Comm, char **); */
#ifdef HAVE_NO_C_CONST
/* Default Solaris compiler does not accept const but does accept prototypes */
#if defined(USE_STDARG) 
IMPORT_MPI_API int PMPI_Pcontrol(int, ...);
#else
IMPORT_MPI_API int PMPI_Pcontrol(int);
#endif
#else
IMPORT_MPI_API int PMPI_Pcontrol(const int, ...);
#endif

/* external */
IMPORT_MPI_API int PMPI_Status_set_cancelled( MPI_Status *, int );
IMPORT_MPI_API int PMPI_Status_set_elements( MPI_Status *, MPI_Datatype, int );

IMPORT_MPI_API int PMPI_Alloc_mem (MPI_Aint, MPI_Info, void *);
IMPORT_MPI_API int PMPI_Free_mem (void *);

/* Window management */
IMPORT_MPI_API int PMPI_Win_create (void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
IMPORT_MPI_API int PMPI_Win_free (MPI_Win *);
IMPORT_MPI_API int PMPI_Win_get_attr (MPI_Win, int, void *, int *);
IMPORT_MPI_API int PMPI_Win_get_group (MPI_Win, MPI_Group *);

/* Communication Calls */

IMPORT_MPI_API int PMPI_Put (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
IMPORT_MPI_API int PMPI_Get (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
IMPORT_MPI_API int PMPI_Accumulate (void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win);

/* Synchronization Calls */

IMPORT_MPI_API int PMPI_Win_fence (int, MPI_Win);
IMPORT_MPI_API int PMPI_Win_start (MPI_Group, int, MPI_Win);
IMPORT_MPI_API int PMPI_Win_complete (MPI_Win);
IMPORT_MPI_API int PMPI_Win_post (MPI_Group, int, MPI_Win);
IMPORT_MPI_API int PMPI_Win_wait (MPI_Win);
IMPORT_MPI_API int PMPI_Win_test (MPI_Win, int *);
IMPORT_MPI_API int PMPI_Win_lock (int, int, int, MPI_Win);
IMPORT_MPI_API int PMPI_Win_unlock (int, MPI_Win);

/* Error handlers */

IMPORT_MPI_API int PMPI_Win_create_errhandler (	MPI_Win_errhandler_fn *, 
						MPI_Errhandler *);
IMPORT_MPI_API int PMPI_Win_set_errhandler (MPI_Win , MPI_Errhandler);
IMPORT_MPI_API int PMPI_Win_get_errhandler (MPI_Win , MPI_Errhandler *);

#endif  /* MPI_BUILD_PROFILING */
/* End of MPI bindings */

#if defined(__cplusplus)
}
/* Add the C++ bindings */
/* 
   If MPICH_SKIP_MPICXX is defined, the mpi++.h file will *not* be included.
   This is necessary, for example, when building the C++ interfaces.  It
   can also be used when you want to use a C++ compiler to compile C code,
   and do not want to load the C++ bindings
 */
#if defined(HAVE_MPI_CPP) && !defined(MPICH_SKIP_MPICXX)
#include "mpi++.h"
#endif 
#endif

#endif

