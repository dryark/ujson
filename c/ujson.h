// Copyright (C) 2025 David Helkowski
// Fair Coding License 1.0+

#ifndef __UJSON_H
#define __UJSON_H
#include"string-tree.h"
#include"red_black_tree.h"
#include"sdsalloc.h"
#include<stdint.h>

//#define UJDEBUG

// sds is used for strings
// Anytime you get a sds back, you can treat it like a char*, but you must sdsfree it when done.
#include"sds.h"

typedef struct uj_node_s uj_node;

#define NODEBASE uint8_t type; uj_node *parent;
// type 1=hash, 2=str

#define SAFE(x) if(pos>=len) { endstate=x; goto Done; }
#define SAFEGET(x) if(pos>=len) { endstate=x; goto Done; } let=data[pos++];
#define SPACES for( unsigned j=0;j<depth;j++ ) printf("  ");
//#define SPACESCAT for( int j=0;j<depth;j++ ) str = sdscat( str, "  ");
//#define SPACESCAT str = add_indent( str, depth );

struct uj_node_s { NODEBASE };

typedef struct uj_hash_s { NODEBASE
    uint8_t refCnt;
    string_tree *tree;
} uj_hash;

#define NODE_STR_LEN_TYPE unsigned
// #define NODE_STR_LEN_TYPE long // If you really want strings over INT_MAX in length

typedef struct uj_str_s { NODEBASE
    const char *str;
    NODE_STR_LEN_TYPE len;
    char alloc;
} uj_str;

typedef struct uj_arr_s { NODEBASE
    uj_node *head;
    uj_node *tail;
    unsigned count;
} uj_arr;

typedef struct uj_state_s {
    int state;
} uj_state;

uj_hash *uj_parse( const char *data, unsigned long len, uj_state *beginState, int *err );
uj_hash *uj_parse_file( const char *filename, int *err );

uj_hash *uj_hash__new(void);
void uj_hash__delete( uj_hash *node );
uj_node *uj_hash__get( uj_hash *self, const char *key, unsigned keyLen );
sds uj_hash__get_str( uj_hash *self, const char *key, unsigned keyLen );
void uj_hash__store( uj_hash *self, const char *key, unsigned keyLen, uj_node *node );
void uj_hash__remove( uj_hash *self, const char *key, unsigned keylen );

uj_str *uj_str__new( const char *str, NODE_STR_LEN_TYPE len, uint8_t type );
uj_str *uj_str__new_from_json( const char *str, NODE_STR_LEN_TYPE len );

uj_arr *uj_arr__new(void);
void uj_arr__add( uj_arr *self, uj_node *el );

uj_node *uj_null__new(void);
uj_node *uj_true__new(void);
uj_node *uj_false__new(void);
uj_node *uj_bool__new( uint8_t val );

// This is an ugly method to dump the contents of a jnode.
// The output is not a proper JSON representation. You probably shouldn't use this.
void uj_node__dump( uj_node *self, unsigned depth );

// Get the JSONx representation of the jnode. If run on a string, will give a proper JSON string surrounded with quotes
sds uj_node__jsonx( uj_node *self, unsigned depth, sds str );

// Get the JSON representation of the jnode. If run on a string, will give a proper JSON string surrounded with quotes
sds uj_node__json( uj_node *self, unsigned depth, sds str );

// Get the raw value of a string in jnode form
sds uj_node__str( uj_node *self ); // str value of node

char *slurp_file( const char *filename, unsigned long *outlen );
void uj_init(void);
void uj_node__dump_env( uj_node *self );
void uj_hash__dump_to_makefile( uj_hash *self, char *prefix );
void uj_hash__delete( uj_hash *self );
uj_hash *uj_parse_with_default( char *file, char *def, char **d1, char **d2 );
#endif