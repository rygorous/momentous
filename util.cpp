#define _CRT_SECURE_NO_WARNINGS
#include "util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vector>
#include <algorithm>

void panic(char const * fmt, ...)
{
    va_list arg;
    va_start( arg, fmt );
    fputs( "Error: ", stderr );
    vfprintf( stderr, fmt, arg );
    va_end( arg );
    exit( 1 );
}

char * read_file(char const * filename)
{
    FILE *f = fopen( filename, "rb" );
    if ( !f )
        return 0;

    fseek( f, 0, SEEK_END );
    size_t sz = ftell( f );
    fseek( f, 0, SEEK_SET );

    char * buffer = (char *)malloc( sz + 1 );
    if (buffer) {
        buffer[ sz ] = 0;
        if (fread( buffer, sz, 1, f ) != 1) {
            free( buffer );
            buffer = 0;
        }
    }

    fclose( f );
    return buffer;
}

void dump_dwords( unsigned int const * vals, unsigned int num_dwords )
{
    for ( unsigned int row = 0 ; row < num_dwords ; row += 8 )
    {
        printf( "[%04x]", row );
        for ( unsigned int i = 0 ; i < 8 && row + i < num_dwords ; i++)
            printf( " %08x", vals[ row + i ] );
        printf( "\n" );
    }
}

int pixel_compare_pos( unsigned char const * a, int stride_a, unsigned char const * b, int stride_b, int w, int h, int * pos_x, int * pos_y )
{
    for ( int y = 0 ; y < h ; y++ )
    {
        unsigned char const * pa = a + y * stride_a;
        unsigned char const * pb = b + y * stride_b;
        int d = memcmp( pa, pb, w );
        if ( d != 0 )
        {
            if ( pos_y )
                *pos_y = y;

            if ( pos_x )
            {
                // we know there's a mismatch in this line
                // find its x position
                int x = 0;
                while ( pa[x] == pb[x] )
                    x++;

                *pos_x = x;
            }

            return d;
        }
    }

    return 0;
}

int pixel_compare( unsigned char const * a, int stride_a, unsigned char const * b, int stride_b, int w, int h )
{
    return pixel_compare_pos( a, stride_a, b, stride_b, w, h, NULL, NULL );
}

void print_pixels( unsigned char const * a, int stride_a, unsigned char const * b, int stride_b, int w, int h )
{
    for ( int y = 0 ; y < h ; y++ )
    {
        for ( int x = 0 ; x < w ; x++ )
            printf( "%02x ", a[x] );

        printf(" - ");

        for ( int x = 0 ; x < w ; x++ )
            printf( " %02x", b[x] );

        printf("\n");
        a += stride_a;
        b += stride_b;
    }
}

struct run_stats
{
    std::vector<float> values;
};

run_stats * run_stats_create( )
{
    return new run_stats;
}

void run_stats_destroy( run_stats * stats )
{
    delete stats;
}

void run_stats_clear( run_stats * stats )
{
    stats->values.clear();
}

void run_stats_record( run_stats * stats, float val )
{
    stats->values.push_back( val );
}

void run_stats_report( run_stats * stats, char const * desc )
{
    size_t count = stats->values.size();
    if (count < 2)
        return;

    // Print min/max and different percentiles
    std::sort(stats->values.begin(), stats->values.end());

    // desc, min,25th,med,75th,max, mean,sdev
    char buffer[512];
    char *p = buffer;

    p += sprintf(p, "%s, ", desc );

    for (int i=0; i < 5; i++)
        p += sprintf(p, "%.3f,", stats->values[i * (count - 1) / 4]);

    // Mean and standard deviation
    double mean = 0.0;
    for (std::vector<float>::const_iterator it = stats->values.begin(); it != stats->values.end(); ++it)
        mean += *it;
    mean /= count;

    double varsum = 0.0;
    for (std::vector<float>::const_iterator it = stats->values.begin(); it != stats->values.end(); ++it)
        varsum += (*it - mean) * (*it - mean);
    double sdev = sqrt(varsum / (count - 1.0));

    p += sprintf(p, " %.3f,%.3f\n", mean, sdev );
    printf( "%s", buffer );
}
