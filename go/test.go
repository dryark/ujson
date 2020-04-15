package main

import (
    "io/ioutil"
)

func main() {
    content, _ := ioutil.ReadFile("../test.json")
    root := parse( content )
    root.dump()
    sub := root.get("sub")
    sub.dump()
    print("z:", sub.get("z").int() )
    print("\nneg:", root.get("neg").int() )
    print("\nz2:", root.get("sub.z").int() )
}