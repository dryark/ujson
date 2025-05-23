// Copyright (C) 2018 David Helkowski

#include "string-tree.h"
#include <string.h>
//#include<stdio.h>
#include "red_black_tree.h"

void KeycacheDestroyNode(void *a);

typedef struct keycache_node_s {
    const char *str;
    unsigned strlen;
    struct keycache_node_s *next;
} keycache_node;

uint32_t fnv1a_len( const char *str, unsigned strlen ) {
    uint32_t hval = 0;
    unsigned char *s = (unsigned char *) str;

    for( unsigned i=0;i<strlen;i++ ) {
        //while (*s) {
        hval ^= (uint32_t)*s++;
        hval *= ((uint32_t)0x01000193);
        //hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
    }
    //printf("Hash '%.*s' to %u\n", strlen, str, hval );
    return hval;
}

void string_tree__delkey_len( string_tree *self, const char *key, unsigned keylen ) {
    uint32_t hash = fnv1a_len( key, keylen );
    rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
    
    // We unfortunately cannot just delete the rbnode; as multiple keys may hash to this same value
    
    if( !rbnode ) { return; }
    snode *node = (snode *) rbnode->info;
    snode *prev = NULL;
    while( node ) {
        snode *next = node->next;
        int delete = 0;
        if( node->strlen ) {
            if( keylen == node->strlen && !strncmp( node->str, key, node->strlen ) ) delete = 1;
        }
        else {
            int nslen = strlen( node->str );
            if( nslen == keylen && !strncmp( node->str, key, keylen ) ) delete = 1;
        }
        if( delete ) {
            snode__delete( node );
            if( prev ) {
                prev->next = next;
            }
            else {
                rbnode->info = next;
            }
            break;
        }
        node = next;
    }
    
    if( !rbnode->info ) {
        RBDelete( (rb_red_blk_tree *) self->tree, rbnode );
    }
}

string_tree *string_tree__new(void) {
    string_tree *self = ( string_tree * ) malloc( sizeof( string_tree ) );
    self->tree = (void *) RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint);
    return self;
}

string_tree *keycache = NULL;
void keycache__new(void) {
    if( keycache ) return;
    //printf("Creating keycache\n");
    keycache = ( string_tree * ) malloc( sizeof( string_tree ) );
    keycache->tree = (void *) RBTreeCreate(IntComp,IntDest,KeycacheDestroyNode,IntPrint,InfoPrint);
}

void string_tree__delete( string_tree *self ) {
    RBTreeDestroy( (rb_red_blk_tree *) self->tree );
    free( self );
}

void *string_tree__get_len( string_tree *self, const char *key, unsigned keylen, char *dataType ) {
    //printf("Getting %s\n", key );
    snode *node = string_tree__rawget_len( self, key, keylen );
    if( !node ) {
        //printf("Could not find node %s\n", key );
        return 0;
    }
    *dataType = node->dataType;
    return node->data;
}

void *string_tree__get_precalc( string_tree *self, StrWithHash pre, char *dataType ) {
    snode *node = string_tree__rawget_precalc( self, pre );
    if( !node ) return 0;
    *dataType = node->dataType;
    return node->data;
}

snode *string_tree__rawget_precalc( string_tree *self, StrWithHash pre ) {
    uint32_t hash = pre.hash;
    rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
    if( !rbnode ) return 0;
    snode *node = (snode *) rbnode->info;
    while( node ) {
        if( node->strlen ) {
            if( pre.len == node->strlen && !strncmp( node->str, pre.str, pre.len ) ) return node;
        }
        else {
            int nslen = strlen( node->str );
            if( nslen == pre.len && !strncmp( node->str, pre.str, pre.len ) ) return node;
        }
        node = node->next;
    }
    return NULL;
}

snode *string_tree__rawget_len( string_tree *self, const char *key, unsigned keylen ) {
    //printf("Attempting to get node %s\n", key );
    uint32_t hash = fnv1a_len( key, keylen );
    rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
    if( !rbnode ) { return 0; }
    //printf("Found rbnode\n");
    snode *node = (snode *) rbnode->info;
    //printf("got %i\n", (int) node );
    while( node ) {
        if( node->strlen ) {
            if( keylen == node->strlen && !strncmp( node->str, key, node->strlen ) ) return node;
        }
        else {
            int nslen = strlen( node->str );
            if( nslen == keylen && !strncmp( node->str, key, keylen ) ) return node;
        }
        node = node->next;
    }
    
    //printf("ret\n");
    return NULL;
}

keycache_node *keycache__rawget_len( string_tree *self, const char *key, unsigned keylen ) {
    uint32_t hash = fnv1a_len( key, keylen );
    rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
    if( !rbnode ) { return 0; }
    keycache_node *node = (keycache_node *) rbnode->info;
    while( node ) {
        if( node->strlen ) {
            if( keylen == node->strlen && !strncmp( node->str, key, node->strlen ) ) return node;
        }
        else {
            int nslen = strlen( node->str );
            if( nslen == keylen && !strncmp( node->str, key, keylen ) ) return node;
        }
        node = node->next;
    }
    return NULL;
}

keycache_node *keycache_node__new( const char *key, unsigned keylen ) {
    keycache_node *self = ( keycache_node * ) malloc( sizeof( keycache_node ) );
    self->next = NULL;
    self->str = strdup(key);
    self->strlen = keylen;
    return self;
}

const char *keycache__store( const char *key, unsigned keylen ) {
    if( !keycache ) keycache__new();
    uint32_t hash = fnv1a_len( key, keylen );
    
    keycache_node *curnode = keycache__rawget_len( keycache, key, keylen );
    if( curnode ) {
        if( curnode->strlen == keylen && !strncmp( curnode->str, key, keylen ) ) return curnode->str;
        
        while( curnode->next ) {
            curnode = curnode->next;
            if( curnode->strlen == keylen && !strncmp( curnode->str, key, keylen ) ) return curnode->str;
        }
        
        keycache_node *newnode = keycache_node__new( key, keylen );
        curnode->next = newnode;
        return newnode->str;
    }
    else {
        curnode = keycache_node__new( key, keylen );
        uint32_t *hdup = malloc( sizeof( uint32_t ) );
        *hdup = hash;
        RBTreeInsert( (rb_red_blk_tree *) keycache->tree, hdup, curnode );
        return curnode->str;
    }
}

void string_tree__store_len( string_tree *self, const char *key, unsigned keylen, void *node, char dataType ) {
    uint32_t hash = fnv1a_len( key, keylen );
    snode *curnode = string_tree__rawget_len( self, key, keylen );
    if( curnode ) {
        //snode *next = curnode->next;
        while( curnode->next ) curnode = curnode->next;
        
        snode *newnode = snode__new_len( key, keylen, node, dataType, NULL );
        curnode->next = newnode;
    }
    else {
        curnode = snode__new_len( key, keylen, node, dataType, NULL );
        uint32_t *hdup = malloc( sizeof( uint32_t ) );
        *hdup = hash;
        RBTreeInsert( (rb_red_blk_tree *) self->tree, hdup, curnode );
    }
}

void string_tree__store_precalc( string_tree *self, StrWithHash key, void *node, char dataType ) {
    snode *curnode = string_tree__rawget_precalc( self, key );
    if( curnode ) {
        while( curnode->next ) curnode = curnode->next;
        snode *newnode = snode__new_len( key.str, key.len, node, dataType, NULL );
        curnode->next = newnode;
    }
    else {
        curnode = snode__new_len( key.str, key.len, node, dataType, NULL );
        uint32_t *hdup = malloc( sizeof( uint32_t ) );
        *hdup = key.hash;
        RBTreeInsert( (rb_red_blk_tree *) self->tree, hdup, curnode );
    }
}

void IntDest(void* a) {
    free((uint32_t*)a);
}
int IntComp(const void* a,const void* b) {
    if( *(uint32_t*)a > *(uint32_t*)b) return(1);
    if( *(uint32_t*)a < *(uint32_t*)b) return(-1);
    return(0);
}
void IntPrint(const void* a) {
}
void InfoPrint(void* a) {
}
void InfoDest(void *a){
    //free( a );
    snode__delete( a );
}
void KeycacheDestroyNode(void *a){
    //char *str = (char *) a;
    keycache_node *node = (keycache_node *) a;
    free( (char *) node->str );
    free( node );
}

void snode__delete( snode *self ) {
    snode *curnode = self;
    while( curnode ) {
        snode *nextnode = curnode->next;
        free( curnode );
        curnode = nextnode;
    }
}

snode *snode__new_len( const char *newstr, unsigned nstrlen, void *newdata, char dataType, snode *newnext ) {
    snode *self = ( snode * ) malloc( sizeof( snode ) );
    self->next = newnext;
    self->str = newstr;
    self->strlen = nstrlen;
    self->data = newdata;
    self->dataType = dataType;
    //printf("New snodec - next=%i str=%s data=%i\n", (int)next, str, (int)data );
    return self;
}

xjr_arr *xjr_arr__new(void) {
    xjr_arr *arr = ( xjr_arr * ) calloc( sizeof( xjr_arr ), 1 ); // calloc to ensure initial count is 0
    arr->items = malloc( sizeof( void * ) * XJR_ARR_MAX );
    arr->max = XJR_ARR_MAX;
    return arr;
}

void xjr_arr__double( xjr_arr *self) {
    const void **olditems = self->items;
    unsigned max = self->max * 2;
    self->items = malloc( sizeof( void * ) * max );
    memcpy( self->items, olditems, sizeof( void * ) * self->max );
    free( olditems );
    self->max = max;
}

void xjr_arr__delete( xjr_arr *self ) {
    free( self->items );
    free( self );
}

xjr_key_arr *xjr_key_arr__new(void) {
    xjr_key_arr *arr = ( xjr_key_arr * ) calloc( sizeof( xjr_key_arr ), 1 ); // calloc to ensure initial count is 0
    arr->items = malloc( sizeof( void * ) * XJR_KEY_ARR_MAX );
    arr->sizes = malloc( sizeof( unsigned ) * XJR_KEY_ARR_MAX );
    arr->max = XJR_KEY_ARR_MAX;
    return arr;
}

void xjr_key_arr__double( xjr_key_arr *self) {
    const char **olditems = self->items;
    void *oldsizes = self->sizes;
    unsigned max = self->max * 2;
    self->items = malloc( sizeof( char * ) * max );
    self->sizes = malloc( sizeof( unsigned ) * max );
    memcpy( self->items, olditems, sizeof( char * ) * self->max );
    memcpy( self->sizes, oldsizes, sizeof( unsigned ) * self->max );
    free( olditems );
    free( oldsizes );
    self->max = max;
}

void xjr_key_arr__delete( xjr_key_arr *self ) {
    free( self->items );
    free( self->sizes );
    free( self );
}

/*void keycache__delete_one( void *node, void *result ) {
    // result is NULL; don't need it
    free( node ); // it's just a char *
}*/

void keycache__delete(void) {
    if( !keycache ) return;
    //TreeForEach1p( keycache->tree, keycache__delete_one, NULL, NULL );
    RBTreeDestroy( (rb_red_blk_tree *) keycache->tree );
    free( keycache );
    keycache = NULL;
}

xjr_key_arr *string_tree__getkeys( string_tree *self ) {
    xjr_key_arr *arr = xjr_key_arr__new();
    TreeForEach1p( self->tree, string_tree__getkeys_rec, arr, NULL );
    return arr;
}

void string_tree__getkeycount_rec( void *snodeV, void *countV );

unsigned string_tree__getkeycount( string_tree *self ) {
    xjr_key_arr *arr = xjr_key_arr__new();
    unsigned count;
    TreeForEach1p( self->tree, string_tree__getkeycount_rec, &count, NULL );
    return count;
}

void string_tree__getkeycount_rec( void *snodeV, void *count ) {
    unsigned *countP = ( unsigned * ) count;
    *countP++;
}

void string_tree__getkeys_rec( void *snodeV, void *arrV ) {
    snode *snodex = ( snode * ) snodeV;
    xjr_key_arr *arr = ( xjr_key_arr * ) arrV;
    arr->sizes[ arr->count ] = snodex->strlen;
    arr->items[ arr->count++ ] = snodex->str;
    if( arr->count >= arr->max ) xjr_key_arr__double( arr );
}
