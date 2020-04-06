#include"stack.h"
#include"misc.h"
#include"red_black_tree.h"
#include"string-tree.h"

#include"stack.c"
#include"misc.c"
#include"red_black_tree.c"
#include"string-tree.c"
#include"helpers.c"
#include<stdio.h>

typedef struct jnode_s jnode;
typedef struct node_hash_s node_hash;

struct jnode_s {
    int type; // 1=hash, 2=str
    void *node;
    jnode *parent;
};

struct node_hash_s {
    string_tree *tree;
};

typedef struct node_str_s node_str;
struct node_str_s {
    char *str;
    int len;
};

jnode *parse( char *data, int len, int *err );
jnode *node_hash__get( node_hash *self, char *key, int keyLen );
void jnode__dump( jnode *self );

int main( int argc, char *argv[] ) {
    int len;
    char *data = slurp_file( "test.json", &len );
    //printf("Raw file:%.*s", len, data );
    int err;
    jnode *node = parse( data, len, &err );
    node_hash *root = node->node;
    jnode *xnode = node_hash__get( root, "x", 1 );
    //printf("fetch x jnode: %p, node: %p\n", xnode, NULL );//xnode->node );
    //printf("type of x node: %i\n", xnode->type );
    
    jnode__dump( node );
    return 0;
}

node_hash *node_hash__new() {
    node_hash *self = ( node_hash * ) calloc( sizeof( node_hash ), 1 );
    self->tree = string_tree__new();
    return self;
}

node_str *node_str__new( char *str, int len ) {
    node_str *self = ( node_str * ) calloc( sizeof( node_str ), 1 );
    self->str = str;
    self->len = len;
    return self;
}

jnode *jnode__new( jnode *parent ) {
    jnode *self = ( jnode * ) calloc( sizeof( jnode ), 1 );
    self->type = 0;
    self->parent = parent;
    return self;
}

void node_hash__dump( node_hash *self ) {
    xjr_key_arr *keys = string_tree__getkeys( self->tree );
    int count = keys->count;
    for( int i=0;i<count;i++ ) {
        char *key = keys->items[i];
        int len = keys->sizes[i];
        printf("key:%.*s\n",len,key);
        jnode *val = node_hash__get( self, key, len );
        jnode__dump( val );
    }
}

void node_str__dump( node_str *self ) {
    printf("str: %.*s\n", self->len, self->str );
}

void jnode__dump( jnode *self ) {
    int type = self->type;
    if( type == 1 ) {
        node_hash *hash = (node_hash *) self->node;
        node_hash__dump( hash );
    }
    if( type == 2 ) {
        node_str *str = (node_str *) self->node;
        node_str__dump( str );
    }
}

jnode *jnode__new_hash( jnode *parent ) {
    jnode *self = ( jnode * ) calloc( sizeof( jnode ), 1 );
    self->type = 1;
    
    node_hash *node = node_hash__new();
    self->node = node;
    self->parent = parent;
    return self;
}

jnode *jnode__new_str( char *str, int len ) {
    jnode *self = ( jnode * ) calloc( sizeof( jnode ), 1 );
    self->type = 2;
    
    node_str *node = node_str__new( str, len );
    self->node = node;
    return self;
}

void node_hash__store( node_hash *self, char *key, int keyLen, jnode *node ) {
    if( node->type == 1 ) {
        node_hash *hashN = (node_hash *) node->node;
        printf("Storing key %.*s hash %p tree %p on tree %p\n", keyLen, key, hashN, hashN->tree, self->tree );
    }
    if( node->type == 2 ) {
        node_str *strN = (node_str *) node->node;
        printf("Storing key %.*s str %.*s on tree %p\n", keyLen, key, strN->len, strN->str, self->tree );
    }
    string_tree__store_len( self->tree, key, keyLen, (void *) node, 0 );
}

jnode *node_hash__get( node_hash *self, char *key, int keyLen ) {
    printf("Attempting to fetch '%.*s' from tree %p\n", keyLen, key, self->tree );
    char type;
    return (jnode *) string_tree__get_len( self->tree, key, keyLen, &type );
}

jnode *parse( char *data, int len, int *err ) {
    int pos = 1;
    if( data[0] != '{' ) {
        *err = 1;
        return NULL;
    }
    jnode *root = jnode__new_hash( NULL );
    printf("root jnode: %p\n", root );
    
    jnode *cur = root;
    int curType = 1; // currently in a hash
    node_hash *curHash = root->node;
    printf("root hash: %p\n", curHash );
    printf("root tree: %p\n", curHash->tree );
    
    char *keyStart;
    int keyLen;
    char *strStart;
    int strLen;
    char let;
Hash:
    if( pos >= len ) goto Done;
    let = data[pos++];
    if( let == '"' ) {
        goto KeyNameL1;
    }
    if( let == '}' ) {
        if( cur->parent ) {
            cur = cur->parent;
            curType = cur->type;
            if( curType == 1 ) curHash = cur->node;
        }
    }
    goto Hash;
KeyNameL1:
    keyStart = &data[pos++];
KeyNameLx:
    let = data[pos++];
    if( let == '"' ) {
        keyLen = &data[pos-1] - keyStart;
        //printf("key: %.*s\n", keyLen, keyStart );
        goto Colon;
    }
    goto KeyNameLx;
Colon:
    let = data[pos++];
    if( let == ':' ) goto AfterColon;
    goto Colon;
AfterColon:
    let = data[pos++];
    if( let == '"' ) {
        goto String1;
    }
    if( let == '{' ) {
        jnode *newHash = jnode__new_hash( cur );
        if( curType == 1 ) {
            node_hash__store( curHash, keyStart, keyLen, newHash );
        }
        cur = newHash;
        curHash = cur->node;
        curType = 1;
        
        goto Hash;
    }
    // if( let == 't' || let == 'f' ) ... for true/false
    // if( let >= '0' && let <= '9' ) ... for numbers
    // if( let == '[' ) ... for array
    goto AfterColon;
String1:
    let = data[pos];
    if( let == '"' ) {
        // empty string
    }
    strStart = &data[pos++];
StringX:
    let = data[pos++];
    if( let == '"' ) {
       // string finished; save it
       int strLen = &data[pos-1] - strStart;
       //printf("str: %.*s\n", strLen, strStart);
       
       if( curType == 1 ) {
           jnode *newStr = jnode__new_str( strStart, strLen );
           node_hash__store( curHash, keyStart, keyLen, newStr );
       }
       
       goto AfterVal;
    }
    goto StringX;   
AfterVal:
    // who cares about commas in between thing; we can just ignore them :D
    goto Hash;
Done:
    return root;
}