// Copyright (C) 2025 David Helkowski
// Fair Coding License 1.0+

#include<stdio.h>
#include<stdint.h>
#include<limits.h>
#include"red_black_tree.h"
#include"string-tree.h"
#include"ujson.h"
#include"sds.h"

sds add_indent( sds str, int depth ) {
    if( depth <= 0 ) return str;
    const char spaces[] = "                              "; // 30 spaces
    if( depth <= 15 ) {
        return sdscat( str, &spaces[ 30 - depth * 2 ] );
    }
    sds res = str;
    for( int j=0;j<depth;j++ ) res = sdscat( res, "  ");
    return res;
}

uj_hash *uj_parse_with_default( char *file, char *def, char **d1, char **d2 ) {
    unsigned long flen;
    char *fdata = slurp_file( file, &flen );
    if( !fdata ) {
        printf("Could not open file '%s'\n", file );
        exit(1);
    }
    int ferr;
    uj_hash *froot = uj_parse( fdata, flen, NULL, &ferr );
    unsigned long dlen;
    char *ddata = NULL;
    int derr;
    
    uj_hash *both;
    if( def ) {
        ddata = slurp_file( def, &dlen );
        uj_hash *droot = uj_parse( ddata, dlen, NULL, &derr );
    }
    else {
        both = froot;
    }
    *d1 = fdata;
    if( ddata ) *d2 = ddata;
    return both;
}

char *slurp_file( const char *filename, unsigned long *outlen ) { 
    char *data;
    FILE *fh = fopen( filename, "r" );
    if( !fh ) {
        *outlen = -1;
        return 0;
    }
    fseek( fh, 0, SEEK_END );
    unsigned long fileLength = ftell( fh );
    fseek( fh, 0, SEEK_SET );
    char *input = malloc( fileLength + 1 );
    input[ fileLength ] = 0x00;
    fread( input, (size_t) fileLength, (size_t) 1, fh );
    *outlen = fileLength;
    return input;
}

uj_node *uj_node__new( int type ) {
    uj_node *self = ( uj_node * ) calloc( sizeof( uj_node ), 1 );
    self->type = type;
    return self;
}

uj_hash *uj_hash__new(void) {
    uj_hash *self = ( uj_hash * ) calloc( sizeof( uj_hash ), 1 );
    self->type = 1;
    self->tree = string_tree__new();
    // We are using refCnt 0 as 1, and 255 as 0
    return self;
}

uj_str *uj_str__new( const char *str, NODE_STR_LEN_TYPE len, uint8_t type ) {
    uj_str *self = ( uj_str * ) calloc( sizeof( uj_str ), 1 );
    self->type = type; // 2 is str, 4 is number, 5 is a negative number
    self->str = str;
    self->len = len;
    self->alloc = 0;
    return self;
}

char hex_to_num( char hex ) {
    if( hex >= '0' && hex <= '9' ) return hex - '0';
    if( hex >= 'a' && hex <= 'f' ) return hex - 'a' + 10;
    if( hex >= 'A' && hex <= 'F' ) return hex - 'A' + 10;
    return 0;
}

uj_str *uj_str__new_from_json( const char *str, NODE_STR_LEN_TYPE len ) {
    uj_str *self = ( uj_str * ) calloc( sizeof( uj_str ), 1 );
    
    char *alloc = malloc( len );
    
    NODE_STR_LEN_TYPE len2;
    char let;
    NODE_STR_LEN_TYPE io = 0;
    for( NODE_STR_LEN_TYPE i=0;i<len;i++ ) {
        let = str[i];
        if( let == '\\' ) {
            if( str[i+1] == 'u' ) {
                char b1 = hex_to_num( str[i+2] );
                char b2 = hex_to_num( str[i+3] );
                char b3 = hex_to_num( str[i+4] );
                char b4 = hex_to_num( str[i+5] );
                i += 5;
                if( !b1 && !b2 ) {
                    let = b4 + 16*b3;
                    if( let <= 0x7f ) {
                        alloc[io++] = let;
                        continue;
                    }
                    alloc[io++] = (char)(0xC0 | (let >> 6));
                    alloc[io++] = (char)(0x80 | (let & 0x3F));
                }
                uint16_t unicode = b4 + 16*b3 + 16*16*b2 + 16*16*16*b1;
                if (unicode <= 0x7F) {
                    // 1-byte character
                    alloc[io++] = (char)unicode;
                }
                else if (unicode <= 0x7FF) {
                    // 2-byte character
                    alloc[io++] = (char)(0xC0 | (unicode >> 6));
                    alloc[io++] = (char)(0x80 | (unicode & 0x3F));
                }
                else {
                    // 3-byte character
                    alloc[io++] = (char)(0xE0 | (unicode >> 12));
                    alloc[io++] = (char)(0x80 | ((unicode >> 6) & 0x3F));
                    alloc[io++] = (char)(0x80 | (unicode & 0x3F));
                }
                continue;
            }
            let = str[++i];
            if( let == 'n' ) {
                alloc[io++] = 0x0a;
                continue;
            }
            if( let == 'r' ) {
                alloc[io++] = 0x0d;
                continue;
            }
            if( let == '"' ) {
                alloc[io++] = '"';
                continue;
            }
            if( let == '\'' ) {
                alloc[io++] = '\'';
                continue;
            }
            io++;
            continue;
        }
        alloc[io++] = let;
    }
    alloc[io] = 0x00;
    len2 = io;
    
    self->type = 2; // 2 is str
    self->str = alloc;
    self->len = len2;
    self->alloc = 1;
    
    return self;
}

uj_arr *uj_arr__new(void) {
    uj_arr *self = ( uj_arr * ) calloc( sizeof( uj_arr ), 1 );
    self->type = 3;
    return self;
}

void uj_arr__add( uj_arr *self, uj_node *el ) {
    self->count++;
    if( !self->head ) {
        self->head = self->tail = el;
        return;
    }
    self->tail->parent = el;
    self->tail = el;
}

void uj_hash__dump( uj_hash *self, unsigned depth ) {
    xjr_key_arr *keys = string_tree__getkeys( self->tree );
    printf("{\n");
    for( unsigned i=0;i<keys->count;i++ ) {
        const char *key = keys->items[i];
        unsigned len = keys->sizes[i];
        SPACES printf ("\"%.*s\":", len, key );
        uj_node__dump( uj_hash__get( self, key, len ), depth );
    }
    depth--;
    SPACES printf("}\n");
}

sds uj_hash__str( uj_hash *self, unsigned depth, sds str ) {
    xjr_key_arr *keys = string_tree__getkeys( self->tree );
    str = sdscat( str, "{\n");
    for( unsigned i=0;i<keys->count;i++ ) {
        const char *key = keys->items[i];
        unsigned len = keys->sizes[i];
        str = add_indent( str, depth );
        str = sdscatfmt( str, "\"%p\":", (unsigned) len, key );
        str = uj_node__json( uj_hash__get( self, key, len ), depth, str );
    }
    str = add_indent( str, depth-1 );
    str = sdscat( str, "}\n" );
    return str;
}

void uj_arr__delete( uj_arr *arr );
void uj_str__delete( uj_str *node );

void uj_hash__delete( uj_hash *self ) {
    self->refCnt--;
    if( self->refCnt < 255 ) return; // 255 == no references; 0 = 1 reference
    xjr_key_arr *keys = string_tree__getkeys( self->tree );
    for( unsigned i=0;i<keys->count;i++ ) {
        const char *key = keys->items[i];
        unsigned len = keys->sizes[i];
        uj_node *sub = uj_hash__get( self, key, len );
        if( sub->type == 1 ) uj_hash__delete( (uj_hash *) sub );
        else if( sub->type == 3 ) uj_arr__delete( (uj_arr *) sub );
        else if( sub->type == 2 ) uj_str__delete( (uj_str *) sub );
        else free( sub );
    }
    xjr_key_arr__delete( keys );
    string_tree__delete( self->tree );
    free( self );
}

void uj_arr__delete( uj_arr *arr ) {
    uj_node *cur = arr->head;
    
    for( unsigned i=0;i<arr->count;i++ ) {
        uj_node *next = cur->parent;
        if( cur->type == 1 ) uj_hash__delete( (uj_hash *) cur );
        else if( cur->type == 3 ) uj_arr__delete( (uj_arr *) cur );
        else if( cur->type == 2 ) uj_str__delete( (uj_str *) cur );
        else free( cur );
        
        cur = next;
    }
}

void uj_str__delete( uj_str *node ) {
    if( node->alloc ) free( (void *) node->str );
    free( node );
}

void uj_node__dump_to_makefile( uj_node *self, char *prefix );
void uj_hash__dump_to_makefile( uj_hash *self, char *prefix ) {
    xjr_key_arr *keys = string_tree__getkeys( self->tree );
    char pref2[ 100 ];
    for( unsigned i=0;i<keys->count;i++ ) {
        const char *key = keys->items[i];
        NODE_STR_LEN_TYPE len = keys->sizes[i];
        uj_node *val = uj_hash__get( self, key, len );
        if( val->type != 1 ) printf("%s%.*s := ",prefix?prefix:"",len,key);
        else {
            if( prefix ) sprintf( pref2, "%s%.*s_", prefix, len, key );
            else sprintf( pref2, "%.*s", len, key );
            prefix = pref2;
        }
        uj_node__dump_to_makefile( val, prefix );
    }
}

void uj_arr__dump( uj_arr *self, unsigned depth ) {
    printf("[\n");
    uj_node *cur = self->head;
    for( unsigned i=0;i<self->count;i++ ) {
        SPACES uj_node__dump( cur, depth+1 );
        cur = cur->parent; // parent is being reused as next
    }
    depth--;
    SPACES printf("]\n");
}

sds uj_arr__json( uj_arr *self, unsigned depth, sds str ) {
    str = sdscat( str, "[\n" );
    uj_node *cur = self->head;
    for( unsigned i=0;i<self->count;i++ ) {
        str = add_indent( str, depth );
        str = uj_node__json( cur, depth, str );
        cur = cur->parent; // parent is being reused as next
    }
    //depth--;
    str = add_indent( str, depth-1 );
    str = sdscat( str, "]\n" );
    return str;
}

/*
1 = node hash
2 = double quoted string
3 = array
4 = positive int
5 = negative int
6 = true
7 = false
8 = null
*/
void uj_node__dump( uj_node *self, unsigned depth ) {
    if( self->type == 1 ) uj_hash__dump( (uj_hash *) self, depth+1 );
    else if( self->type == 2 ) printf("\"%.*s\"\n", ( (uj_str *) self )->len, ( (uj_str *) self )->str );
    else if( self->type == 3 ) uj_arr__dump( (uj_arr *) self, depth+1 );
    else if( self->type == 4 ) printf("%.*s\n", ( (uj_str *) self )->len, ( (uj_str *) self )->str );
    else if( self->type == 5 ) printf("-%.*s\n", ( (uj_str *) self )->len, ( (uj_str *) self )->str );  
    else if( self->type == 6 ) printf("true\n");
    else if( self->type == 7 ) printf("false\n");
    else if( self->type == 8 ) printf("null\n");
}

sds uj_node__str( uj_node *self ) {
    sds str = sdsempty();
    int depth = 0;
    
    if( self->type == 1 ) return uj_hash__str( (uj_hash *) self, depth+1, str );
    else if( self->type == 2 ) {
        return sdscatlen( str, ( (uj_str *) self )->str, ( (uj_str *) self )->len );
    }
    else if( self->type == 3 ) return uj_arr__json( (uj_arr *) self, depth+1, str );
    else if( self->type == 4 ) {
        return sdscatlen( str, ( (uj_str *) self )->str, ( (uj_str *) self )->len );
    }
    else if( self->type == 5 ) {
        str = sdscat( str, "-" );
        return sdscatlen( str, ( (uj_str *) self )->str, ( (uj_str *) self )->len );
    }
    else if( self->type == 6 ) return sdscat( str, "true");
    else if( self->type == 7 ) return sdscat( str, "false");
    else if( self->type == 8 ) return sdscat( str, "null");
    return str;
}

sds uj_node__json( uj_node *self, unsigned depth, sds str ) {
    if( !str ) {
        depth = 0;
        str = sdsempty();
    }
    if( self->type == 1 ) return uj_hash__str( (uj_hash *) self, depth+1, str );
    else if( self->type == 2 ) {
        str = sdscatrepr( str, ( (uj_str *) self )->str, ( (uj_str *) self )->len );
        return sdscat( str, "\n" );
    }
    else if( self->type == 3 ) return uj_arr__json( (uj_arr *) self, depth+1, str );
    else if( self->type == 4 ) {
        str = sdscatlen( str, ( (uj_str *) self )->str, ( (uj_str *) self )->len );
        return sdscat( str, "\n" );
    }
    else if( self->type == 5 ) {
        str = sdscat( str, "-" );
        str = sdscatlen( str, ( (uj_str *) self )->str, ( (uj_str *) self )->len );
        return sdscat( str, "\n" );
    }
    else if( self->type == 6 ) return sdscat( str, "true\n");
    else if( self->type == 7 ) return sdscat( str, "false\n");
    else if( self->type == 8 ) return sdscat( str, "null\n");
    
    str = sdscat( str, "!error!" );
    return str;
}

void uj_node__dump_to_makefile( uj_node *self, char *prefix ) {
    if( self->type == 1 ) uj_hash__dump_to_makefile( (uj_hash *) self, prefix );
    else {
        printf("\"");
        if( self->type == 2 ) printf("%.*s", ( (uj_str *) self )->len, ( (uj_str *) self )->str );
        //else if( self->type == 3 ) node_arr__dump_to_makefile( (node_arr *) self, 0 );
        else if( self->type == 4 ) printf("%.*s", ( (uj_str *) self )->len, ( (uj_str *) self )->str );
        else if( self->type == 5 ) printf("-%.*s", ( (uj_str *) self )->len, ( (uj_str *) self )->str );  
        else if( self->type == 6 ) printf("true");
        else if( self->type == 7 ) printf("false");
        else if( self->type == 8 ) printf("false");
        printf("\"\n");
    }
}

void uj_node__dump_env( uj_node *self ) {
    printf("\"");
    if( self->type == 2 ) printf("%.*s", ( (uj_str *) self )->len, ( (uj_str *) self )->str );
    else if( self->type == 4 ) printf("%.*s", ( (uj_str *) self )->len, ( (uj_str *) self )->str );
    else if( self->type == 5 ) printf("-%.*s", ( (uj_str *) self )->len, ( (uj_str *) self )->str );  
    else if( self->type == 6 ) printf("true");
    else if( self->type == 7 ) printf("false");
    else if( self->type == 8 ) printf("null");
    printf("\"");
}

void uj_hash__store( uj_hash *self, const char *key, unsigned keyLen, uj_node *node ) {
    string_tree__store_len( self->tree, key, keyLen, (void *) node, 0 );
}

void uj_hash__remove( uj_hash *self, const char *key, unsigned keylen ) {
    string_tree__delkey_len( self->tree, key, keylen );
}

uj_node *uj_hash__get( uj_hash *self, const char *key, unsigned keyLen ) {
    char type;
    return (uj_node *) string_tree__get_len( self->tree, key, keyLen, &type );
}

sds uj_hash__get_str( uj_hash *self, const char *key, unsigned keyLen ) {
    char type;
    uj_node *node = string_tree__get_len( self->tree, key, keyLen, &type );
    if( !node ) return 0;
    return uj_node__str( node );
}

char nullStr[2] = { 0, 0 };

typedef struct handle_res_s {
    int dest;
    uj_node *node;
} handle_res;

handle_res *handle_res__new(void) {
    handle_res *self = ( handle_res * ) calloc( sizeof( handle_res ), 1 );
    return self;
}

handle_res *handle_true( const char *data, unsigned long *pos ) {
    uj_node *node = uj_node__new( 6 );
    handle_res *res = handle_res__new();
    res->node = node;
    return res;
}

handle_res *handle_false( const char *data, unsigned long *pos ) {
    uj_node *node = uj_node__new( 7 );
    handle_res *res = handle_res__new();
    res->node = node;
    return res;
}

handle_res *handle_null( const char *data, unsigned long  *pos ) {
    uj_node *node = uj_node__new( 8 );
    handle_res *res = handle_res__new();
    res->node = node;
    return res;
}

uj_node *uj_null__new(void) {
    return uj_node__new( 8 );
}

uj_node *uj_true__new(void) {
    return uj_node__new( 6 );
}

uj_node *uj_false__new(void) {
    return uj_node__new( 7 );
}

uj_node *uj_bool__new( uint8_t val ) {
    return uj_node__new( val ? 6 : 7 );
}

typedef handle_res* (*ahandler)(const char *, unsigned long * ); 

string_tree *handlers;
void uj_init(void) {
    handlers = string_tree__new();
    string_tree__store_len( handlers, "true",  4, (void *) &handle_true,  0 );
    string_tree__store_len( handlers, "false", 5, (void *) &handle_false, 0 );
    string_tree__store_len( handlers, "null",  4, (void *) &handle_null,  0 );
}

uj_hash *uj_parse_file( const char *filename, int *err ) {
    unsigned long len;
    char *data = slurp_file( filename, &len );
    if( len < 0 ) {
        *err = -1;
        return NULL;
    }
    return uj_parse( data, len, NULL, err );
}

uj_hash *uj_parse( const char *data, unsigned long len, uj_state *beginState, int *err ) {
    unsigned long pos = 1, typeStart;
    unsigned keyLen;
    int endstate = 0;
    uint8_t neg = 0;
    const char *keyStart, *strStart;
    char let;
    int slashCnt = 0;// \ count in a string
    int errIgnore;
    if( !err ) err = &errIgnore;
    char stringType = 0;
    unsigned arrayStackPos = 0;
    uj_node *arrayParentStack[10];
    
    uj_hash *root = uj_hash__new();
    uj_node *cur = ( uj_node * ) root;
    if( beginState ) {
        // If we start in the middle of a key or value, we must merge the previous value
        // This means we cannot start in the normal state; we must start in one capable of handling
        // the merge. The merge state needs to be able to handle that situation repeatedly also
        // in order to handle long values.
        switch( beginState->state ) {
            case 1: goto HashComment; // contents of a comment are discard
            case 2: goto HashComment2; // does matter as ending could be split between * and /
            case 3: goto QQKeyName1; 
            case 4: goto QQKeyNameX; // double quoted key
            case 5: goto QKeyName1; 
            case 6: goto QKeyNameX; // single quoted key
            case 7: goto KeyName1; 
            case 8: goto KeyNameX; // unquoted key
            case 9: goto Colon;
            case 10: goto AfterColon;
            case 11: goto TypeX; // capture of a named type ( true/false included )
            //case 12: goto Arr;
            case 13: goto AC_Comment; // comments after : and before value
            case 14: goto AC_Comment2;
            case 15: goto Number1;
            case 16: goto NumberX;
            case 17: goto String1;
            case 18: goto StringX;
            case 19: goto AfterVal;
        }
    }
    
Hash: SAFE(0)
    let = data[pos++];
    if( let == '"' ) goto QQKeyName1;
    if( let == '\'' ) goto QKeyName1;
    if( let >= 'a' && let <= 'z' ) { pos--; goto KeyName1; }
    if( let == '}' && cur->parent ) {
        cur = cur->parent;
        if( cur->type == 3 ) goto AfterColon;
    }
    if( let == '/' && pos < (len-1) ) {
        if( data[pos] == '/' ) { pos++; goto HashComment; }
        if( data[pos] == '*' ) { pos++; goto HashComment2; }
    }
    goto Hash;
HashComment: SAFEGET(1)
    if( let == 0x0d || let == 0x0a ) goto Hash;
    goto HashComment;
HashComment2: SAFEGET(2)
    if( let == '*' && pos < (len-1) && data[pos] == '/' ) { pos++; goto Hash; }
    goto HashComment2;
QQKeyName1: SAFE(3)
    let = data[pos];
    keyStart = &data[pos++];
    if( let == '\\' ) pos++;
QQKeyNameX: SAFEGET(4)
    if( let == '\\' ) { pos++; goto QQKeyNameX; }
    if( let == '"' ) {
        keyLen = &data[pos-1] - keyStart;
        goto Colon;
    }
    goto QQKeyNameX;
QKeyName1: SAFE(5)
    let = data[pos];
    keyStart = &data[pos++];
    if( let == '\\' ) pos++;
QKeyNameX: SAFEGET(6)
    if( let == '\\' ) { pos++; goto QKeyNameX; }
    if( let == '\'' ) {
        keyLen = &data[pos-1] - keyStart;
        goto Colon;
    }
    goto QKeyNameX;
KeyName1: SAFE(7)
    keyStart = &data[pos++];
KeyNameX: SAFEGET(8)
    if( let == ':' ) {
        keyLen = &data[pos-1] - keyStart;
        #ifdef UJDEBUG
        printf("key %.*s\n", keyLen, keyStart );
        #endif
        goto AfterColon;
    }
    if( let == ' ' || let == '\t' ) {
        keyLen = &data[pos-1] - keyStart;
        goto Colon;
    }
    goto KeyNameX;
Colon: SAFEGET(9)
    if( let == ':' ) goto AfterColon;
    goto Colon;
AfterColon: SAFEGET(10)
    if( let == '"' ) {
        stringType = 0;
        goto String1;
    }
    if( let == '\'' ) {
        stringType = 1;
        goto String1;
    }
    if( let == '{' ) {
        uj_hash *newHash = uj_hash__new();
        newHash->parent = cur;
        if( cur->type == 1 ) uj_hash__store( (uj_hash *) cur, keyStart, keyLen, (uj_node *) newHash );
        if( cur->type == 3 ) uj_arr__add( (uj_arr *) cur, (uj_node *) newHash );
        cur = (uj_node *) newHash;
        goto Hash;
    }
    if( let >= 'a' && let <= 'z' ) {
        typeStart = pos - 1;
        goto TypeX;
    }
    /*if( let >= 'A' && let <= 'Z' ) {
        data[pos-1] = let - 'A' + 'a';
        typeStart = pos - 1;
        goto TypeX;
    }*/
    
    if( let == '/' && pos < (len-1) ) {
        if( data[pos] == '/' ) { pos++; goto AC_Comment; }
        if( data[pos] == '*' ) { pos++; goto AC_Comment2; }
    }
    // if( let == 't' || let == 'f' ) ... for true/false
    if( let >= '0' && let <= '9' ) { neg=0; goto Number1; }
    if( let == '-' ) { neg=1; pos++; goto Number1; }
    if( let == '[' ) {
        #ifdef UJDEBUG
        printf("Entering array\n" );
        #endif
        uj_arr *newArr = uj_arr__new();
        //newArr->parent = cur;
        arrayParentStack[ arrayStackPos++ ] = cur;
        if( cur->type == 1 ) uj_hash__store( (uj_hash *) cur, keyStart, keyLen, (uj_node *) newArr );
        if( cur->type == 3 ) uj_arr__add( (uj_arr *) cur, (uj_node *) newArr );
        cur = (uj_node *) newArr;
        goto AfterColon;
    }
    if( let == ']' ) {
        #ifdef UJDEBUG
        printf("Exiting array\n" );
        #endif
        //cur = cur->parent;
        cur = arrayParentStack[ --arrayStackPos ];
        if( cur->type == 3 ) goto AfterColon;
        if( cur->type == 1 ) goto Hash;
        // should never reach here
    }
    goto AfterColon;
TypeX: SAFEGET(11)
    if( ( let >= '0' && let <= 9 ) || ( let >= 'a' && let <= 'z' ) ) goto TypeX;
    /*if( let >= 'A' && let <= 'Z' ) {
        data[pos-1] = let - 'A' + 'a';
        goto TypeX;
    }*/
    pos--; // go back a character; we've encountered a non-type character
    // Type name is done
    unsigned typeLen = (unsigned) pos - typeStart; 
    // do something with the type
    char htype;
    ahandler handler = (ahandler) string_tree__get_len( handlers, &data[ typeStart ], typeLen, &htype );
    if( handler == NULL ) {
        printf("disaster; no handler for %.*s", typeLen, &data[typeStart] );
        exit(1);
    }
    handle_res *res = (*handler)( data, &pos );
    if( res == NULL ) {
        printf("disaster");
        exit(1);
    }
    if( res->dest == 0 ) {
        if( cur->type == 1 ) {
            uj_hash__store( (uj_hash *) cur, keyStart, keyLen, res->node );
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            uj_arr__add( (uj_arr *) cur, res->node );
            goto AfterColon;
        }
    }
    goto TypeX; // should never reach here
/*AfterType: SAFEGET
    if( let == '.' ) goto GotDot;
    // skip whitespace till .
    if( let == ' ' || let == 0x0d || let == 0x0a || let == '\t' ) goto AfterType;
    // something else. garbage. :(
    goto AfterType;*/
AC_Comment: SAFEGET(13)
    if( let == 0x0d || let == 0x0a ) goto AfterColon;
    goto AC_Comment;
AC_Comment2: SAFEGET(14)
    if( let == '*' && pos < (len-1) && data[pos] == '/' ) { pos++; goto Hash; }
    goto AC_Comment2;
Number1: SAFE(15)
    strStart = &data[pos-1];
NumberX: SAFEGET(16)
    if( let == '.' ) goto AfterDot;
    if( let < '0' || let > '9' ) {
        NODE_STR_LEN_TYPE strLen = (NODE_STR_LEN_TYPE) (&data[pos-1] - strStart);
        uj_node *newStr = (uj_node *) uj_str__new( strStart, strLen, neg ? 5 : 4 );
        if( cur->type == 1 ) { // hash, expecting key
            uj_hash__store( (uj_hash *) cur, keyStart, keyLen, newStr );
            pos--; // Don't eat this character
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            #ifdef UJDEBUG
            printf("Finished number, waiting for value\n" );
            #endif
            uj_arr__add( (uj_arr *) cur, newStr );
            pos--; // Don't eat this character
            goto AfterColon;
        }
    }
    goto NumberX;
AfterDot: SAFEGET(20)
    if( let < '0' || let > '9' ) {
        NODE_STR_LEN_TYPE strLen = (NODE_STR_LEN_TYPE) ( &data[pos-1] - strStart );
        uj_node *newStr = (uj_node *) uj_str__new( strStart, strLen, neg ? 5 : 4 );
        if( cur->type == 1 ) {
            uj_hash__store( (uj_hash *) cur, keyStart, keyLen, newStr );
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            uj_arr__add( (uj_arr *) cur, newStr );
            goto AfterColon;
        }
    }
    goto AfterDot;
String1: SAFEGET(17)
    slashCnt = 0;
    if( let == '"' ) {
        uj_node *newStr = (uj_node *) uj_str__new( nullStr, 0, 2 );
        if( cur->type == 1 ) {
            uj_hash__store( (uj_hash *) cur, keyStart, keyLen, newStr );
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            uj_arr__add( (uj_arr *) cur, newStr );
            goto AfterColon;
        }
        goto AfterVal; // Should never be reached
    }
    if( let == '\'' ) {
        uj_node *newStr = (uj_node *) uj_str__new( nullStr, 0, 2 );
        if( cur->type == 1 ) {
            uj_hash__store( (uj_hash *) cur, keyStart, keyLen, newStr );
            goto AfterVal;
        }
        if( cur->type == 3 ) {
            uj_arr__add( (uj_arr *) cur, newStr );
            goto AfterColon;
        }
        goto AfterVal; // Should never be reached
    }
    strStart = &data[pos-1];
    goto StringX;
StringX: SAFEGET(18)
    StringX2:
    if( ( !stringType && let == '"' ) || ( stringType && let == '\'' ) ) {
       NODE_STR_LEN_TYPE strLen = (NODE_STR_LEN_TYPE) (&data[pos-1] - strStart);
       uj_node *newStr;
       if( slashCnt ) newStr = (uj_node *) uj_str__new_from_json( strStart, strLen );
       else newStr = (uj_node *) uj_str__new( strStart, strLen, 2 );
         
       if( cur->type == 1 ) {
           uj_hash__store( (uj_hash *) cur, keyStart, keyLen, newStr );
           goto AfterVal;
       }
       if( cur->type == 3 ) {
           uj_arr__add( (uj_arr *) cur, newStr );
           goto AfterColon;
       }
       goto AfterVal; // should never be reached
    }
    if( let == '\\' ) {
       slashCnt++;
       goto StrSlash1;
    }
    goto StringX;
StrSlash1: SAFEGET(21) // hex or "
    if( ( !stringType && let == '"' ) || (stringType && let == '\'' ) || let == '\\' ) {
        goto StringX;
    }
    if( let == 'u' ) {
        //uCnt++;
        goto SlashHex1;
    }
    goto StringX2;
SlashHex1: SAFEGET(22)
    if( ( let >= '0' && let <= '9' ) || ( let >= 'A' && let <= 'F' ) || ( let >= 'a' && let <= 'f' ) ) {
        goto SlashHex2;
    }
    goto StringX2;
SlashHex2: SAFEGET(22)
    if( ( let >= '0' && let <= '9' ) || ( let >= 'A' && let <= 'F' ) || ( let >= 'a' && let <= 'f' ) ) {
        goto SlashHex3;
    }
    goto StringX2;
SlashHex3: SAFEGET(22)
    if( ( let >= '0' && let <= '9' ) || ( let >= 'A' && let <= 'F' ) || ( let >= 'a' && let <= 'f' ) ) {
        goto SlashHex4;
    }
    goto StringX2;
SlashHex4: SAFEGET(22)
    goto StringX2;
AfterVal: SAFE(19)
    // who cares about commas in between things; we can just ignore them :D
    goto Hash;
Done:
    return root;
}