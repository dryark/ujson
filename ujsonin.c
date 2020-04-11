#include<stdio.h>
#include"stack.h"
#include"misc.h"
#include"red_black_tree.h"
#include"string-tree.h"

#include"stack.c"
#include"misc.c"
#include"red_black_tree.c"
#include"string-tree.c"
#include"helpers.c"

typedef struct jnode_s jnode;

#define NODEBASE char type; jnode *parent;
// type 1=hash, 2=str

#define SAFE if(pos>=len) goto Done;
#define SAFEGET if(pos>=len) goto Done; let=data[pos++];
#define SPACES for( int j=0;j<depth;j++ ) printf("  ");

struct jnode_s { NODEBASE };

typedef struct node_hash_s { NODEBASE
    string_tree *tree;
} node_hash;

typedef struct node_str_s { NODEBASE
    char *str;
    int len;
} node_str;

node_hash *parse( char *data, int len, int *err );
jnode *node_hash__get( node_hash *self, char *key, int keyLen );
void jnode__dump( jnode *self, int depth );

int main( int argc, char *argv[] ) {
    int len;
    char *data = slurp_file( "test.json", &len );
    int err;
    node_hash *root = parse( data, len, &err );
    jnode__dump( (jnode *) root, 0 );
    return 0;
}

node_hash *node_hash__new() {
    node_hash *self = ( node_hash * ) calloc( sizeof( node_hash ), 1 );
    self->type = 1;
    self->tree = string_tree__new();
    return self;
}

node_str *node_str__new( char *str, int len ) {
    node_str *self = ( node_str * ) calloc( sizeof( node_str ), 1 );
    self->type = 2;
    self->str = str;
    self->len = len;
    return self;
}

void node_hash__dump( node_hash *self, int depth ) {
    xjr_key_arr *keys = string_tree__getkeys( self->tree );
    printf("{\n");
    for( int i=0;i<keys->count;i++ ) {
        char *key = keys->items[i];
        int len = keys->sizes[i];
        SPACES printf("\"%.*s\":",len,key);
        jnode__dump( node_hash__get( self, key, len ), depth );
    }
    depth--;
    SPACES printf("}\n");
}

void jnode__dump( jnode *self, int depth ) {
    if( self->type == 1 ) node_hash__dump( (node_hash *) self, depth+1 );
    else if( self->type == 2 ) printf("\"%.*s\"\n", ( (node_str *) self )->len, ( (node_str *) self )->str );
}

void node_hash__store( node_hash *self, char *key, int keyLen, jnode *node ) {
    string_tree__store_len( self->tree, key, keyLen, (void *) node, 0 );
}

jnode *node_hash__get( node_hash *self, char *key, int keyLen ) {
    char type;
    return (jnode *) string_tree__get_len( self->tree, key, keyLen, &type );
}

char nullStr[2] = { 0, 0 };

node_hash *parse( char *data, int len, int *err ) {
    int pos = 1, keyLen;
    char *keyStart, *strStart, let;
    
    if( data[0] != '{' ) {
        *err = 1;
        return NULL;
    }
    node_hash *root = node_hash__new();
    jnode *cur = ( jnode * ) root;
Hash: SAFE
    let = data[pos++];
    if( let == '"' ) goto KeyName1;
    if( let == '}' && cur->parent ) cur = cur->parent;
    goto Hash;
KeyName1: SAFE
    keyStart = &data[pos++];
KeyNameX: SAFEGET
    if( let == '"' ) {
        keyLen = &data[pos-1] - keyStart;
        goto Colon;
    }
    goto KeyNameX;
Colon: SAFEGET
    if( let == ':' ) goto AfterColon;
    goto Colon;
AfterColon: SAFEGET
    if( let == '"' ) goto String1;
    if( let == '{' ) {
        node_hash *newHash = node_hash__new();
        newHash->parent = cur;
        if( cur->type == 1 ) node_hash__store( (node_hash *) cur, keyStart, keyLen, (jnode *) newHash );
        cur = (jnode *) newHash;
        goto Hash;
    }
    // if( let == 't' || let == 'f' ) ... for true/false
    // if( let >= '0' && let <= '9' ) ... for numbers
    // if( let == '[' ) ... for array
    goto AfterColon;
String1: SAFEGET
    if( let == '"' ) {
       jnode *newStr = (jnode *) node_str__new( nullStr, 0 );
       node_hash__store( (node_hash *) cur, keyStart, keyLen, newStr );
       goto AfterVal;
    }
    strStart = &data[pos-1];
StringX: SAFEGET
    if( let == '"' ) {
       int strLen = &data[pos-1] - strStart;
       if( cur->type == 1 ) {
           jnode *newStr = (jnode *) node_str__new( strStart, strLen );
           node_hash__store( (node_hash *) cur, keyStart, keyLen, newStr );
       }
       goto AfterVal;
    }
    goto StringX;   
AfterVal: SAFE
    // who cares about commas in between things; we can just ignore them :D
    goto Hash;
Done:
    return root;
}