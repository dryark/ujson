#include "xjr-machine.h"
#include <stdint.h>

xjr_node *parse_file( xjr_mempool *pool, char *filename, char **bufferPtr ) { 
    if( !filename ) {
        fprintf( stderr, "Filename not specified to open\n" );
        return 0;
    }
    char *data;
    FILE *fh = fopen( filename, "r" );
    if( !fh ) {
        fprintf( stderr, "Could not open %s\n", filename );
        return 0;
    }
    fseek( fh, 0, SEEK_END );
    long int fileLength = ftell( fh );
    fseek( fh, 0, SEEK_SET );
    if( fileLength > UINT16_MAX ) {
        fprintf( stderr, "XJR file is over %li bytes long; cannot open\n", (long int) UINT16_MAX );
        fclose( fh );
        return 0;
    }
    
    char *input = malloc( fileLength + 1 );
    input[ fileLength ] = 0x00;
    fread( input, (size_t) fileLength, (size_t) 1, fh );
    printf("Going to parse `%.*s`\n", fileLength, input );
    
    *bufferPtr = input;
    return parse_onto( pool, input, fileLength, 0 );
}

// Return a null terminated value of a node
// String is an allocated copy so it should be freed
char *xjr_node__valuez( xjr_node *self ) {
    int len;
    char *rawValue = xjr_node__value( self, &len );
    char *withz = malloc( len + 1 );
    memcpy( withz, rawValue, len );
    withz[ len ] = 0x00;
    return withz;
}

// Both get a node and then its value
char *xjr_node__get_valuez( xjr_node *self, char *name, int nameLen ) {
    xjr_node *node = xjr_node__get( self, name, nameLen );
    if( !node ) return 0;
    return xjr_node__valuez( node ); 
}
