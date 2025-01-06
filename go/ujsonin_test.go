package mod

import (
  "fmt"
)

func dumpj( json string ) (JNode,string,*ParseError) {
  root, left, err := ParseFull( []byte( json ) )
  if root != nil {
    root.Dump()
  }
  if len(left) == 0 { return root,"",err }
  return root,string(left),err
}

func ExampleKeyUnquoted() {
  dumpj("{ x:1 }")
  // Output:
  // {
  //   x:1
  // }
}

func ExampleKeyDoubleQuotes() {
  dumpj("{ \"x\":1 }")
  // Output:
  // {
  //   x:1
  // }
}

func ExampleKeySingleQuotes() {
  dumpj("{ 'x':1 }")
  // Output:
  // {
  //   x:1
  // }
}

func ExampleValNum() {
  dumpj("{ x:1 }")
  // Output:
  // {
  //   x:1
  // }
}

func ExampleValTrue() {
  dumpj("{ x:true }")
  // Output:
  // {
  //   x:true
  // }
}

func ExampleValFalse() {
  dumpj("{ x:false }")
  // Output:
  // {
  //   x:false
  // }
}

func ExampleValNull() {
  dumpj("{ x:null }")
  // Output:
  // {
  //   x:null
  // }
}

func ExampleTrailing() {
  _,trail,_ := dumpj("{ x:1 }blah")
  fmt.Println("")
  fmt.Println( trail )
  // Output:
  // {
  //   x:1
  // }
  // blah
}

func ExampleValArray() {
  _,trail,_ := dumpj("{ x:[1,'b'] }blah")
  fmt.Println("")
  fmt.Println( trail )
  // Output:
  // {
  //   x:[
  //     1
  //     "b"
  //   ]
  // }
  // blah
}

func ExampleArrayRoot() {
  dumpj("[1,'b']")
  // Output:
  // [
  //   1
  //   "b"
  // ]
}

// Demonstrated that only \" is actually decoded
// All other escape sequences are left alone ( including \\ )
// This allows you to post-process the escapes if desired
func ExampleEscapeQuot() {
  root,_,_ := dumpj(`["\'test\"b\\"]`)
  fmt.Println("")
  fmt.Printf( "%s", root.GetAt(0).String() )
  // Output:
  // [
  //   "\'test\"b\\"
  // ]
  // \'test"b\\
}

func ExampleBadType() {
  _,_,err := dumpj("[1,2,blah]")
  fmt.Println("")
  if err != nil {
    fmt.Printf("Error:%s\n", err.text )
  }
  // Output:
  // [
  //   1
  //   2
  // ]
  // Error:unknown type 'blah'
}

func ExampleInset() {
  _,_,err := dumpj("{a:{b:1}}")
  fmt.Println("")
  if err != nil {
    fmt.Printf("Error:%s\n", err.text )
  }
  // Output:
  // {
  //   a:{
  //     b:1
  //   }
  // }
}