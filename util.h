#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

// General utility functions

void panic( char const * fmt, ... );
char * read_file( char const * filename ); // mallocs result, you need to free()
void dump_dwords( unsigned int const * vals, unsigned int num_dwords );

// pixel compare that returns position of first mismatch
// pos_x / pos_y may both be NULL.
int pixel_compare_pos( unsigned char const * a, int stride_a, unsigned char const * b, int stride_b, int w, int h, int * pos_x, int * pos_y );

// pixel compare without position reporting
int pixel_compare( unsigned char const * a, int stride_a, unsigned char const * b, int stride_b, int w, int h );
void print_pixels( unsigned char const * a, int stride_a, unsigned char const * b, int stride_b, int w, int h );

typedef struct run_stats run_stats;

run_stats * run_stats_create( void );
void run_stats_destroy( run_stats * stats );
void run_stats_clear( run_stats * stats ); // reset all measurements
void run_stats_record( run_stats * stats, float value ); // record a measurement
void run_stats_report( run_stats * stats, char const * desc ); // print a report

#ifdef __cplusplus
}
#endif

#endif
