#ifndef __UJSONIN_H
#define __UJSONIN_H
#include"string-tree.h"
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
    char *str;
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

node_hash *parse( char *data, int len, parser_state *beginState, int *err );
node_hash *parse_file( const char *filename, int *err );

node_hash *node_hash__new();
jnode *node_hash__get( node_hash *self, char *key, int keyLen );
void node_hash__store( node_hash *self, char *key, int keyLen, jnode *node );

node_str *node_str__new( char *str, int len, char type );
node_str *node_str__new_from_json( char *str, int len );

node_arr *node_arr__new();
void node_arr__add( node_arr *self, jnode *el );

jnode *node_null__new();

void jnode__dump( jnode *self, int depth );
sds jnode__str( jnode *self, int depth, sds str );
char *slurp_file( const char *filename, int *outlen );
void ujsonin_init();
void jnode__dump_env( jnode *self );
void node_hash__dump_to_makefile( node_hash *self, char *prefix );
void node_hash__delete( node_hash *self );
node_hash *parse_with_default( char *file, char *def, char **d1, char **d2 );
#endif