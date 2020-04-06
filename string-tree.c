// Copyright (C) 2018 David Helkowski

#include "xjr-string-tree.h"
#include<string.h>
#include"red_black_tree.h"

uint32_t fnv1a( char *str ) {
	uint32_t hval = 0;
    unsigned char *s = (unsigned char *) str;

    while (*s) {
    	hval ^= (uint32_t)*s++;
		hval *= ((uint32_t)0x01000193);
		//hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	//printf("Hash '%s' to %u\n", str, hval );
    return hval;
}

uint32_t fnv1a_len( char *str, int strlen ) {
	uint32_t hval = 0;
    unsigned char *s = (unsigned char *) str;

    for( int i=0;i<strlen;i++ ) {
    //while (*s) {
    	hval ^= (uint32_t)*s++;
		hval *= ((uint32_t)0x01000193);
		//hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
	}
	//printf("Hash '%.*s' to %u\n", strlen, str, hval );
    return hval;
}

void string_tree__delkey( string_tree *self, char *key ) {
	uint32_t hash = fnv1a( key );
	rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
	
	// We unfortunately cannot just delete the rbnode; as multiple keys may hash to this same value
	//if( rbnode ) RBDelete( (rb_red_blk_tree *) self->tree, rbnode ); // automatically deletes info nodes
	int keylen = strlen( key );
	
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
		    free( node );// destroy the snode
		}
		else {
            if( prev ) prev->next = node;
            else rbnode->info = node;
            prev = node;
        }
		node = next;
	}
	prev->next = NULL;
}

void string_tree__delkey_len( string_tree *self, char *key, int keylen ) {
	uint32_t hash = fnv1a_len( key, keylen );
	rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
	
	// We unfortunately cannot just delete the rbnode; as multiple keys may hash to this same value
	//if( rbnode ) RBDelete( (rb_red_blk_tree *) self->tree, rbnode ); // automatically deletes info nodes
	
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
		    free( node );// destroy the snode
		}
		else {
            if( prev ) prev->next = node;
            else rbnode->info = node;
            prev = node;
        }
		node = next;
	}
	if( prev ) {
	    prev->next = NULL;
	}
	else {
	    rbnode->info = NULL;
	}
}

string_tree *string_tree__new() {
    string_tree *self = ( string_tree * ) malloc( sizeof( string_tree ) );
	self->tree = (void *) RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint);
	return self;
}

void string_tree__delete( string_tree *self ) {
	RBTreeDestroy( (rb_red_blk_tree *) self->tree );
	free( self );
}

void *string_tree__get( string_tree *self, char *key, char *dataType ) {
	//printf("Getting %s\n", key );
	snode *node = string_tree__rawget( self, key );
	if( !node ) {
		//printf("Could not find node %s\n", key );
		return 0;
	}
	*dataType = node->dataType;
	return node->data;
}

void *string_tree__get_len( string_tree *self, char *key, int keylen, char *dataType ) {
	//printf("Getting %s\n", key );
	snode *node = string_tree__rawget_len( self, key, keylen );
	if( !node ) {
		//printf("Could not find node %s\n", key );
		return 0;
	}
	*dataType = node->dataType;
	return node->data;
}

xjr_arr *string_tree__getarr( string_tree *self, char *key ) {
	snode *node = string_tree__rawget( self, key );
	if( !node ) return 0;
	xjr_arr *arr = xjr_arr__new();
	arr->count = 1;
	arr->items[ 0 ] = node->data;
	arr->types[ 0 ] = node->dataType;
	node = node->next;
	
	while( node ) {
		if( node->strlen ) {
			int keylen = strlen( key );
			if( keylen == node->strlen && !strncmp( node->str, key, node->strlen ) ) {
			    arr->types[ arr->count ] = node->dataType;
			    arr->items[ arr->count++ ] = node->data;
			    if( arr->count >= arr->max ) xjr_arr__double( arr );
			}
		}
		else {
			if( !strcmp( node->str, key ) ) {
			    arr->types[ arr->count ] = node->dataType;
			    arr->items[ arr->count++ ] = node->data;
			    if( arr->count >= arr->max ) xjr_arr__double( arr );
			}		    
		}
		node = node->next;
	}
	
	return arr;
}

xjr_arr *string_tree__getarr_len( string_tree *self, char *key, int keylen ) {
	snode *node = string_tree__rawget_len( self, key, keylen );
	if( !node ) return 0;
	xjr_arr *arr = xjr_arr__new();
	arr->count = 1;
	arr->items[ 0 ] = node->data;
	arr->types[ 0 ] = node->dataType;
	node = node->next;
	
	while( node ) {
		if( node->strlen ) {
			int keylen = strlen( key );
			if( keylen == node->strlen && !strncmp( node->str, key, node->strlen ) ) {
			    arr->types[ arr->count   ] = node->dataType;
				arr->items[ arr->count++ ] = node->data;
				if( arr->count >= arr->max ) xjr_arr__double( arr );
		    }
		}
		else {
			if( !strcmp( node->str, key ) ) {
			    arr->types[ arr->count   ] = node->dataType;
			    arr->items[ arr->count++ ] = node->data;
			    if( arr->count >= arr->max ) xjr_arr__double( arr );
			}
		}
		node = node->next;
	}
	
	return arr;
}

/*
snode *string_tree_c::rawget( char *key, uint32_t hash ) {
	snode *node = (snode *) RBExactQuery( tree, &hash );
	while( node ) {
		if( !strcmp( node->str, key ) ) return node;
		node = node->next;
	}
	return 0;
}*/

snode *string_tree__rawget( string_tree *self, char *key ) {
	//printf("Attempting to get node %s\n", key );
	uint32_t hash = fnv1a( key );
	rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
	if( !rbnode ) { return 0; }
	//printf("Found rbnode\n");
	snode *node = (snode *) rbnode->info;
	//printf("got %i\n", (int) node );
	int keylen = strlen( key );
	while( node ) {
		if( node->strlen ) {
			//printf("checking %.*s\n", node->strlen, node->str );
			if( keylen == node->strlen && !strncmp( node->str, key, node->strlen ) ) return node;
		}
		else {
			//printf("checking %s\n", node->str );
			if( !strcmp( node->str, key ) ) return node;
		}
		node = node->next;
	}
	
	//printf("ret\n");
	return NULL;
}

snode *string_tree__rawget_len( string_tree *self, char *key, int keylen ) {
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

void string_tree__rawput( string_tree *self, char *key, int keylen, snode *orig, snode *newnode ) {
	uint32_t hash = fnv1a_len( key, keylen );
	rb_red_blk_node* rbnode = RBExactQuery( (rb_red_blk_tree *) self->tree, &hash );
	if( !rbnode ) { return; } // this function should only be used to replace
	if( rbnode->info == orig ) {
		rbnode->info = newnode;
		return;
	}
	snode *node = (snode *) rbnode->info;
	while( node ) {
		if( node->next == orig ) {
			node->next = newnode;
			return;
		}
		node = node->next;
	}
}

void string_tree__store( string_tree *self, char *key, void *node, char dataType ) {
	uint32_t hash = fnv1a( key );
	snode *curnode = string_tree__rawget( self, key );
	//printf("Storing %s\n", key );
	if( curnode ) {
		while( curnode->next ) curnode = curnode->next;
		
		snode *newnode = snode__new( key, node, dataType, NULL );
		curnode->next = newnode;
	}
	else {
		//printf("No curnode\n");
		curnode = snode__new( key, node, dataType, NULL );
		//printf("New node ptr: %i\n", (int) curnode );
		uint32_t *hdup = malloc( sizeof( uint32_t ) );
		*hdup = hash;
		RBTreeInsert( (rb_red_blk_tree *) self->tree, hdup, curnode );
	}
}


void string_tree__store_len( string_tree *self, char *key, int keylen, void *node, char dataType ) {
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

void IntDest(void* a) {
	//printf("k");
	free((uint32_t*)a);
}
int IntComp(const void* a,const void* b) {
	//printf("compare %u to %u\n", *(uint32_t*)a, *(uint32_t*)b);
  if( *(uint32_t*)a > *(uint32_t*)b) return(1);
  if( *(uint32_t*)a < *(uint32_t*)b) return(-1);
  return(0);
}
void IntPrint(const void* a) {
    //printf("%li",*(uint32_t*)a);     
    printf("%li",*(long int*)a);
}
void InfoPrint(void* a) {
	snode *node = (snode *)a;
	printf("%.*s",node->strlen,node->str);
}
void InfoDest(void *a){
	//printf("t");
	//snode *n = (snode *) a;
	//printf("destroy %s\n", n->str );
	free( a );
}

snode *snode__new( char *newstr, void *newdata, char dataType, snode *newnext ) {
    snode *self = ( snode * ) malloc( sizeof( snode ) );
	self->next = newnext;
	self->str = newstr;
	self->data = newdata;
	self->dataType = dataType;
	self->strlen = 0;
	//printf("New snodec - next=%i str=%s data=%i\n", (int)next, str, (int)data );
	return self;
}

void snode__delete( snode *self ) {
    snode *curnode = self;
    while( curnode ) {
        snode *nextnode = curnode->next;
        free( curnode );
        curnode = nextnode;
    }
}

snode *snode__new_len( char *newstr, int nstrlen, void *newdata, char dataType, snode *newnext ) {
    snode *self = ( snode * ) malloc( sizeof( snode ) );
	self->next = newnext;
	self->str = newstr;
	self->strlen = nstrlen;
	self->data = newdata;
	self->dataType = dataType;
	//printf("New snodec - next=%i str=%s data=%i\n", (int)next, str, (int)data );
	return self;
}

void string_tree__dump( string_tree *self ) {
	RBTreePrint( (rb_red_blk_tree *) self->tree );
}

xjr_arr *xjr_arr__new() {
    xjr_arr *arr = ( xjr_arr * ) calloc( sizeof( xjr_arr ), 1 ); // calloc to ensure initial count is 0
    arr->items = malloc( sizeof( void * ) * XJR_ARR_MAX );
    arr->max = XJR_ARR_MAX;
    return arr;
}

void xjr_arr__double( xjr_arr *self) {
    void **olditems = self->items;
    int max = self->max * 2;
    self->items = malloc( sizeof( void * ) * max );
    memcpy( self->items, olditems, sizeof( void * ) * self->max );
    free( olditems );
    self->max = max;
}

void xjr_arr__delete( xjr_arr *self ) {
    free( self->items );
    free( self );
}

xjr_key_arr *xjr_key_arr__new() {
    xjr_key_arr *arr = ( xjr_key_arr * ) calloc( sizeof( xjr_key_arr ), 1 ); // calloc to ensure initial count is 0
    arr->items = malloc( sizeof( void * ) * XJR_KEY_ARR_MAX );
    arr->sizes = malloc( sizeof( int ) * XJR_KEY_ARR_MAX );
    arr->max = XJR_KEY_ARR_MAX;
    return arr;
}

void xjr_key_arr__double( xjr_key_arr *self) {
    char **olditems = self->items;
    void *oldsizes = self->sizes;
    int max = self->max * 2;
    self->items = malloc( sizeof( char * ) * max );
    self->sizes = malloc( sizeof( int ) * max );
    memcpy( self->items, olditems, sizeof( char * ) * self->max );
    memcpy( self->sizes, oldsizes, sizeof( int ) * self->max );
    free( olditems );
    free( oldsizes );
    self->max = max;
}

void xjr_key_arr__delete( xjr_key_arr *self ) {
    free( self->items );
    free( self->sizes );
    free( self );
}

xjr_key_arr *string_tree__getkeys( string_tree *self ) {
    xjr_key_arr *arr = xjr_key_arr__new();
    TreeForEach1p( self->tree, string_tree__getkeys_rec, arr, NULL );
    return arr;
}
void string_tree__getkeys_rec( void *snodeV, void *arrV ) {
    snode *snodex = ( snode * ) snodeV;
    xjr_key_arr *arr = ( xjr_key_arr * ) arrV;
    arr->sizes[ arr->count ] = snodex->strlen;
    arr->items[ arr->count++ ] = snodex->str;
    if( arr->count >= arr->max ) xjr_key_arr__double( arr );
}
