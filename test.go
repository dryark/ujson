package main

import (
    "fmt"
    "io/ioutil"
    uj "github.com/nanoscopic/ujsonin/mod"
)

func main() {
    content, _ := ioutil.ReadFile("test.json")
    root, left := uj.Parse( content )
    fmt.Printf("extra stuff:" + string(left) + "\n" )
    root.Dump()
    sub := root.Get("sub")
    sub.Dump()
    print("z:", sub.Get("z").Int() )
    print("\nneg:", root.Get("neg").Int() )
    print("\nz2:", root.Get("sub.z").Int() )
    fmt.Println( "\narray items\n" )
    arrnode := root.Get("arr")
    arrnode.ForEach( func( item *uj.JNode ) {
		fmt.Printf( "  item: %d\n", item.Int() )
    } )
}
