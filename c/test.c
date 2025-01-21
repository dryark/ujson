#include"ujson.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"sds.h"

int main( int argc, char *argv[] ) {
    uj_init();
    
    if( argc == 1 ) {
        printf("Usage: ???\n");
        exit(0);
    }
    string_tree *args = string_tree__new();
    char *cmd = argv[1];
    for( int i=2;i<argc;i++ ) {
        char *key = argv[i];
        if( key[0] == '-' ) {
            key++;
            char *val = argv[++i];
            char extra = 0;
            char *vals[7] = {0,0,0,0,0,0,0};
            for( int j=i+1;j<argc;j++ ) {
                char *another = argv[j];
                if( another[0] == '-' ) break;
                extra++;
                vals[extra] = another;
            }
            if( extra ) {
                vals[0] = val;
                string_tree__store_len( args, key, strlen( key ), (void *) vals, 2 );
                i += extra;
            }
            else {
                string_tree__store_len( args, key, strlen( key ), (void *) val, 1 );
            }
        }
    }
    
    char type;
    if( !strncmp(cmd,"makefile",8) ) {
        char *file = string_tree__get_len( args, "file", 4, &type );
        char *defaults = string_tree__get_len( args, "defaults", 8, &type );
        char *prefix1 = string_tree__get_len( args, "prefix", 6, &type );
        
        char prefix[100];
        if( prefix1 ) sprintf(prefix,"%s_",prefix1);
        char *d1, *d2;
        uj_hash__dump_to_makefile( uj_parse_with_default(file,defaults, &d1, &d2 ), prefix1 ? prefix : 0 );
        exit(0);
    }
    if( !strncmp(cmd,"get",3) ) {
        char *file = string_tree__get_len( args, "file", 4, &type );
        if( !file ) {
            fprintf(stderr,"-file must be specified\n");
            exit(1);
        }
        char *defaults = string_tree__get_len( args, "defaults", 8, &type );
        char *d1,*d2;
        uj_node *cur = (uj_node *) uj_parse_with_default( file, defaults, &d1, &d2 );
        
        void *pathV = string_tree__get_len( args, "path", 4, &type );
        if( !pathV ) {
            fprintf(stderr,"-path must be specified\n");
            exit(1);
        }
        char *onepart[2] = {0,0};
        char **parts;
        if( type == 1 ) {
            onepart[0] = (char *) pathV;
            parts = onepart;
        }
        if( type == 2 ) parts = ( char ** ) pathV;
        
        for( int i=0;i<6;i++ ) {
            char *part = parts[i];
            if( !part ) break;
            uj_hash *curhash = ( uj_hash * ) cur;
            cur = uj_hash__get( curhash, part, strlen( part ) );
        }
        //jnode__dump_env( cur );
        exit(0);
    }
    if( !strcmp(cmd,"test") ) {
        unsigned long len;
        char *data = slurp_file( "test.json", &len );
        int err;
        uj_hash *root = uj_parse( data, len, NULL, &err );
        
        printf("Dump of node:\n");
        uj_node__dump( (uj_node *) root, 0 );
        printf("--\n\n");
        
        printf("jnode__jsonof node:[");
        sds str = uj_node__json( (uj_node *) root, 0, 0 );
        fputs( str, stdout );
        printf("]\n");
        sdsfree( str );
        
        printf("jnode__asstr of str:[");
        sds str2 = uj_hash__get_str( (uj_hash *) root, "str", 3 );
        fputs( str2, stdout );
        printf("]\n");
        sdsfree( str2 );
        
        printf("jnode__asstr of str2:[");
        sds str3 = uj_hash__get_str( (uj_hash *) root, "str2", 3 );
        fputs( str3, stdout );
        printf("]\n");
        sdsfree( str3 );
        
        char *json = uj_node__json( (uj_node *) root, 0, NULL );
        printf("json:%s\n", json );
        sdsfree( json );
        
        uj_hash__delete( root );
        exit(0);   
    }
    if( !strcmp(cmd,"test2") ) {
        unsigned long len;
        char *data = slurp_file( "test2.json", &len );
        int err;
        uj_hash *root = uj_parse( data, len, NULL, &err );
        
        char *json = uj_node__json( (uj_node *) root, 0, NULL );
        printf("json:%s\n", json );
        sdsfree( json );
        
        uj_hash__delete( root );
        exit(0);   
    }
    if( !strcmp(cmd,"del") ) {
        unsigned long len;
        char *data = slurp_file( "test_del.json", &len );
        int err;
        uj_hash *root = uj_parse( data, len, NULL, &err );
        
        uj_str *d = uj_str__new( "4", 1 , 4 );
        uj_hash__store( root, "d", 1, (uj_node *) d );
        
        uj_hash__remove( root, "b", 1 );
        
        uj_hash__remove( root, "d", 1 );
        
        char *json = uj_node__json( (uj_node *) root, 0, NULL );
        printf("json:%s\n", json );
        sdsfree( json );
        
        uj_hash__delete( root );
        exit(0);   
    }
    if( !strcmp(cmd,"kc") ) { // keycache test
        const char *key = "test";
        
        const char *k1 = keycache__store( "test", 4 );
        printf("k1 == key? %i; strings equal k1,key:%i\n", k1 == key, !strcmp(k1,key) );
        
        const char *k2 = keycache__store( "test", 4 );
        printf("k2 == k1? %i\n", k2 == k1 );
        
        keycache__delete();
        exit(0);
    }
    
    fprintf(stderr,"Unknown command '%s'\n", cmd );
    return 1;
}