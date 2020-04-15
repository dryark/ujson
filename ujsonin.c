#include<stdio.h>
#include<stdint.h>
#include"red_black_tree.h"
#include"string-tree.h"

#include"red_black_tree.c"
#include"string-tree.c"

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

typedef struct node_arr_s { NODEBASE
    jnode *head;
    jnode *tail;
    int count;
} node_arr;

node_hash *parse( char *data, int len, int *err );
jnode *node_hash__get( node_hash *self, char *key, int keyLen );
void jnode__dump( jnode *self, int depth );
char *slurp_file( char *filename, int *outlen );

int main( int argc, char *argv[] ) {
    int len;
    char *data = slurp_file( "test.json", &len );
    int err;
    node_hash *root = parse( data, len, &err );
    jnode__dump( (jnode *) root, 0 );
    return 0;
}

char *slurp_file( char *filename, int *outlen ) { 
    char *data;
    FILE *fh = fopen( filename, "r" );
    fseek( fh, 0, SEEK_END );
    long int fileLength = ftell( fh );
    fseek( fh, 0, SEEK_SET );
    char *input = malloc( fileLength + 1 );
    input[ fileLength ] = 0x00;
    fread( input, (size_t) fileLength, (size_t) 1, fh );
    *outlen = fileLength;
    return input;
}

node_hash *node_hash__new() {
    node_hash *self = ( node_hash * ) calloc( sizeof( node_hash ), 1 );
    self->type = 1;
    self->tree = string_tree__new();
    return self;
}

node_str *node_str__new( char *str, int len, char type ) {
    node_str *self = ( node_str * ) calloc( sizeof( node_str ), 1 );
    self->type = type; // 2 is str, 4 is number, 5 is a negative number
    self->str = str;
    self->len = len;
    return self;
}

node_arr *node_arr__new() {
    node_arr *self = ( node_arr * ) calloc( sizeof( node_arr ), 1 );
    self->type = 3;
    return self;
}

void node_arr__add( node_arr *self, jnode *el ) {
    self->count++;
    if( !self->head ) {
        self->head = self->tail = el;
        return;
    }
    self->tail->parent = el;
    self->tail = el;
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

void node_arr__dump( node_arr *self, int depth ) {
    printf("[\n");
    jnode *cur = self->head;
    for( int i=0;i<self->count;i++ ) {
        SPACES jnode__dump( cur, depth+1 );
        cur = cur->parent; // parent is being reused as next
    }
    depth--;
    SPACES printf("]\n");
}

void jnode__dump( jnode *self, int depth ) {
    if( self->type == 1 ) node_hash__dump( (node_hash *) self, depth+1 );
    else if( self->type == 2 ) printf("\"%.*s\"\n", ( (node_str *) self )->len, ( (node_str *) self )->str );
    else if( self->type == 3 ) node_arr__dump( (node_arr *) self, depth+1 );
    else if( self->type == 4 ) printf("%.*s\n", ( (node_str *) self )->len, ( (node_str *) self )->str );
    else if( self->type == 5 ) printf("-%.*s\n", ( (node_str *) self )->len, ( (node_str *) self )->str );
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
    uint8_t neg = 0;
    char *keyStart, *strStart, let;
    
    node_hash *root = node_hash__new();
    jnode *cur = ( jnode * ) root;
Hash: SAFE
    let = data[pos++];
    if( let == '"' ) goto QQKeyName1;
    if( let == '\'' ) goto QKeyName1;
    if( let >= 'a' && let <= 'z' ) { pos--; goto KeyName1; }
    if( let == '}' && cur->parent ) cur = cur->parent;
    if( let == '/' && pos < (len-1) ) {
        if( data[pos] == '/' ) { pos++; goto HashComment; }
        if( data[pos] == '*' ) { pos++; goto HashComment2; }
    }
    goto Hash;
HashComment: SAFEGET
    if( let == 0x0d || let == 0x0a ) goto Hash;
    goto HashComment;
HashComment2: SAFEGET
    if( let == '*' && pos < (len-1) && data[pos] == '/' ) { pos++; goto Hash; }
    goto HashComment2;
QQKeyName1: SAFE
    let = data[pos];
    keyStart = &data[pos++];
    if( let == '\\' ) pos++;
QQKeyNameX: SAFEGET
    if( let == '\\' ) { pos++; goto QQKeyNameX; }
    if( let == '"' ) {
        keyLen = &data[pos-1] - keyStart;
        goto Colon;
    }
    goto QQKeyNameX;
QKeyName1: SAFE
    let = data[pos];
    keyStart = &data[pos++];
    if( let == '\\' ) pos++;
QKeyNameX: SAFEGET
    if( let == '\\' ) { pos++; goto QKeyNameX; }
    if( let == '\'' ) {
        keyLen = &data[pos-1] - keyStart;
        goto Colon;
    }
    goto QKeyNameX;
KeyName1: SAFE
    keyStart = &data[pos++];
KeyNameX: SAFEGET
    if( let == ':' ) {
        keyLen = &data[pos-1] - keyStart;
        goto AfterColon;
    }
    if( let == ' ' || let == '\t' ) {
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
        if( cur->type == 3 ) node_arr__add( (node_arr *) cur, (jnode *) newHash );
        cur = (jnode *) newHash;
        goto Hash;
    }
    if( let == '/' && pos < (len-1) ) {
        if( data[pos] == '/' ) { pos++; goto AC_Comment; }
        if( data[pos] == '*' ) { pos++; goto AC_Comment2; }
    }
    // if( let == 't' || let == 'f' ) ... for true/false
    if( let >= '0' && let <= '9' ) { neg=0; goto Number1; }
    if( let == '-' ) { neg=1; pos++; goto Number1; }
    if( let == '[' ) {
        node_arr *newArr = node_arr__new();
        newArr->parent = cur;
        if( cur->type == 1 ) node_hash__store( (node_hash *) cur, keyStart, keyLen, (jnode *) newArr );
        if( cur->type == 3 ) node_arr__add( (node_arr *) cur, (jnode *) newArr );
        cur = (jnode *) newArr;
        goto AfterColon;
    }
    if( let == ']' ) {
        cur = cur->parent;
        if( cur->type == 3 ) goto AfterColon;
        if( cur->type == 1 ) goto Hash;
        // should never reach here
    }
    goto AfterColon;
Arr: SAFEGET
    // TODO: stack of array tails
    if( let == ']' ) {
        goto AfterVal;
    }
    goto Arr;
AC_Comment: SAFEGET
    if( let == 0x0d || let == 0x0a ) goto AfterColon;
    goto AC_Comment;
AC_Comment2: SAFEGET
    if( let == '*' && pos < (len-1) && data[pos] == '/' ) { pos++; goto Hash; }
    goto AC_Comment2;
Number1: SAFE
    strStart = &data[pos-1];
NumberX: SAFEGET
    //if( let == '.' ) goto AfterDot;
    if( let < '0' || let > '9' ) {
        int strLen = &data[pos-1] - strStart;
        jnode *newStr = (jnode *) node_str__new( strStart, strLen, neg ? 5 : 4 );
        if( cur->type == 1 ) {
            node_hash__store( (node_hash *) cur, keyStart, keyLen, newStr );
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            node_arr__add( (node_arr *) cur, newStr );
            goto AfterColon;
        }
    }
    goto NumberX;
//AfterDot: SAFEGET
String1: SAFEGET
    if( let == '"' ) {
        jnode *newStr = (jnode *) node_str__new( nullStr, 0, 2 );
        if( cur->type == 1 ) {
            node_hash__store( (node_hash *) cur, keyStart, keyLen, newStr );
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            node_arr__add( (node_arr *) cur, newStr );
            goto AfterColon;
        }
        goto AfterVal; // Should never be reached
    }
    strStart = &data[pos-1];
StringX: SAFEGET
    if( let == '"' ) {
       int strLen = &data[pos-1] - strStart;
       jnode *newStr = (jnode *) node_str__new( strStart, strLen, 2 );
       if( cur->type == 1 ) {
           node_hash__store( (node_hash *) cur, keyStart, keyLen, newStr );
           goto AfterVal;
       }
       if( cur->type == 3 ) {
           node_arr__add( (node_arr *) cur, newStr );
           goto AfterColon;
       }
       goto AfterVal; // should never be reached
    }
    goto StringX;   
AfterVal: SAFE
    // who cares about commas in between things; we can just ignore them :D
    goto Hash;
Done:
    return root;
}