// Copyright (C) 2018 David Helkowski
// Fair Coding License 1.0+

#ifndef __STRING_TREE_H
#define __STRING_TREE_H
#include <stdint.h>

uint32_t fnv1a_len( const char *str, unsigned strlen );

struct snode_s {
	const char *str;
	unsigned strlen;
	char dataType;
	void *data;
	struct snode_s *next;
};
typedef struct snode_s snode;

snode *snode__new( const char *newstr, void *newdata, char dataType, snode *newnext );
void snode__delete( snode *self );
snode *snode__new_len( const char *newstr, unsigned strlen, void *newdata, char dataType, snode *newnext );

#define XJR_ARR_MAX 5
typedef struct xjr_arr_s xjr_arr;
struct xjr_arr_s {
	unsigned count;
	unsigned max;
	const void **items;
	char *types;
};
xjr_arr *xjr_arr__new(void);
void xjr_arr__double( xjr_arr *self );
void xjr_arr__delete( xjr_arr *self );

#define XJR_KEY_ARR_MAX 5
typedef struct xjr_key_arr_s xjr_key_arr;
struct xjr_key_arr_s {
	unsigned count;
	unsigned max;
	const char **items;
	unsigned *sizes;
};
xjr_key_arr *xjr_key_arr__new(void);
void xjr_key_arr__double( xjr_key_arr *self );
void xjr_key_arr__delete( xjr_key_arr *self );

struct string_tree_s {
	void *tree;
};
typedef struct string_tree_s string_tree;
snode *string_tree__rawget_len( string_tree *self, const char *key, unsigned keylen );
string_tree *string_tree__new(void);
void string_tree__delete( string_tree *self );
void *string_tree__get_len( string_tree *self, const char *key, unsigned keylen, char *dataType );
void string_tree__delkey_len( string_tree *self, const char *key, unsigned keylen );

void string_tree__store_len( string_tree *self, const char *key, unsigned keylen, void *node, char dataType );

void IntDest(void *); int IntComp(const void *,const void *);
void IntPrint(const void* a); void InfoPrint(void *); void InfoDest(void *);

xjr_key_arr *string_tree__getkeys( string_tree *self );
void string_tree__getkeys_rec( void *snodeV, void *arrV );

// Key Cache
const char *keycache__store( const char *key, unsigned keylen );
void keycache__delete(void);

#endif
