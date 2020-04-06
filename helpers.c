#include <stdint.h>

char *slurp_file( char *filename, int *outlen ) { 
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
    
    *outlen = fileLength;
    return input;
}
