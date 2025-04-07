#import <Foundation/Foundation.h>

#import "ujsonObjc.h"
#include <objc/runtime.h>

@interface NSString (Ujson)
- (uj_node *)toNode;
@end

@implementation NSString (Ujson)
- (uj_node *)toNode {
    const char *strC = strdup( [self UTF8String] );
    unsigned len = (unsigned) strlen( strC );
    uj_str *node = uj_str__new( strC, len, 2 );
    node->alloc = 1; // to indicate it was allocated ( strdup above )
    return (uj_node *) node;
}
@end

@interface NSNumber (Ujson)
- (uj_node *)toNode;
@end

@implementation NSNumber (Ujson)
- (uj_node *)toNode {
    //const char *strC = strdup( [self UTF8String] );
    //unsigned len = (unsigned) strlen( strC );
    uint8_t type = 4; // positive number
    const char *numType = [self objCType];
    /*
     int       -> i
     float     -> f
     double    -> d
     BOOL      -> B
     long      -> l
     long long -> q
     */
    char *str;
    if( numType[0] == 'c' ) { // signed char
        int8_t val = [self intValue];
        if( val < 0 ) {
            type = 5;
            val = -val;
        }
        else if( val == 0 || val == 1 ) {
            if( [self isKindOfClass:objc_getClass("__NSCFBoolean")] ) {
                return uj_bool__new( val );
            }
        }
        asprintf( &str, "%d", (int) val );
    }
    if( numType[0] == 'i' ) { // int
        int val = [self intValue];
        if( val < 0 ) {
            type = 5;
            val = -val;
        }
        asprintf( &str, "%d", val );
    }
    if( numType[0] == 'f' ) { // float
        float val = [self floatValue];
        if( val < 0 ) {
            type = 5;
            val = -val;
        }
        // Unfortunately this can print 5e+10 or similar and JSON needs 5e10
        asprintf( &str, "%g", val );
    }
    if( numType[0] == 'd' ) { // double
        double val = [self doubleValue];
        if( val < 0 ) {
            type = 5;
            val = -val;
        }
        asprintf( &str, "%g", val );
    }
    //if( numType[0] == 'B' ) { // BOOL
    //    return node_bool__new( [self boolValue] );
    //}
    if( numType[0] == 'l' ) { // long int
        long long val = [self longValue];
        if( val < 0 ) {
            type = 5;
            val = -val;
        }
        asprintf( &str, "%ld", val );
    }
    if( numType[0] == 'q' ) { // long long int
        long long val = [self longLongValue];
        if( val < 0 ) {
            type = 5;
            val = -val;
        }
        asprintf( &str, "%lld", val );
    }
    unsigned len = (unsigned) strlen( str );
    uj_str *node = uj_str__new( str, len, type );
    node->alloc = 1; // to indicate it was allocated ( strdup above )
    return (uj_node *) node;
}
@end

@interface NSDictionary (Ujson)
- (uj_node *)toNode;
@end

@implementation NSDictionary (Ujson)
- (uj_node *)toNode {
    uj_hash *hash = uj_hash__new();
    for( id key in self ) {
        NSString *keyns = (NSString *) key;
        id value = self[keyns];
        const char *keyc = [keyns UTF8String];
        unsigned keylen = (unsigned) strlen( keyc );
        const char *cachedKey = keycache__store( keyc, keylen );
        uj_hash__store( hash, cachedKey, keylen, [value toNode] );
    }
    return (uj_node *) hash;
}
@end

@interface NSArray (Ujson)
- (uj_node *)toNode;
@end

@implementation NSArray (Ujson)
- (uj_node *)toNode {
    uj_arr *arr = uj_arr__new();
    for( id value in self ) {
        uj_arr__add( arr, [value toNode] );
    }
    return (uj_node *) arr;
}
@end

@implementation NodeHashEnumerator

- (instancetype)initWithNode:(uj_hash *)hash {
    self = [super init];
    _ujhash = hash;
    _keys = string_tree__getkeys( _ujhash->tree );
    _i = 0;
    return self;
}

- (id)nextObject {
    if( _i >= _keys->count ) return nil;
    const char *key = _keys->items[_i];
    int len = _keys->sizes[_i++];
    return [NSString stringWithFormat:@"%.*s",len,key];
}

@end

@implementation UJson

+ (id)node2ns:(uj_node *)node {
    // need to check the jnode type to determine what to return
    // if it is a hash, wrap it in a new NodeHash
    int type = node->base.type;
    if( type == 1 ) return [[UJHash alloc] initWithNode:(uj_hash *)node];
    // if it is a str, wrap it in a new NSString
    if( type == 2 ) { // string
        uj_str *node2 = (uj_str *) node;
        const char *val = node2->str;
        int len = node2->len;
        return [NSString stringWithFormat:@"%.*s",len,val];
    }
    if( type == 3 ) {
        // create an NSArray and populate it with the contents recursing through this
        NSMutableArray *arr = [NSMutableArray array];
        uj_arr *arrc = (uj_arr *) node;
        uj_node *cur = arrc->head;
        for( int i=0;i<arrc->count; i++) {
            [arr addObject:[self node2ns:cur]];
            cur = cur->base.parent;
        }
        return arr;
    }
    if( type == 4 ) { // positive int
        // wrap in NSInt
        uj_str *node2 = (uj_str *) node;
        const char *val = node2->str;
        int len = node2->len;
        NSString *str = [NSString stringWithFormat:@"%.*s",len,val];
        return @([str intValue]);
    }
    if( type == 5 ) { // negative int
        uj_str *node2 = (uj_str *) node;
        const char *val = node2->str;
        int len = node2->len;
        NSString *str = [NSString stringWithFormat:@"-%.*s",len,val];
        return @([str intValue]);
    }
    if( type == 6 ) return @(YES);
    if( type == 7 ) return @(NO);
    if( type == 8 ) return [NSNull null];
    return nil;
}

@end

@implementation UJHash

- (instancetype)init {
    return [self initWithNode:uj_hash__new()];
}

- (instancetype)initWithNode:(uj_hash *) hash {
    self = [super init];
    _ujhash = hash;
    hash->refCnt++;
    return self;
}

- (instancetype)initWithString:(NSString *) str {
    const char *strc = [str UTF8String];
    int parseErr;
    uj_hash *root = uj_parse( strc, (int) strlen( strc ), NULL, &parseErr );
    root->refCnt--; // because initWithNode will increase the refCnt
    return [self initWithNode:root];
    //return self;
}

+ (instancetype)hashWithString:(NSString *) str {
    return [[self alloc] initWithString:str];
}

- (instancetype)initWithData:(NSData *) data {
    NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    return [self initWithString:str];
}

+ (instancetype)hashWithData:(NSData *)data {
    return [[self alloc] initWithData:data];
}

- (instancetype)initWithDict:(NSDictionary *)dict {
    return [self initWithNode:[dict toNode]];
}

+ (instancetype)hashWithDict:(NSDictionary *)dict {
    return [[self alloc] initWithDict:dict];
}

- (NSUInteger)count {
    return string_tree__getkeycount( _ujhash->tree );
}

- (NSEnumerator *)keyEnumerator {
    return [[NodeHashEnumerator alloc] initWithNode:_ujhash];
}

- (id)objectForKey:(id)key {
    NSString *keyns = (NSString *) key;
    const char *keyc = [keyns UTF8String];
    uj_node *node = uj_hash__get( _ujhash, keyc, (int) strlen( keyc ) );
    if( !node ) return nil;
    return [UJson node2ns:node];
}

- (void)setObject:(id)obj forKey:(id <NSCopying>)key {
    uj_node *node = [obj toNode];
    NSString *keyns = (NSString *) key;
    const char *keyc = [keyns UTF8String];
    unsigned keylen = (unsigned) strlen( keyc );
    // Use a cached key because this C version of the NSString will vanish
    const char *cachedKey = keycache__store( keyc, keylen );
    uj_hash__store( _ujhash, cachedKey, keylen, node );
}

- (void)removeObjectForKey:(id)key {
    NSString *keyns = (NSString *)key;
    const char *keyc = [keyns UTF8String];
    unsigned keylen = (unsigned) strlen( keyc );
    uj_hash__remove( _ujhash, keyc, keylen );
}

- (NSString *)description {
    NSLog(@"custom description called");
    return [self toJSONx];
}

- (NSString *)toJSON {
    char *json = uj_node__json( _ujhash, 0, 0 );
    NSString *json2 = [NSString stringWithFormat:@"%s", json];
    sdsfree(json);
    return json2;
}

- (NSString *)toJSONx {
    char *json = uj_node__jsonx( _ujhash, 0, 0 );
    NSString *json2 = [NSString stringWithFormat:@"%s", json];
    sdsfree(json);
    return json2;
}

- (void)dealloc {
    // node_hash now uses a reference count, so this may not actually delete it
    uj_hash__delete( _ujhash );
    //[super dealloc];
}

@end
