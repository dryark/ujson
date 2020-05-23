package ujsoningo

import (
    "fmt"
    "strconv"
    "strings"
)

type JNode struct {
    parent   *JNode
    nodeType uint8
    hash map [ string ] *JNode
    str      *string
}

func ( self JNode ) Get( key string ) ( *JNode ) {
    if strings.Contains(key,".") {
        parts := strings.Split(key,".")
        cur := &self
        for _, part := range parts {
            cur = cur.Get(part)
            if cur == nil { return nil }
        }
        return cur
    }
    return self.hash[ key ]
}

func ( self JNode ) String() (string) {
    return *self.str
}

func ( self JNode ) Int() (int) {
    if( self.nodeType == 4 ) {
        i,_:=strconv.Atoi( *self.str )
        return i;
    }
    if( self.nodeType == 5 ) {
        i,_:=strconv.Atoi( *self.str )
        return -i
    }
    i,_:=strconv.Atoi( *self.str )
    return i
}

func NewNodeHash() ( map [ string ] *JNode ) {
    return make( map [ string ] *JNode )
}

func NewNodeArr() ( *JNode ) {
    return &JNode{ nodeType: 3, hash: NewNodeHash() }
}

func ( self JNode ) add_item( el *JNode ) {
    last := self.parent // parent serves as last till the array is done
    if last == nil {
        self.parent = el
        self.hash["first"] = el
        return
    }
    last.parent = el
    self.parent = el
}

func ( self JNode ) Dump() {
    self.dump_internal( 1 )
}

func ( self JNode ) dump_internal( depth int ) {
    fmt.Printf("{\n")
    
    for key, val := range self.hash {
        basicKey := true
        l := key[0]
        if ( ( l < 'a' && l > 'z' ) || ( l < 'A' && l > 'Z' ) ) {
            basicKey = false
        } else {
            for i := 1; i < len(key); i++ {
                l = key[i]
                if      l >= 'a' && l <= 'z' {
                } else if l >= 'A' && l <= 'Z' {
                } else if l >= '0' && l <= '9' {
                } else {
                    basicKey = false
                }
            }
        }
        if basicKey {
            fmt.Printf("%s%s:",strings.Repeat("  ",depth), key);
        } else {
            fmt.Printf("%s\"%s\":",strings.Repeat("  ",depth), key);
        }
        val.dump_val( depth );
    }
    depth--;
    fmt.Printf("%s}\n",strings.Repeat("  ",depth))
}

func ( self JNode ) dump_array( depth int ) {
    fmt.Printf("[\n")
    
    cur := self.parent
    for cur != nil {
        fmt.Printf(strings.Repeat("  ",depth));
        cur.dump_val( depth );
        cur = cur.parent
    }
    depth--;
    fmt.Printf("%s]\n",strings.Repeat("  ",depth))
}

func ( self JNode ) dump_val( depth int ) {
    if self.nodeType == 1 {
        self.dump_internal( depth+1 )
    } else if self.nodeType == 2 {
        fmt.Printf("\"%s\"\n", *self.str )
    } else if self.nodeType == 3 {
        self.dump_array( depth+1 )
    } else if self.nodeType == 4 {
        fmt.Printf("%s\n", *self.str )
    } else if self.nodeType == 5 {
        fmt.Printf("-%s\n", *self.str )
    }
}

func Parse( data [] byte ) (*JNode) {
    size := len( data )
    
    pos := 1
    keyStart := 0
    strStart := 0
    key := ""
    neg := false
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
            cur.add_item( newJNode )
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
    if let >= '0' && let <= '9' {
        neg = false
        goto Number1
    }
    if let == '-' {
        neg = true
        pos++
        goto Number1
    }
    if let == '[' {
        newArr := NewNodeArr()
        newArr.hash["parent"] = cur
        if cur.nodeType == 1 {
            cur.hash[ key ] = newArr
        }
        if cur.nodeType == 3 {
            cur.add_item( newArr )
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
Number1:
    strStart = pos - 1
NumberX:
    let = data[pos]
    pos++
    //if( let == '.' ) goto AfterDot;
    if let < '0' || let > '9' {
        str := string( data[ strStart : pos - 1 ] )
        newJNode := &JNode{ str: &str }
        if neg {
            newJNode.nodeType = 5
        } else {
            newJNode.nodeType = 4
        }
        if cur.nodeType == 1 {
            cur.hash[ key ] = newJNode
            goto AfterVal
        }
        if cur.nodeType == 3 {
            cur.add_item( newJNode )
            goto AfterColon
        }
    }
    goto NumberX;
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
           cur.add_item( newJNode )
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
           cur.add_item( newJNode )
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
