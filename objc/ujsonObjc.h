#ifndef ujsoninObjc_h
#define ujsoninObjc_h

#import <Foundation/Foundation.h>
#import <ujson.h>

@interface NodeHashEnumerator : NSEnumerator

@property (nonatomic) int i;
@property (nonatomic) uj_hash *ujhash;
@property (nonatomic) xjr_key_arr *keys;

- (instancetype)initWithNode:(uj_hash *)hash;

@end

@interface UJson : NSObject
+ (id)node2ns:(uj_node *)node;
@end


@interface UJHash : NSMutableDictionary

@property (nonatomic) uj_hash *ujhash;
@property (nonatomic) bool mortal;

- (instancetype)init;
- (instancetype)initWithDict:(NSDictionary *)dict;
- (instancetype)initWithNode:(uj_hash *) hash;
- (instancetype)initWithString:(NSString *) str;
- (instancetype)initWithData:(NSData *) data;
- (NSString *)toJSON;
- (NSString *)toJSONx;

+ (instancetype)hashWithDict:(NSDictionary *)dict;
+ (instancetype)hashWithString:(NSString *)str;
+ (instancetype)hashWithData:(NSData *) data;

@end

#endif
