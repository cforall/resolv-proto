# 1 "raii/dtor-early-exit.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdbool.h" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdbool.h"

# 1 "/usr/lib/gcc/x86_64-linux-gnu/6/include/stdbool.h" 1 3 4
# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdbool.h" 2

# 1 "<command-line>" 2
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "raii/dtor-early-exit.c"
# 16 "raii/dtor-early-exit.c"
# 1 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
       

# 1 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
       

# 1 "/home/a3moss/bin/cfa/include/cfa/iterator.hfa" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/iterator.hfa"
       


trait iterator( otype iterator_type, otype elt_type ) {


 iterator_type ++?( iterator_type & );
 iterator_type --?( iterator_type & );


 int ?==?( iterator_type, iterator_type );
 int ?!=?( iterator_type, iterator_type );


 elt_type & *?( iterator_type );
};

trait iterator_for( otype iterator_type, otype collection_type, otype elt_type | iterator( iterator_type, elt_type ) ) {

 iterator_type begin( collection_type );
 iterator_type end( collection_type );
};

forall( otype iterator_type, otype elt_type | iterator( iterator_type, elt_type ) )
void for_each( iterator_type begin, iterator_type end, void (* func)( elt_type ) );

forall( otype iterator_type, otype elt_type | iterator( iterator_type, elt_type ) )
void for_each_reverse( iterator_type begin, iterator_type end, void (* func)( elt_type ) );
# 19 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 2

trait ostream( dtype ostype ) {

 
# 22 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
_Bool 
# 22 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
     sepPrt( ostype & );
 void sepReset( ostype & );
 void sepReset( ostype &, 
# 24 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
                         _Bool 
# 24 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
                              );
 const char * sepGetCur( ostype & );
 void sepSetCur( ostype &, const char * );
 
# 27 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
_Bool 
# 27 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
     getNL( ostype & );
 void setNL( ostype &, 
# 28 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
                      _Bool 
# 28 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
                           );

 void sepOn( ostype & );
 void sepOff( ostype & );
 
# 32 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
_Bool 
# 32 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
     sepDisable( ostype & );
 
# 33 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
_Bool 
# 33 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
     sepEnable( ostype & );

 const char * sepGet( ostype & );
 void sepSet( ostype &, const char * );
 const char * sepGetTuple( ostype & );
 void sepSetTuple( ostype &, const char * );

 int fail( ostype & );
 int flush( ostype & );
 void open( ostype & os, const char * name, const char * mode );
 void close( ostype & os );
 ostype & write( ostype &, const char *, size_t );
 int fmt( ostype &, const char fmt[], ... );
};





trait writeable( otype T, dtype ostype | ostream( ostype ) ) {
 ostype & ?|?( ostype &, T );
};



forall( dtype ostype | ostream( ostype ) ) {
 ostype & ?|?( ostype &, 
# 59 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
                        _Bool 
# 59 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
                             );

 ostype & ?|?( ostype &, char );
 ostype & ?|?( ostype &, signed char );
 ostype & ?|?( ostype &, unsigned char );

 ostype & ?|?( ostype &, short int );
 ostype & ?|?( ostype &, unsigned short int );
 ostype & ?|?( ostype &, int );
 ostype & ?|?( ostype &, unsigned int );
 ostype & ?|?( ostype &, long int );
 ostype & ?|?( ostype &, long long int );
 ostype & ?|?( ostype &, unsigned long int );
 ostype & ?|?( ostype &, unsigned long long int );

 ostype & ?|?( ostype &, float );
 ostype & ?|?( ostype &, double );
 ostype & ?|?( ostype &, long double );

 ostype & ?|?( ostype &, float _Complex );
 ostype & ?|?( ostype &, double _Complex );
 ostype & ?|?( ostype &, long double _Complex );

 ostype & ?|?( ostype &, const char * );





 ostype & ?|?( ostype &, const void * );


 ostype & ?|?( ostype &, ostype & (*)( ostype & ) );
 ostype & endl( ostype & );
 ostype & sep( ostype & );
 ostype & sepTuple( ostype & );
 ostype & sepOn( ostype & );
 ostype & sepOff( ostype & );
 ostype & sepDisable( ostype & );
 ostype & sepEnable( ostype & );
}


forall( dtype ostype, otype T, ttype Params | writeable( T, ostype ) | { ostype & ?|?( ostype &, Params ); } )
ostype & ?|?( ostype & os, T arg, Params rest );


forall( dtype ostype, otype elt_type | writeable( elt_type, ostype ), otype iterator_type | iterator( iterator_type, elt_type ) )
void write( iterator_type begin, iterator_type end, ostype & os );

forall( dtype ostype, otype elt_type | writeable( elt_type, ostype ), otype iterator_type | iterator( iterator_type, elt_type ) )
void write_reverse( iterator_type begin, iterator_type end, ostype & os );



trait istream( dtype istype ) {
 int fail( istype & );
 int eof( istype & );
 void open( istype & is, const char * name );
 void close( istype & is );
 istype & read( istype &, char *, size_t );
 istype & ungetc( istype &, char );
 int fmt( istype &, const char fmt[], ... );
};

trait readable( otype T ) {
 forall( dtype istype | istream( istype ) ) istype & ?|?( istype &, T );
};

forall( dtype istype | istream( istype ) ) {
 istype & ?|?( istype &, 
# 129 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 3 4
                        _Bool 
# 129 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa"
                             & );

 istype & ?|?( istype &, char & );
 istype & ?|?( istype &, signed char & );
 istype & ?|?( istype &, unsigned char & );

 istype & ?|?( istype &, short int & );
 istype & ?|?( istype &, unsigned short int & );
 istype & ?|?( istype &, int & );
 istype & ?|?( istype &, unsigned int & );
 istype & ?|?( istype &, long int & );
 istype & ?|?( istype &, long long int & );
 istype & ?|?( istype &, unsigned long int & );
 istype & ?|?( istype &, unsigned long long int & );

 istype & ?|?( istype &, float & );
 istype & ?|?( istype &, double & );
 istype & ?|?( istype &, long double & );

 istype & ?|?( istype &, float _Complex & );
 istype & ?|?( istype &, double _Complex & );
 istype & ?|?( istype &, long double _Complex & );


 istype & ?|?( istype &, istype & (*)( istype & ) );
 istype & endl( istype & is );
}

struct _Istream_cstrUC { char * s; };
_Istream_cstrUC cstr( char * );
forall( dtype istype | istream( istype ) ) istype & ?|?( istype &, _Istream_cstrUC );

struct _Istream_cstrC { char * s; int size; };
_Istream_cstrC cstr( char *, int size );
forall( dtype istype | istream( istype ) ) istype & ?|?( istype &, _Istream_cstrC );


# 1 "/home/a3moss/bin/cfa/include/cfa/time_t.hfa" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/time_t.hfa"
       




struct Duration {
 int64_t tv;
};

static inline void ?{}( Duration & dur ) with( dur ) { tv = 0; }
static inline void ?{}( Duration & dur, zero_t ) with( dur ) { tv = 0; }




struct Time {
 uint64_t tv;
};

static inline void ?{}( Time & time ) with( time ) { tv = 0; }
static inline void ?{}( Time & time, zero_t ) with( time ) { tv = 0; }
# 167 "/home/a3moss/bin/cfa/include/cfa/iostream.hfa" 2

forall( dtype ostype | ostream( ostype ) ) ostype & ?|?( ostype & os, Duration dur );
forall( dtype ostype | ostream( ostype ) ) ostype & ?|?( ostype & os, Time time );
# 19 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 2

enum { sepSize = 16 };
struct ofstream {
 void * file;
 
# 23 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 23 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
     sepDefault;
 
# 24 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 24 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
     sepOnOff;
 
# 25 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 25 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
     sawNL;
 const char * sepCur;
 char separator[sepSize];
 char tupleSeparator[sepSize];
};



# 32 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 32 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
    sepPrt( ofstream & );
void sepReset( ofstream & );
void sepReset( ofstream &, 
# 34 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
                          _Bool 
# 34 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
                               );
const char * sepGetCur( ofstream & );
void sepSetCur( ofstream &, const char * );

# 37 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 37 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
    getNL( ofstream & );
void setNL( ofstream &, 
# 38 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
                       _Bool 
# 38 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
                            );


void sepOn( ofstream & );
void sepOff( ofstream & );

# 43 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 43 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
    sepDisable( ofstream & );

# 44 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa" 3 4
_Bool 
# 44 "/home/a3moss/bin/cfa/include/cfa/fstream.hfa"
    sepEnable( ofstream & );

const char * sepGet( ofstream & );
void sepSet( ofstream &, const char * );
const char * sepGetTuple( ofstream & );
void sepSetTuple( ofstream &, const char * );

int fail( ofstream & );
int flush( ofstream & );
void open( ofstream &, const char * name, const char * mode );
void open( ofstream &, const char * name );
void close( ofstream & );
ofstream & write( ofstream &, const char * data, size_t size );
int fmt( ofstream &, const char fmt[], ... );

void ?{}( ofstream & os );
void ?{}( ofstream & os, const char * name, const char * mode );
void ?{}( ofstream & os, const char * name );

extern ofstream & sout, & serr;


struct ifstream {
 void * file;
};


int fail( ifstream & is );
int eof( ifstream & is );
void open( ifstream & is, const char * name, const char * mode );
void open( ifstream & is, const char * name );
void close( ifstream & is );
ifstream & read( ifstream & is, char * data, size_t size );
ifstream & ungetc( ifstream & is, char c );
int fmt( ifstream &, const char fmt[], ... );

void ?{}( ifstream & is );
void ?{}( ifstream & is, const char * name, const char * mode );
void ?{}( ifstream & is, const char * name );

extern ifstream & sin;
# 17 "raii/dtor-early-exit.c" 2
# 1 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa"
       

# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdlib.h" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdlib.h"
extern "C" {
# 1 "/usr/include/stdlib.h" 1 3 4
# 24 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 367 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 1 3 4
# 410 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 411 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 368 "/usr/include/features.h" 2 3 4
# 391 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4
# 392 "/usr/include/features.h" 2 3 4
# 25 "/usr/include/stdlib.h" 2 3 4







# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 1 3 4
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 3 4

# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 3 4
extern "C" {
# 1 "/usr/lib/gcc/x86_64-linux-gnu/6/include/stddef.h" 1 3 4
# 216 "/usr/lib/gcc/x86_64-linux-gnu/6/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 328 "/usr/lib/gcc/x86_64-linux-gnu/6/include/stddef.h" 3 4
typedef int wchar_t;
# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 2 3 4


}
# 33 "/usr/include/stdlib.h" 2 3 4








# 1 "/usr/include/x86_64-linux-gnu/bits/waitflags.h" 1 3 4
# 50 "/usr/include/x86_64-linux-gnu/bits/waitflags.h" 3 4
typedef enum
{
  P_ALL,
  P_PID,
  P_PGID
} idtype_t;
# 42 "/usr/include/stdlib.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 1 3 4
# 64 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 36 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endian.h" 1 3 4
# 37 "/usr/include/endian.h" 2 3 4
# 60 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;







typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
# 121 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/typesizes.h" 1 3 4
# 122 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;
# 28 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 2 3 4






# 1 "/usr/include/x86_64-linux-gnu/bits/byteswap-16.h" 1 3 4
# 36 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 2 3 4
# 44 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 3 4
static __inline unsigned int
__bswap_32 (unsigned int __bsx)
{
  return __builtin_bswap32 (__bsx);
}
# 108 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 3 4
static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{
  return __builtin_bswap64 (__bsx);
}
# 61 "/usr/include/endian.h" 2 3 4
# 65 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 2 3 4

union wait
  {
    int w_status;
    struct
      {

 unsigned int __w_termsig:7;
 unsigned int __w_coredump:1;
 unsigned int __w_retcode:8;
 unsigned int:16;







      } __wait_terminated;
    struct
      {

 unsigned int __w_stopval:8;
 unsigned int __w_stopsig:8;
 unsigned int:16;






      } __wait_stopped;
  };
# 43 "/usr/include/stdlib.h" 2 3 4
# 67 "/usr/include/stdlib.h" 3 4
typedef union
  {
    union wait *__uptr;
    int *__iptr;
  } __WAIT_STATUS __attribute__ ((__transparent_union__));
# 95 "/usr/include/stdlib.h" 3 4


typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;







__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;


# 139 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) ;




extern double atof (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





__extension__ extern long long int atoll (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





extern double strtod (const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern float strtof (const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern long double strtold (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern long int strtol (const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern unsigned long int strtoul (const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




__extension__
extern long long int strtoq (const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





__extension__
extern long long int strtoll (const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

# 305 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) __attribute__ ((__nothrow__ , __leaf__)) ;


extern long int a64l (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;




# 1 "/usr/include/x86_64-linux-gnu/sys/types.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4






typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;




typedef __loff_t loff_t;



typedef __ino_t ino_t;
# 60 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;





typedef __off_t off_t;
# 98 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __pid_t pid_t;





typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;
# 132 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/time.h" 1 3 4
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/time.h" 3 4
extern "C" {
# 1 "/usr/include/time.h" 1 3 4
# 57 "/usr/include/time.h" 3 4


typedef __clock_t clock_t;



# 73 "/usr/include/time.h" 3 4


typedef __time_t time_t;



# 91 "/usr/include/time.h" 3 4
typedef __clockid_t clockid_t;
# 103 "/usr/include/time.h" 3 4
typedef __timer_t timer_t;
# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/time.h" 2 3 4
}
# 133 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4
# 146 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 1 3 4
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 3 4
extern "C" {
# 1 "/usr/lib/gcc/x86_64-linux-gnu/6/include/stddef.h" 1 3 4
# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 2 3 4


}
# 147 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
# 200 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));

typedef int register_t __attribute__ ((__mode__ (__word__)));
# 219 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/select.h" 1 3 4
# 30 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/select.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/select.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/select.h" 2 3 4
# 31 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/sigset.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/sigset.h" 3 4
typedef int __sig_atomic_t;




typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
# 34 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4



typedef __sigset_t sigset_t;





# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/time.h" 1 3 4
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/time.h" 3 4
extern "C" {
# 1 "/usr/include/time.h" 1 3 4
# 120 "/usr/include/time.h" 3 4
struct timespec
  {
    __time_t tv_sec;
    __syscall_slong_t tv_nsec;
  };
# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/time.h" 2 3 4
}
# 44 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/time.h" 1 3 4
# 30 "/usr/include/x86_64-linux-gnu/bits/time.h" 3 4
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 46 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4


typedef __suseconds_t suseconds_t;





typedef long int __fd_mask;
# 64 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
typedef struct
  {






    __fd_mask __fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];


  } fd_set;






typedef __fd_mask fd_mask;
# 96 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4

# 106 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 118 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 131 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4

# 220 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/sys/sysmacros.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/sys/sysmacros.h" 3 4


__extension__
extern unsigned int gnu_dev_major (unsigned long long int __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__
extern unsigned int gnu_dev_minor (unsigned long long int __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__
extern unsigned long long int gnu_dev_makedev (unsigned int __major,
            unsigned int __minor)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 58 "/usr/include/x86_64-linux-gnu/sys/sysmacros.h" 3 4

# 223 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;






typedef __blkcnt_t blkcnt_t;



typedef __fsblkcnt_t fsblkcnt_t;



typedef __fsfilcnt_t fsfilcnt_t;
# 270 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 1 3 4
# 21 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 2 3 4
# 60 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
typedef unsigned long int pthread_t;


union pthread_attr_t
{
  char __size[56];
  long int __align;
};

typedef union pthread_attr_t pthread_attr_t;





typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;
# 90 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
typedef union
{
  struct __pthread_mutex_s
  {
    int __lock;
    unsigned int __count;
    int __owner;

    unsigned int __nusers;



    int __kind;

    short __spins;
    short __elision;
    __pthread_list_t __list;
# 125 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
  } __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  struct
  {
    int __lock;
    unsigned int __futex;
    __extension__ unsigned long long int __total_seq;
    __extension__ unsigned long long int __wakeup_seq;
    __extension__ unsigned long long int __woken_seq;
    void *__mutex;
    unsigned int __nwaiters;
    unsigned int __broadcast_seq;
  } __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;





typedef union
{

  struct
  {
    int __lock;
    unsigned int __nr_readers;
    unsigned int __readers_wakeup;
    unsigned int __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
    int __writer;
    int __shared;
    signed char __rwelision;




    unsigned char __pad1[7];


    unsigned long int __pad2;


    unsigned int __flags;

  } __data;
# 220 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 271 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4



# 315 "/usr/include/stdlib.h" 2 3 4






extern long int random (void) __attribute__ ((__nothrow__ , __leaf__));


extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));






extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));




extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__ , __leaf__));







extern double drand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;

  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));









extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) ;










extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__));

extern void free (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));




extern void cfree (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/alloca.h" 1 3 4
# 24 "/usr/include/alloca.h" 3 4
# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 1 3 4
# 16 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 3 4
extern "C" {
# 1 "/usr/lib/gcc/x86_64-linux-gnu/6/include/stddef.h" 1 3 4
# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stddef.h" 2 3 4


}
# 25 "/usr/include/alloca.h" 2 3 4







extern void *alloca (size_t __size) __attribute__ ((__nothrow__ , __leaf__));






# 493 "/usr/include/stdlib.h" 2 3 4





extern void *valloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern void *aligned_alloc (size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_size__ (2))) ;




extern void abort (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int at_quick_exit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void quick_exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));







extern void _Exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));






extern char *getenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;

# 578 "/usr/include/stdlib.h" 3 4
extern int putenv (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int setenv (const char *__name, const char *__value, int __replace)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));


extern int unsetenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int clearenv (void) __attribute__ ((__nothrow__ , __leaf__));
# 606 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 619 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 641 "/usr/include/stdlib.h" 3 4
extern int mkstemps (char *__template, int __suffixlen) __attribute__ ((__nonnull__ (1))) ;
# 662 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 711 "/usr/include/stdlib.h" 3 4





extern int system (const char *__command) ;

# 733 "/usr/include/stdlib.h" 3 4
extern char *realpath (const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__ , __leaf__)) ;






typedef int (*__compar_fn_t) (const void *, const void *);
# 751 "/usr/include/stdlib.h" 3 4



extern void *bsearch (const void *__key, const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;







extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));
# 774 "/usr/include/stdlib.h" 3 4
extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;



__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;







extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;




__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;

# 811 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));






extern int mblen (const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int mbtowc (wchar_t *__restrict __pwc,
     const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__));



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));

extern size_t wcstombs (char *__restrict __s,
   const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__));








extern int rpmatch (const char *__response) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 898 "/usr/include/stdlib.h" 3 4
extern int getsubopt (char **__restrict __optionp,
        char *const *__restrict __tokens,
        char **__restrict __valuep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2, 3))) ;
# 950 "/usr/include/stdlib.h" 3 4
extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


# 1 "/usr/include/x86_64-linux-gnu/bits/stdlib-float.h" 1 3 4
# 955 "/usr/include/stdlib.h" 2 3 4
# 967 "/usr/include/stdlib.h" 3 4

# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdlib.h" 2

# 18 "/home/a3moss/bin/cfa/include/cfa/stdhdr/stdlib.h"
}
# 19 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa" 2
extern "C" {
 void * memalign( size_t align, size_t size );
 void * memset( void * dest, int fill, size_t size );
 void * memcpy( void * dest, const void * src, size_t size );
    void * cmemalign( size_t alignment, size_t noOfElems, size_t elemSize );
}
# 35 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa"
static inline forall( dtype T | sized(T) ) {


 T * malloc( void ) {
  return (T *)(void *)malloc( (size_t)sizeof(T) );
 }
# 49 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa"
 T * calloc( size_t dim ) {
  return (T *)(void *)calloc( dim, sizeof(T) );
 }

 T * realloc( T * ptr, size_t size ) {
  return (T *)(void *)realloc( (void *)ptr, size );
 }

 T * memalign( size_t align ) {
  return (T *)memalign( align, sizeof(T) );
 }

 T * aligned_alloc( size_t align ) {
  return (T *)aligned_alloc( align, sizeof(T) );
 }

 int posix_memalign( T ** ptr, size_t align ) {
  return posix_memalign( (void **)ptr, align, sizeof(T) );
 }




 T * alloc( void ) {
  return (T *)(void *)malloc( (size_t)sizeof(T) );
 }

 T * alloc( char fill ) {
  T * ptr = (T *)(void *)malloc( (size_t)sizeof(T) );
  return (T *)memset( ptr, (int)fill, sizeof(T) );
 }

 T * alloc( size_t dim ) {
  return (T *)(void *)malloc( dim * (size_t)sizeof(T) );
 }

 T * alloc( size_t dim, char fill ) {
  T * ptr = (T *)(void *)malloc( dim * (size_t)sizeof(T) );
  return (T *)memset( ptr, (int)fill, dim * sizeof(T) );
 }

 T * alloc( T ptr[], size_t dim ) {
  return (T *)(void *)realloc( (void *)ptr, dim * (size_t)sizeof(T) );
 }
}


forall( dtype T | sized(T) ) T * alloc( T ptr[], size_t dim, char fill );


static inline forall( dtype T | sized(T) ) {
 T * align_alloc( size_t align ) {
  return (T *)memalign( align, sizeof(T) );
 }

 T * align_alloc( size_t align, char fill ) {
  T * ptr = (T *)memalign( align, sizeof(T) );
  return (T *)memset( ptr, (int)fill, sizeof(T) );
 }

 T * align_alloc( size_t align, size_t dim ) {
  return (T *)memalign( align, dim * sizeof(T) );
 }

 T * align_alloc( size_t align, size_t dim, char fill ) {
  T * ptr;
  if ( fill == '\0' ) {
   ptr = (T *)cmemalign( align, dim, sizeof(T) );
  } else {
   ptr = (T *)memalign( align, dim * sizeof(T) );
   return (T *)memset( ptr, (int)fill, dim * sizeof(T) );
  }
  return ptr;
 }
}


static inline forall( dtype T | sized(T) ) {


 T * memset( T * dest, char fill ) {
  return (T *)memset( dest, fill, sizeof(T) );
 }

 T * memcpy( T * dest, const T * src ) {
  return (T *)memcpy( dest, src, sizeof(T) );
 }
}

static inline forall( dtype T | sized(T) ) {


 T * amemset( T dest[], char fill, size_t dim ) {
  return (T *)(void *)memset( dest, fill, dim * sizeof(T) );
 }

 T * amemcpy( T dest[], const T src[], size_t dim ) {
  return (T *)(void *)memcpy( dest, src, dim * sizeof(T) );
 }
}


forall( dtype T | sized(T), ttype Params | { void ?{}( T &, Params ); } ) T * new( Params p );
forall( dtype T | sized(T) | { void ^?{}( T & ); } ) void delete( T * ptr );
forall( dtype T, ttype Params | sized(T) | { void ^?{}( T & ); void delete( Params ); } ) void delete( T * ptr, Params rest );


forall( dtype T | sized(T), ttype Params | { void ?{}( T &, Params ); } ) T * anew( size_t dim, Params p );
forall( dtype T | sized(T) | { void ^?{}( T & ); } ) void adelete( size_t dim, T arr[] );
forall( dtype T | sized(T) | { void ^?{}( T & ); }, ttype Params | { void adelete( Params ); } ) void adelete( size_t dim, T arr[], Params rest );



static inline {
 int strto( const char * sptr, char ** eptr, int base ) { return (int)strtol( sptr, eptr, base ); }
 unsigned int strto( const char * sptr, char ** eptr, int base ) { return (unsigned int)strtoul( sptr, eptr, base ); }
 long int strto( const char * sptr, char ** eptr, int base ) { return strtol( sptr, eptr, base ); }
 unsigned long int strto( const char * sptr, char ** eptr, int base ) { return strtoul( sptr, eptr, base ); }
 long long int strto( const char * sptr, char ** eptr, int base ) { return strtoll( sptr, eptr, base ); }
 unsigned long long int strto( const char * sptr, char ** eptr, int base ) { return strtoull( sptr, eptr, base ); }

 float strto( const char * sptr, char ** eptr ) { return strtof( sptr, eptr ); }
 double strto( const char * sptr, char ** eptr ) { return strtod( sptr, eptr ); }
 long double strto( const char * sptr, char ** eptr ) { return strtold( sptr, eptr ); }
}

float _Complex strto( const char * sptr, char ** eptr );
double _Complex strto( const char * sptr, char ** eptr );
long double _Complex strto( const char * sptr, char ** eptr );

static inline {
 int ato( const char * sptr ) {return (int)strtol( sptr, 0, 10 ); }
 unsigned int ato( const char * sptr ) { return (unsigned int)strtoul( sptr, 0, 10 ); }
 long int ato( const char * sptr ) { return strtol( sptr, 0, 10 ); }
 unsigned long int ato( const char * sptr ) { return strtoul( sptr, 0, 10 ); }
 long long int ato( const char * sptr ) { return strtoll( sptr, 0, 10 ); }
 unsigned long long int ato( const char * sptr ) { return strtoull( sptr, 0, 10 ); }

 float ato( const char * sptr ) { return strtof( sptr, 0 ); }
 double ato( const char * sptr ) { return strtod( sptr, 0 ); }
 long double ato( const char * sptr ) { return strtold( sptr, 0 ); }

 float _Complex ato( const char * sptr ) { return strto( sptr, 
# 191 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa" 3 4
                                                              0 
# 191 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa"
                                                                   ); }
 double _Complex ato( const char * sptr ) { return strto( sptr, 
# 192 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa" 3 4
                                                               0 
# 192 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa"
                                                                    ); }
 long double _Complex ato( const char * sptr ) { return strto( sptr, 
# 193 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa" 3 4
                                                                    0 
# 193 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa"
                                                                         ); }
}



forall( otype E | { int ?<?( E, E ); } ) {
 E * bsearch( E key, const E * vals, size_t dim );
 size_t bsearch( E key, const E * vals, size_t dim );
 E * bsearchl( E key, const E * vals, size_t dim );
 size_t bsearchl( E key, const E * vals, size_t dim );
 E * bsearchu( E key, const E * vals, size_t dim );
 size_t bsearchu( E key, const E * vals, size_t dim );
}

forall( otype K, otype E | { int ?<?( K, K ); K getKey( const E & ); } ) {
 E * bsearch( K key, const E * vals, size_t dim );
 size_t bsearch( K key, const E * vals, size_t dim );
 E * bsearchl( K key, const E * vals, size_t dim );
 size_t bsearchl( K key, const E * vals, size_t dim );
 E * bsearchu( K key, const E * vals, size_t dim );
 size_t bsearchu( K key, const E * vals, size_t dim );
}

forall( otype E | { int ?<?( E, E ); } ) {
 void qsort( E * vals, size_t dim );
}



extern "C" {
 void srandom( unsigned int seed );
 long int random( void );
}

static inline {
 long int random( long int l, long int u ) { if ( u < l ) [u, l] = [l, u]; return lrand48() % (u - l) + l; }
 long int random( long int u ) { if ( u < 0 ) return random( u, 0 ); else return random( 0, u ); }
 unsigned long int random( void ) { return lrand48(); }
 unsigned long int random( unsigned long int l, unsigned long int u ) { if ( u < l ) [u, l] = [l, u]; return lrand48() % (u - l) + l; }
 unsigned long int random( unsigned long int u ) { return lrand48() % u; }

 char random( void ) { return (unsigned long int)random(); }
 char random( char u ) { return random( (unsigned long int)u ); }
 char random( char l, char u ) { return random( (unsigned long int)l, (unsigned long int)u ); }
 int random( void ) { return (long int)random(); }
 int random( int u ) { return random( (long int)u ); }
 int random( int l, int u ) { return random( (long int)l, (long int)u ); }
 unsigned int random( void ) { return (unsigned long int)random(); }
 unsigned int random( unsigned int u ) { return random( (unsigned long int)u ); }
 unsigned int random( unsigned int l, unsigned int u ) { return random( (unsigned long int)l, (unsigned long int)u ); }
}

float random( void );
double random( void );
float _Complex random( void );
double _Complex random( void );
long double _Complex random( void );



# 1 "/home/a3moss/bin/cfa/include/cfa/common.hfa" 1
# 16 "/home/a3moss/bin/cfa/include/cfa/common.hfa"
       



[ int, int ] div( int num, int denom );
[ long int, long int ] div( long int num, long int denom );
[ long long int, long long int ] div( long long int num, long long int denom );
forall( otype T | { T ?/?( T, T ); T ?%?( T, T ); } )
[ T, T ] div( T num, T demon );



extern "C" {
 int abs( int );
 long int labs( long int );
 long long int llabs( long long int );
}

static inline {
 unsigned char abs( signed char v ) { return abs( (int)v ); }

 unsigned long int abs( long int v ) { return labs( v ); }
 unsigned long long int abs( long long int v ) { return llabs( v ); }
}

extern "C" {
 double fabs( double );
 float fabsf( float );
 long double fabsl( long double );
}
static inline {
 float abs( float x ) { return fabsf( x ); }
 double abs( double x ) { return fabs( x ); }
 long double abs( long double x ) { return fabsl( x ); }
}

extern "C" {
 double cabs( double _Complex );
 float cabsf( float _Complex );
 long double cabsl( long double _Complex );
}
static inline {
 float abs( float _Complex x ) { return cabsf( x ); }
 double abs( double _Complex x ) { return cabs( x ); }
 long double abs( long double _Complex x ) { return cabsl( x ); }
}

forall( otype T | { void ?{}( T &, zero_t ); int ?<?( T, T ); T -?( T ); } )
T abs( T );



static inline {
 forall( otype T | { int ?<?( T, T ); } )
 T min( T t1, T t2 ) { return t1 < t2 ? t1 : t2; }

 forall( otype T | { int ?>?( T, T ); } )
 T max( T t1, T t2 ) { return t1 > t2 ? t1 : t2; }

 forall( otype T | { T min( T, T ); T max( T, T ); } )
 T clamp( T value, T min_val, T max_val ) { return max( min_val, min( value, max_val ) ); }

 forall( otype T )
 void swap( T & v1, T & v2 ) { T temp = v1; v1 = v2; v2 = temp; }
}
# 254 "/home/a3moss/bin/cfa/include/cfa/stdlib.hfa" 2
# 18 "raii/dtor-early-exit.c" 2
# 1 "/home/a3moss/bin/cfa/include/cfa/stdhdr/assert.h" 1
# 17 "/home/a3moss/bin/cfa/include/cfa/stdhdr/assert.h"
extern "C" {


# 1 "/usr/include/assert.h" 1 3 4
# 66 "/usr/include/assert.h" 3 4




# 69 "/usr/include/assert.h" 3 4
extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));


extern void __assert_perror_fail (int __errnum, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));




extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));



# 21 "/home/a3moss/bin/cfa/include/cfa/stdhdr/assert.h" 2
# 29 "/home/a3moss/bin/cfa/include/cfa/stdhdr/assert.h"
 
# 29 "/home/a3moss/bin/cfa/include/cfa/stdhdr/assert.h"
void __assert_fail_f( const char *assertion, const char *file, unsigned int line, const char *function, const char *fmt, ... ) __attribute__((noreturn, format( printf, 5, 6) ));
# 42 "/home/a3moss/bin/cfa/include/cfa/stdhdr/assert.h"
}
# 19 "raii/dtor-early-exit.c" 2

struct A {
 const char * name;
 int * x;
};


void ?{}(A & a) { 
# 26 "raii/dtor-early-exit.c" 3 4
                 ((0) ? (void) (0) : __assert_fail (
# 26 "raii/dtor-early-exit.c"
                 "false"
# 26 "raii/dtor-early-exit.c" 3 4
                 , "raii/dtor-early-exit.c", 26, __PRETTY_FUNCTION__))
# 26 "raii/dtor-early-exit.c"
                                ; }
void ?{}(A & a, const char * name) { a.name = name; sout | "construct " | name | endl; a.x = (int*)malloc(); }
void ?{}(A & a, const char * name, int * ptr) { 
# 28 "raii/dtor-early-exit.c" 3 4
                                               ((0) ? (void) (0) : __assert_fail (
# 28 "raii/dtor-early-exit.c"
                                               "false"
# 28 "raii/dtor-early-exit.c" 3 4
                                               , "raii/dtor-early-exit.c", 28, __PRETTY_FUNCTION__))
# 28 "raii/dtor-early-exit.c"
                                                              ; }

A ?=?(A & a, A b) { sout | "assign " | a.name | " " | b.name; return a; }
void ?{}(A & a, A b) { sout | "copy construct " | b.name | endl; a.x = (int*)malloc(); }
void ^?{}(A & a) { sout | "destruct " | a.name | endl; free(a.x); }


void f(int i) {
 sout | "f i=" | i | endl;
 A x = { "x" };
 {
  A y = { "y" };
  {
   A z = { "z" };
   {
    if (i == 0) return;
   }
   if (i == 1) return;

  }
  if (i == 2) return;

 }
 return;
}


void g() {
 for (int i = 0; i < 10; i++) {
  sout | "g for i=" | i | endl;
  A x = { "x" };


 }
 sout | endl;
 {
  int i = 0;
  while (i < 10) {
   sout | "g while i=" | i | endl;
   A x = { "x" };

   i++;

  }
 }
 sout | endl;
 for (int i = 0; i < 10; i++) {
  switch(10) {
   case 0:
   case 5:
   case 10: {
    A y = { "y" };
    sout | "g switch i=" | i | endl;

    break;
   }
   default: {
    sout | "g switch i=" | i | endl;
    A x = { "x" };

    break;
   }
  }
 }
 sout | endl;
 for (int k = 0; k < 2; k++) {
  sout | "g for k=" | k | endl;
  L1: for (int i = 0; i < 10; i++) {
   sout | "g for i=" | i | endl;

   A x = { "x" };
   if (i == 2) {
    sout | "continue L1" | endl;
    continue;
   } else if (i == 3) {
    sout | "break L1" | endl;
    break;
   }

   L2: for (int j = 0; j < 10; j++) {
    sout | "g for j=" | j | endl;
    A y = { "y" };
    if (j == 0) {
     sout | "continue L2" | endl;
     continue;
    } else if (j == 1) {
     sout | "break L2" | endl;
     break;
    } else if (i == 1) {
     sout | "continue L1" | endl;
     continue L1;
    } else if (k == 1) {
     sout | "break L1" | endl;
     break L1;
    }
   }
  }
 }

 sout | endl;
 L3: if( 3 ) {
  A w = { "w" };
  if( 4 ) {
   A v = { "v" };
   sout | "break L3" | endl;
   break L3;
  }
 }
}


void h() {
 int i = 0;





 sout | "h" | endl;
 {
  L0: ;



   A y = { "y" };

  L1: sout | "L1" | endl;
   A x = { "x" };

  L2: sout | "L2" | endl;
   if (i == 0) {
    ++i;
    sout | "goto L1" | endl;

    goto L1;


   } else if (i == 1) {
    ++i;
    sout | "goto L2" | endl;

    goto L2;


   } else if (i == 2) {
    ++i;
    sout | "goto L3" | endl;

    goto L3;


   } else if (
# 179 "raii/dtor-early-exit.c" 3 4
             0
# 179 "raii/dtor-early-exit.c"
                  ) {
    ++i;
    A z = { "z" };
    sout | "goto L3-2" | endl;

    goto L3;


   } else {
    ++i;
    sout | "goto L4" | endl;

    goto L4;


   }

  L3: sout | "L3" | endl;
   sout | "goto L2-2" | endl;

   goto L2;


 }

 L4: sout | "L4" | endl;
 if (i == 4) {
  sout | "goto L0" | endl;

  goto L0;


 }





}


void computedGoto() {

  void *ptr;
  ptr = &&foo;
  goto *ptr;
  
# 225 "raii/dtor-early-exit.c" 3 4
 ((0) ? (void) (0) : __assert_fail (
# 225 "raii/dtor-early-exit.c"
 "false"
# 225 "raii/dtor-early-exit.c" 3 4
 , "raii/dtor-early-exit.c", 225, __PRETTY_FUNCTION__))
# 225 "raii/dtor-early-exit.c"
              ;
foo: ;
# 235 "raii/dtor-early-exit.c"
}

int main() {
 sepDisable(sout);
 for (int i = 0; i < 4; i++) {
  f(i);
 }
 sout | endl;
 g();
 sout | endl;
 h();

 computedGoto();
}
