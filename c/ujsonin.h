#ifndef __UJSONIN_H
#define __UJSONIN_H
#include"string-tree.h"

// sds is used for strings
// Anytime you get a sds back, you can treat it like a char*, but you must sdsfree it when done.
#include"sds.h"

typedef struct jnode_s jnode;

#define NODEBASE char type; jnode *parent;
// type 1=hash, 2=str

#define SAFE(x) if(pos>=len) { endstate=x; goto Done; }
#define SAFEGET(x) if(pos>=len) { endstate=x; goto Done; } let=data[pos++];
#define SPACES for( int j=0;j<depth;j++ ) printf("  ");
//#define SPACESCAT for( int j=0;j<depth;j++ ) str = sdscat( str, "  ");
//#define SPACESCAT str = add_indent( str, depth );

struct jnode_s { NODEBASE };

typedef struct node_hash_s { NODEBASE
    string_tree *tree;
} node_hash;

typedef struct node_str_s { NODEBASE
    const char *str;
    int len;
    char alloc;
} node_str;

typedef struct node_arr_s { NODEBASE
    jnode *head;
    jnode *tail;
    int count;
} node_arr;

typedef struct parser_state_s {
    int state;
} parser_state;

node_hash *parse( const char *data, int len, parser_state *beginState, int *err );
node_hash *parse_file( const char *filename, int *err );

node_hash *node_hash__new();
void node_hash__delete( node_hash *node );
jnode *node_hash__get( node_hash *self, const char *key, int keyLen );
sds node_hash__get_str( node_hash *self, const char *key, int keyLen );
void node_hash__store( node_hash *self, const char *key, int keyLen, jnode *node );

node_str *node_str__new( const char *str, int len, char type );
node_str *node_str__new_from_json( const char *str, int len );

node_arr *node_arr__new();
void node_arr__add( node_arr *self, jnode *el );

jnode *node_null__new();

// This is an ugly method to dump the contents of a jnode.
// The output is not a proper JSON representation. You probably shouldn't use this.
void jnode__dump( jnode *self, int depth );

// Get the JSON representation of the jnode. If run on a string, will give a proper JSON string surrounded with quotes
sds jnode__json( jnode *self, int depth, sds str );

// Get the raw value of a string in jnode form
sds jnode__str( jnode *self ); // str value of node

char *slurp_file( const char *filename, int *outlen );
void ujsonin_init();
void jnode__dump_env( jnode *self );
void node_hash__dump_to_makefile( node_hash *self, char *prefix );
void node_hash__delete( node_hash *self );
node_hash *parse_with_default( char *file, char *def, char **d1, char **d2 );
#endif