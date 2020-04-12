package main

import (
    "fmt"
    "io/ioutil"
    "strings"
)

func main() {
    content, _ := ioutil.ReadFile("test.json")
    root := parse( content )
    DumpNodeHash( root, 1 )
}

type JNode struct {
    parent   *JNode
    nodeType uint8
    hash map [ string ] *JNode
    str      *string
}

func NewNodeHash() ( map [ string ] *JNode ) {
    return make( map [ string ] *JNode )
}

func NewNodeArr() ( *JNode ) {
    return &JNode{ nodeType: 3, hash: NewNodeHash() }
}

func NodeArrAdd( self *JNode, el *JNode ) {
    last := self.parent // parent serves as last till the array is done
    if last == nil {
        self.parent = el
        self.hash["first"] = el
        return
    }
    last.parent = el
    self.parent = el
}

func DumpNodeHash( self *JNode, depth int ) {
    fmt.Printf("{\n")
    
    for key, val := range self.hash {
        fmt.Printf("%s\"%s\":",strings.Repeat("  ",depth), key);
        DumpJNode( val, depth );
    }
    depth--;
    fmt.Printf("%s}\n",strings.Repeat("  ",depth))
}

func DumpNodeArr( self *JNode, depth int ) {
    fmt.Printf("[\n")
    
    cur := self.parent
    for cur != nil {
        fmt.Printf(strings.Repeat("  ",depth));
        DumpJNode( cur, depth );
        cur = cur.parent
    }
    depth--;
    fmt.Printf("%s]\n",strings.Repeat("  ",depth))
}

func DumpJNode( self *JNode, depth int ) {
    if self.nodeType == 1 {
        DumpNodeHash( self, depth+1 )
    } else if self.nodeType == 2 {
        fmt.Printf("\"%s\"\n", *self.str )
    } else if self.nodeType == 3 {
        DumpNodeArr( self, depth+1 )
    }
}

func parse( data [] byte ) (*JNode) {
    size := len( data )
    
    pos := 1
    keyStart := 0
    strStart := 0
    key := ""
    var let byte
    
    cur := &JNode{ nodeType: 1, hash: NewNodeHash() }
    root := cur
Hash:
    let = data[pos]
    pos++
    if pos >= size { goto Done } 
    if let == '"' {
        goto QQKeyName1
    }
    if let >= 'a' && let <= 'z' {
        pos--
        goto KeyName1
    }
    if let == '}' && cur.parent != nil {
        cur = cur.parent
    }
    if let == '/' && pos < (size-1) {
        if data[pos] == '/' {
            pos++
            goto HashComment
        }
        if data[pos] == '*' {
            pos++
            goto HashComment2
        }
    }
    goto Hash
HashComment:
    let = data[pos]
    pos++
    if let == 0x0d || let == 0x0a {
        goto Hash
    }
    goto HashComment
HashComment2:
    let = data[pos]
    pos++
    if( let == '*' && pos < (size-1) && data[pos] == '/' ) { pos++; goto Hash; }
    goto HashComment2;
QQKeyName1:
    keyStart = pos
    pos++
QQKeyNameX:
    let = data[pos]
    pos++
    if let == '"' {
        key = string( data[ keyStart : pos - 1 ] )
        goto Colon
    }
    goto QQKeyNameX
KeyName1:
    keyStart = pos
    pos++
KeyNameX:
    let = data[pos]
    pos++
    if let == ':' {
        key = string( data[ keyStart : pos - 1 ] )
        goto AfterColon
    }
    if let == ' ' || let == '\t' {
        key = string( data[ keyStart : pos - 1 ] )
        goto Colon
    }
    goto KeyNameX
Colon:
    let = data[pos]
    pos++
    if let == ':' {
        goto AfterColon
    }
    goto Colon
AfterColon:
    let = data[pos]
    pos++
    if let == '"' {
        goto String1
    }
    if let == '{' {
        newJNode := &JNode{ nodeType: 1, hash: NewNodeHash() }
        newJNode.parent = cur;
        if cur.nodeType == 1 {
            cur.hash[ key ] = newJNode
        }
        if cur.nodeType == 3 {
            NodeArrAdd( cur, newJNode )
        }
        cur = newJNode
        goto Hash
    }
    if let == '/' && pos < (size-1) {
        if data[pos] == '/' {
            pos++
            goto AC_Comment
        }
        if data[pos] == '*' {
            pos++
            goto AC_Comment2
        }
    }
    // if( let == 't' || let == 'f' ) ... for true/false
    // if( let >= '0' && let <= '9' ) ... for numbers
    if let == '[' {
        newArr := NewNodeArr()
        newArr.hash["parent"] = cur
        if cur.nodeType == 1 {
            cur.hash[ key ] = newArr
        }
        if cur.nodeType == 3 {
            NodeArrAdd( cur, newArr )
        }
        cur = newArr;
        goto AfterColon;
    }
    if let == ']' {
        oldCur := cur
        cur = cur.hash["parent"]
        oldCur.parent = oldCur.hash["first"]
        if cur.nodeType == 3 { goto AfterColon }
        if cur.nodeType == 1 { goto Hash }
        // should never reach here
    }    
    goto AfterColon;
AC_Comment:
    let = data[pos]
    pos++
    if let == 0x0d || let == 0x0a {
        goto AfterColon
    }
    goto AC_Comment
AC_Comment2:
    let = data[pos]
    pos++
    if let == '*' && pos < (size-1) && data[pos] == '/' {
        pos++
        goto Hash
    }
    goto AC_Comment2
String1:
    let = data[pos]
    pos++
    if( let == '"' ) {
       empty := ""
       newJNode := &JNode{ nodeType: 2, str: &empty } 
       if cur.nodeType == 1 {
           cur.hash[ key ] = newJNode
           goto AfterVal
       }
       if cur.nodeType == 3 {
           NodeArrAdd( cur, newJNode )
           goto AfterColon
       }
       goto AfterVal // should be unreachable
    }
    strStart = pos - 1
StringX:
    let = data[pos]
    pos++
    if let == '"' {
       str := string( data[ strStart : pos - 1 ] )
       newJNode := &JNode{ nodeType: 2, str: &str }
       if cur.nodeType == 1 {
           cur.hash[ key ] = newJNode
           goto AfterVal
       }
       if cur.nodeType == 3 {
           NodeArrAdd( cur, newJNode )
           goto AfterColon
       }
       goto AfterVal // should be unreachable
    }
    goto StringX;   
AfterVal:
    // who cares about commas in between things; we can just ignore them :D
    goto Hash
Done:
    return root
}