package mod

import (
    "fmt"
    "os"
    "strconv"
    "strings"
)

const (
    TYPE_HASH = iota + 1
    TYPE_STR
    TYPE_ARR
    TYPE_POS
    TYPE_NEG
    TYPE_TRUE
    TYPE_FALSE
    TYPE_NULL
)

type JNode struct {
    parent   *JNode
    NodeType uint8
    hash map [ string ] *JNode
    str      *string
    count    uint8
}

func ( self *JNode ) Get( key string ) ( *JNode ) {
    if strings.Contains(key,".") {
        parts := strings.Split(key,".")
        cur := self
        for _, part := range parts {
            cur = cur.Get(part)
            if cur == nil { return nil }
        }
        return cur
    }
    return self.hash[ key ]
}

func ( self *JNode ) Overlay( ontop *JNode ) {
    if self.NodeType != TYPE_HASH { return }
    if ontop.NodeType != TYPE_HASH { return }
    
    for key, ontopVal := range ontop.hash {
        val, valOk := self.hash[ key ]
        if ontopVal.NodeType == TYPE_HASH {
            if !valOk {
                self.hash[ key ] = ontopVal
            } else {
                val.Overlay( ontopVal )
            }
        } else {
            if !valOk || ontopVal.NodeType != TYPE_HASH {
                self.hash[ key ] = ontopVal
            }
            continue
        }
        
    }
}

func ( self JNode ) String() (string) {
    return *self.str
}

func ( self *JNode ) Bool() (bool) {
    if( self.NodeType == TYPE_TRUE ) {
        return true
    }
    if( self.NodeType == TYPE_FALSE ) {
        return false
    }
    return false
}

func ( self *JNode ) Int() (int) {
    if( self.NodeType == TYPE_POS ) {
        i , _ := strconv.Atoi( *self.str )
        return i;
    }
    if( self.NodeType == TYPE_NEG ) {
        i , _ := strconv.Atoi( *self.str )
        return -i
    }
    if( self.NodeType == TYPE_TRUE ) {
        return 1
    }
    if( self.NodeType == TYPE_FALSE ) {
        return 0
    }
    i , _ := strconv.Atoi( *self.str )
    return i
}

func NewNodeHash() ( map [ string ] *JNode ) {
    return make( map [ string ] *JNode )
}

func NewNodeArr() ( *JNode ) {
    return &JNode{ NodeType: TYPE_ARR, hash: NewNodeHash() }
}

func ( self *JNode ) add_item( el *JNode ) {
    oldLast := self.parent // parent serves as last till the array is done
    if oldLast == nil {
        self.parent = el
        //fmt.Printf("setting first\n")
        self.hash["first"] = el
        self.count = 1
        return
    }
    oldLast.parent = el // set "next"(parent) of the last element to the new last
    self.parent = el // set the "last"(parent) of self to the new last item
    self.count = self.count + 1
}

func (self *JNode) Dump() {
	self.dump_val( 0, false )
}

func (self *JNode) Json() {
    self.dump_val( 0, true )
}

func ( self *JNode ) dump_internal( depth int, json bool ) {
    fmt.Printf("{\n")
    
    keyNum := 0
    for key, val := range self.hash {
        if keyNum > 0 {
            if json {
                fmt.Printf(",\n")
            } else {
                fmt.Printf("\n")
            }
        }
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
        if !json && basicKey {
            fmt.Printf("%s%s:",strings.Repeat("  ",depth), key)
        } else {
            fmt.Printf("%s\"%s\":",strings.Repeat("  ",depth), key)
        }
        val.dump_val( depth, json )
        keyNum++
    }
    depth--;
    fmt.Printf("\n%s}",strings.Repeat("  ",depth))
}

func ( self *JNode ) ForEach( handler func( *JNode ) ) {
	if self.NodeType != TYPE_ARR {
		fmt.Println("not an array")
		return
	}
	//fmt.Printf("Count: %d\n", self.count )
	cur := self.parent
	count := self.count
	i := uint8( 0 )
	for {
		i = i + 1
		if i > count { break }
		handler( cur )
		cur = cur.parent
	}
}

func ( self *JNode ) ForEachKeyed( handler func( string, *JNode ) ) {
  if self.NodeType != TYPE_HASH {
    fmt.Println("not a hash")
    return
  }
  for key, node := range self.hash {
    handler( key, node )
  }
}

func ( self *JNode ) dump_array( depth int, json bool ) {
    fmt.Printf("[\n")
    cur := self.parent
    count := self.count
	i := uint8( 0 )
    for {
    	i = i + 1
    	if i > count { break }
        fmt.Printf(strings.Repeat("  ",depth))
        cur.dump_val( depth, json )
        if i != count {
            if json {
                fmt.Printf(",\n")
            } else {
                fmt.Printf("\n")
            }
        }
        cur = cur.parent
    }
    depth--;
    fmt.Printf("\n%s]",strings.Repeat("  ",depth))
}

func ( self *JNode ) dump_val( depth int, json bool ) {
    if self.NodeType == TYPE_HASH {
        self.dump_internal( depth+1, json )
    } else if self.NodeType == TYPE_STR {
        fmt.Printf("\"%s\"", *self.str )
    } else if self.NodeType == TYPE_ARR {
        self.dump_array( depth+1, json )
    } else if self.NodeType == TYPE_POS { // positive number
        fmt.Printf("%s", *self.str )
    } else if self.NodeType == TYPE_NEG { // negative number
        fmt.Printf("-%s", *self.str )
    } else if self.NodeType == TYPE_TRUE {
        fmt.Printf("true")
    } else if self.NodeType == TYPE_FALSE {
        fmt.Printf("false")
    } else if self.NodeType == TYPE_NULL {
        fmt.Printf("null")
    }
}

var handlers map[string] func( []byte, int ) ( int, *JNode ) = make( map[string] func( []byte, int ) ( int, *JNode ) )

func add_handler( name string, handler func( []byte, int ) ( int, *JNode ) ) {
    handlers[ name ] = handler
}

func handle_true( data []byte, pos int ) ( int, *JNode ) {
    return 0, &JNode{ NodeType: TYPE_TRUE }
}

func handle_false( data []byte, pos int ) ( int, *JNode ) {
    return 0, &JNode{ NodeType: TYPE_FALSE }
}

func handle_null( data []byte, pos int ) ( int, *JNode ) {
    return 0, &JNode{ NodeType: TYPE_NULL }
}

func init() {
    add_handler( "true", handle_true )
    add_handler( "false", handle_false )
    add_handler( "null", handle_null )
}

func Parse( data []byte ) ( *JNode, []byte ) {
    var remainder []byte
    var dest int
    var node *JNode
    var handler func( []byte, int ) ( int, *JNode )
    var typeWord string
    size := len( data )
    
    pos := 1
    keyStart := 0
    strStart := 0
    typeStart := 0
    key := ""
    neg := false
    var let byte
    
    cur := &JNode{ NodeType: TYPE_HASH, hash: NewNodeHash() }
    root := cur
Hash:
    let = data[pos]
    //if let != ' ' && let != '\n' { fmt.Printf("Hash %c %d\n", let, let ) }
    pos++
    if pos >= size { goto Done } 
    if let == '\'' {
        goto QKeyName1
    }
    if let == '"' {
        goto QQKeyName1
    }
    if let >= 'a' && let <= 'z' || let >= 'A' && let <= 'Z' {
        pos--
        goto KeyName1
    }
    if let == '}' {
        if cur.parent != nil {
            cur = cur.parent
            //fmt.Printf("Ascend to type %d\n", cur.NodeType)
            if cur.NodeType == TYPE_ARR {
            	goto AfterColon
            }
            goto Hash
        } else {
            remainder = data[pos:]
            goto Done
        }
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
QKeyName1:
    keyStart = pos
    pos++
QKeyNameX:
    let = data[pos]
    pos++
    if let == '\'' {
        key = string( data[ keyStart : pos - 1 ] )
        goto Colon
    }
    goto QKeyNameX
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
	//fmt.Printf("KeyName1 %c %d\n", let, let )
    keyStart = pos
    pos++
KeyNameX:
    let = data[pos]
    //fmt.Printf("KeyNameX %c %d\n", let, let )
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
    //if let != ' ' && let != '\n' { fmt.Printf("AfterColon %c %d\n", let, let ) }
    pos++
    if let == '"' {
        goto String1
    }
    if let == '\'' {
        goto SQString1
    }
    if let == '{' {
        newJNode := &JNode{ NodeType: TYPE_HASH, hash: NewNodeHash() }
        newJNode.parent = cur;
        if cur.NodeType == TYPE_HASH {
            cur.hash[ key ] = newJNode
        }
        if cur.NodeType == TYPE_ARR {
            cur.add_item( newJNode )
        }
        cur = newJNode
        goto Hash
    }
    if let >= 'a' && let <= 'z' {
        typeStart = pos - 1
        goto TypeX
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
        newArr.count = 0
        newArr.hash["parent"] = cur
        if cur.NodeType == TYPE_HASH {
            cur.hash[ key ] = newArr
        }
        if cur.NodeType == TYPE_ARR {
            cur.add_item( newArr )
        }
        cur = newArr
        goto AfterColon
    }
    if let == ']' {
        array := cur
        cur = array.hash["parent"] // the original parent
        array.parent = array.hash["first"]
        delete( array.hash, "parent" )
        delete( array.hash, "first" )
        if cur.NodeType == TYPE_ARR { goto AfterColon }
        if cur.NodeType == TYPE_HASH { goto Hash }
        // should never reach here
    }
    goto AfterColon;
TypeX:
    let = data[pos]
    pos++
    if ( let >= '0' && let <= '9' ) || ( let >= 'a' && let <= 'z' ) { goto TypeX }
    pos--
    typeWord = string( data[ typeStart : pos ] )
    handler = handlers[ typeWord ]
    if handler == nil {
        fmt.Printf("unknown type '%s'\n", typeWord )
        os.Exit(1)
    }
    dest, node = handler( data, pos )
    if dest == 0 {
        if cur.NodeType == TYPE_HASH {
            cur.hash[ key ] = node
            goto Hash
        } else if cur.NodeType == TYPE_ARR {
            cur.add_item( node )
            goto AfterColon
        }
    }
    fmt.Printf("unknown dest\n")
    os.Exit(1)
    goto TypeX
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
        newJNode := JNode{ str: &str }
        if neg {
            newJNode.NodeType = TYPE_NEG
        } else {
            newJNode.NodeType = TYPE_POS
        }
        if cur.NodeType == 1 {
            cur.hash[ key ] = &newJNode
            pos--
            goto Hash
        }
        if cur.NodeType == TYPE_ARR {
            cur.add_item( &newJNode )
            pos-- // so that ] gets recognized
            goto AfterColon
        }
    }
    goto NumberX;
SQString1:
    let = data[pos]
    //fmt.Printf("String1 %c %d\n", let, let )
    pos++
    if( let == '\'' ) {
       empty := ""
       newJNode := JNode{ NodeType: TYPE_STR, str: &empty } 
       if cur.NodeType == TYPE_HASH {
           cur.hash[ key ] = &newJNode
           goto Hash
       }
       if cur.NodeType == TYPE_ARR {
           cur.add_item( &newJNode )
           goto AfterColon
       }
       goto AfterVal // should be unreachable
    }
    strStart = pos - 1
SQStringX:
    let = data[pos]
    //fmt.Printf("StringX %c %d\n", let, let )
    pos++
    if let == '\'' {
       str := string( data[ strStart : pos - 1 ] )
       newJNode := &JNode{ NodeType: TYPE_STR, str: &str }
       if cur.NodeType == TYPE_HASH {
           cur.hash[ key ] = newJNode
           goto Hash
       }
       if cur.NodeType == TYPE_ARR {
           cur.add_item( newJNode )
           goto AfterColon
       }
       goto AfterVal // should be unreachable
    }
    goto SQStringX;
String1:
    let = data[pos]
    //fmt.Printf("String1 %c %d\n", let, let )
    pos++
    if( let == '"' ) {
       empty := ""
       newJNode := JNode{ NodeType: TYPE_STR, str: &empty } 
       if cur.NodeType == TYPE_HASH {
           cur.hash[ key ] = &newJNode
           goto Hash
       }
       if cur.NodeType == TYPE_ARR {
           cur.add_item( &newJNode )
           goto AfterColon
       }
       goto AfterVal // should be unreachable
    }
    strStart = pos - 1
StringX:
    let = data[pos]
    //fmt.Printf("StringX %c %d\n", let, let )
    pos++
    if let == '"' {
       str := string( data[ strStart : pos - 1 ] )
       newJNode := &JNode{ NodeType: TYPE_STR, str: &str }
       if cur.NodeType == TYPE_HASH {
           cur.hash[ key ] = newJNode
           goto Hash
       }
       if cur.NodeType == TYPE_ARR {
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
    return root, remainder
}
