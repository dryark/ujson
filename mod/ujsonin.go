package mod

import (
    "fmt"
    "os"
    "strconv"
    "strings"
    "reflect"
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
    TYPE_FLOAT_POS
    TYPE_FLOAT_NEG
)

type JNode interface {
    Parent() ( JNode, uint8 )
    Type() uint8
    Get( key string ) JNode
    GetAt( pos int ) JNode
    GetArr() []JNode
    SetArr( []JNode )
    Overlay( ontop JNode )
    
    String() string
    StringEscaped() string
    Bool() bool
    Int() int
    Float32() float32
    
    ForEach( handler func( JNode ) )
    ForEachKeyed( handler func( string, JNode ) )
    Dump()
    DumpSave() string
    Json()
    JsonSave() string
    DumpInternal( depth int, json bool )
    DumpInternalSave( depth int, json bool ) string
    Add( key string, node JNode )
}

type JHash struct {
    parent   JNode
    NodeType uint8
    hash map [ string ] JNode
    count    uint8
}

type JArr struct {
    parent   JNode
    NodeType uint8
    arr [] JNode
}

type JVal struct {
    NodeType uint8
    str string
}

func ( self *JHash ) Parent() (JNode,uint8) {
    if self.parent != nil { return self.parent, self.parent.Type() }
    return nil, 0
}
func ( self *JArr ) Parent() (JNode,uint8) {
    if self.parent != nil { return self.parent, self.parent.Type() }
    return nil, 0
}
func ( self *JVal ) Parent() (JNode,uint8) { return nil, 0 }

func ( self *JHash ) Type() (uint8) { return self.NodeType }
func ( self *JArr ) Type() (uint8) { return self.NodeType }
func ( self *JVal ) Type() (uint8) { return self.NodeType }

func (self *JHash) GetAt( _ int ) JNode { return nil }
func (self *JVal) GetAt( _ int ) JNode { return nil }

func ( self *JArr ) GetAt( pos int ) JNode {
    return self.arr[ pos ]
}

func ( self *JArr ) Get( _ string ) (JNode) { return nil }
func ( self *JVal ) Get( _ string ) (JNode) { return nil }

func ( self *JHash ) Get( key string ) JNode {
    if strings.Contains(key,".") {
        parts := strings.Split(key,".")
        cur := JNode( self )
        for _, part := range parts {
            cur = cur.Get( part )
            if cur == nil { return nil }
        }
        return cur
    }
    return self.hash[ key ]
}

func ( self *JHash ) GetArr() ([]JNode) { return nil }
func ( self *JVal ) GetArr() ([]JNode) { return nil }
func ( self *JArr ) GetArr() ([]JNode) {
    return self.arr
}
func ( self *JHash ) SetArr( []JNode ) {}
func ( self *JVal ) SetArr( []JNode ) {}
func ( self *JArr ) SetArr( newArr []JNode ) {
    self.arr = newArr
}

func ( self *JArr ) Overlay( _ JNode )  {}
func ( self *JVal ) Overlay( _ JNode )  {}

func ( self *JHash ) Overlay( ontop JNode ) {
    ontop.ForEachKeyed( func( key string, ontopVal JNode ) {
        curVal := self.Get( key )
        ontopType := ontopVal.Type()
        curType := uint8(0)
        if curVal != nil {
            curType = curVal.Type()
        }
        if ontopType == TYPE_HASH {
            if curVal == nil {
                self.Add( key, ontopVal )
            } else if curType == TYPE_HASH {
                curVal.Overlay( ontopVal )
            } else {
                self.Add( key, ontopVal )
            }
        } else if ontopType == TYPE_ARR {
            if curVal == nil {
                self.Add( key, ontopVal )
            } else if curType == TYPE_ARR {
                arr := self.GetArr()
                addArr := ontopVal.GetArr()
                self.SetArr( append( arr, addArr... ) )
            } else {
                self.Add( key, ontopVal )
            }
        } else {
            //if !valOk || ontopType != TYPE_HASH {
                self.Add( key, ontopVal )
            //}
        }
        
    } )
}

func ( self *JHash ) String() (string) { return "" }
func ( self *JHash ) StringEscaped() (string) { return "" }
func ( self *JHash ) Bool() (bool) { return false }
func ( self *JHash ) ForEach( handler func( JNode ) ) {}
func ( self *JHash ) Int() int { return 0 }
func ( self *JHash ) Float32() (float32) { return 0 }

func ( self *JArr ) String() (string) { return "" }
func ( self *JArr ) StringEscaped() (string) { return "" }
func ( self *JArr ) Bool() (bool) { return false }
func ( self *JArr ) ForEachKeyed( handler func( string, JNode ) ) {}
func ( self *JArr ) Int() int { return 0 }
func ( self *JArr ) Float32() (float32) { return 0 }

func ( self JVal ) String() (string) {
    if( self.NodeType == TYPE_NEG ) {
        return "-" + self.str
    }
    
    return self.str
}

func ( self JVal ) StringEscaped() (string) {
    if( self.NodeType == TYPE_NEG ) {
        return "-" + self.str
    }
    
    str := []rune(self.str)
    escaped := ""
    for i:=0; i<len(str); i++ {
        let := str[i]
        if let == '\\' {
            i++
            let = str[i]
            if let != 'u' {
                escaped += string(let)
                continue
            }
            numStr := str[i+1:i+5]
            num, _ := strconv.ParseInt( string(numStr), 16, 32 )
            escaped += string( rune( num ) )
            i += 4
            continue
        }
        escaped += string(let)
    }
    return escaped
}

func ( self JVal ) Bool() (bool) {
    if( self.NodeType == TYPE_TRUE ) {
        return true
    }
    if( self.NodeType == TYPE_FALSE ) {
        return false
    }
    return false
}

func ( self *JVal ) Int() (int) {
    if( self.NodeType == TYPE_POS ) {
        i , _ := strconv.Atoi( self.str )
        return i;
    }
    if( self.NodeType == TYPE_NEG ) {
        i , _ := strconv.Atoi( self.str )
        return -i
    }
    if( self.NodeType == TYPE_TRUE ) {
        return 1
    }
    if( self.NodeType == TYPE_FALSE ) {
        return 0
    }
    i , _ := strconv.Atoi( self.str )
    return i
}

func ( self *JVal ) Float32() (float32) {
    if( self.NodeType == TYPE_FLOAT_POS ) {
        i , _ := strconv.ParseFloat( self.str, 32 )
        return float32( i );
    }
    if( self.NodeType == TYPE_FLOAT_NEG ) {
        i , _ := strconv.ParseFloat( self.str, 32 )
        return -float32( i )
    }
    if( self.NodeType == TYPE_POS ) {
        i , _ := strconv.Atoi( self.str )
        return float32( i );
    }
    if( self.NodeType == TYPE_NEG ) {
        i , _ := strconv.Atoi( self.str )
        return -float32( i )
    }
    if( self.NodeType == TYPE_TRUE ) {
        return 1
    }
    if( self.NodeType == TYPE_FALSE ) {
        return 0
    }
    i , _ := strconv.Atoi( self.str )
    return float32( i )
}

func NewJHash( parent JNode ) JNode {
    return JNode( &JHash{
        parent: parent,
        NodeType: TYPE_HASH,
        hash: make( map [ string ] JNode ),
    } )
}

func NewJArr( parent JNode ) JNode {
    return JNode( &JArr{
        parent: parent,
        NodeType: TYPE_ARR,
    } )
}

func (self *JVal) Add( _ string, _ JNode) {}

func (self *JHash) Add( key string, el JNode ) {
    self.hash[ key ] = el
}

func (self *JArr) Add( _ string, el JNode ) {
    self.arr = append( self.arr, el )
}

func (self *JHash) DumpInternal( depth int, json bool ) {
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
            fmt.Printf("%s\"%s\":",strings.Repeat("  ",depth+1), key)
        }
        val.DumpInternal( depth+1, json )
        keyNum++
    }
    depth--;
    fmt.Printf("\n%s}",strings.Repeat("  ",depth))
}

func (self *JHash) DumpInternalSave( depth int, json bool ) string {
    out := "{\n"
    
    keyNum := 0
    for key, val := range self.hash {
        if keyNum > 0 {
            if json {
                out += ",\n"
            } else {
                out += "\n"
            }
        }
        basicKey := true
        l := key[0]
        if ( ( l < 'a' && l > 'z' ) || ( l < 'A' && l > 'Z' ) ) {
            basicKey = false
        } else {
            for i := 1; i < len(key); i++ {
                l = key[i]
                if        l >= 'a' && l <= 'z' {
                } else if l >= 'A' && l <= 'Z' {
                } else if l >= '0' && l <= '9' {
                } else {
                    basicKey = false
                }
            }
        }
        if !json && basicKey {
            out += fmt.Sprintf("%s%s:",strings.Repeat("  ",depth), key)
        } else {
            out += fmt.Sprintf("%s\"%s\":",strings.Repeat("  ",depth+1), key)
        }
        out += val.DumpInternalSave( depth+1, json )
        keyNum++
    }
    depth--;
    out += "\n"
    out += strings.Repeat("  ",depth)
    out += "}"
    return out
}

//func ( self *JHash ) ForEach( _ func( JNode ) ) {}
func ( self *JVal ) ForEach( _ func( JNode ) ) {}

func ( self *JArr ) ForEach( handler func( JNode ) ) {
	for _, val := range( self.arr ) {
	    handler( val )
	}
}

func ( self *JVal ) ForEachKeyed( _ func( string, JNode ) ) {}

func ( self *JHash ) ForEachKeyed( handler func( string, JNode ) ) {
  for key, node := range self.hash {
    handler( key, node )
  }
}

func ( self *JArr ) DumpInternal( depth int, json bool ) {
    fmt.Printf("[\n")
    
    i := 0
    maxi := len( self.arr ) - 1
    for _, el := range self.arr {
        fmt.Printf( strings.Repeat("  ",depth) )
        el.DumpInternal( depth, json )
        if i != maxi {
            if json {
                fmt.Printf(",\n")
            } else {
                fmt.Printf("\n")
            }
        }
        i++
    }
    fmt.Printf("\n%s]",strings.Repeat("  ",depth-1))
}

func ( self *JArr ) DumpInternalSave( depth int, json bool ) string {
    out := "[\n"
    
    i := 0
    maxi := len( self.arr ) - 1
    for _, el := range self.arr {
        fmt.Printf( strings.Repeat("  ",depth) )
        out += el.DumpInternalSave( depth, json )
        if i != maxi {
            if json {
                out += ",\n"
            } else {
                out += "\n"
            }
        }
        i++
    }
    out += "\n"
    out += strings.Repeat("  ",depth-1)
    out += "]"
    return out
}

func ( self *JHash ) Dump() { self.DumpInternal( 1, false ) }
func ( self *JArr  ) Dump() { self.DumpInternal( 1, false ) }
func ( self *JVal  ) Dump() { self.DumpInternal( 1, false ) }

func ( self *JHash ) DumpSave() string { return self.DumpInternalSave( 1, false ) }
func ( self *JArr  ) DumpSave() string { return self.DumpInternalSave( 1, false ) }
func ( self *JVal  ) DumpSave() string { return self.DumpInternalSave( 1, false ) }

func ( self *JHash ) Json() { self.DumpInternal( 1, true ) }
func ( self *JArr  ) Json() { self.DumpInternal( 1, true ) }
func ( self *JVal  ) Json() { self.DumpInternal( 1, true ) }

func ( self *JHash ) JsonSave() string { return self.DumpInternalSave( 1, true ) }
func ( self *JArr  ) JsonSave() string { return self.DumpInternalSave( 1, true ) }
func ( self *JVal  ) JsonSave() string { return self.DumpInternalSave( 1, true ) }

func ( self *JVal ) DumpInternal( depth int, json bool ) {
    if self.NodeType == TYPE_STR {
        //out := strings.ReplaceAll( self.str, "\\", "\\\\" )
        out := strings.ReplaceAll( self.str, "\"", "\\\"" )
        fmt.Printf("\"%s\"", out )
    } else if self.NodeType == TYPE_POS { // positive number
        fmt.Printf("%s", self.str )
    } else if self.NodeType == TYPE_NEG { // negative number
        fmt.Printf("-%s", self.str )
    } else if self.NodeType == TYPE_TRUE {
        fmt.Printf("true")
    } else if self.NodeType == TYPE_FALSE {
        fmt.Printf("false")
    } else if self.NodeType == TYPE_NULL {
        fmt.Printf("null")
    }
}

func ( self *JVal ) DumpInternalSave( depth int, json bool ) string {
    if self.NodeType == TYPE_STR {
        //out := strings.ReplaceAll( self.str, "\\", "\\\\" )
        out := strings.ReplaceAll( self.str, "\"", "\\\"" )
        return fmt.Sprintf("\"%s\"", out )
    } else if self.NodeType == TYPE_POS { // positive number
        return self.str
    } else if self.NodeType == TYPE_NEG { // negative number
        return fmt.Sprintf("-%s", self.str )
    } else if self.NodeType == TYPE_TRUE {
        return fmt.Sprintf("true")
    } else if self.NodeType == TYPE_FALSE {
        return fmt.Sprintf("false")
    } else if self.NodeType == TYPE_NULL {
        return fmt.Sprintf("null")
    }
    return ""
}

var handlers map[string] func( []byte, int ) ( int, JNode ) = make( map[string] func( []byte, int ) ( int, JNode ) )

func add_handler( name string, handler func( []byte, int ) ( int, JNode ) ) {
    handlers[ name ] = handler
}

func handle_true( data []byte, pos int ) ( int, JNode ) {
    return 0, &JVal{ NodeType: TYPE_TRUE }
}

func handle_false( data []byte, pos int ) ( int, JNode ) {
    return 0, &JVal{ NodeType: TYPE_FALSE }
}

func handle_null( data []byte, pos int ) ( int, JNode ) {
    return 0, &JVal{ NodeType: TYPE_NULL }
}

func NewBool( val bool ) JNode {
    if val {
        return &JVal{ NodeType: TYPE_TRUE }
    } else {
        return &JVal{ NodeType: TYPE_FALSE }
    }
}

func NewString( str string ) JNode {
    return &JVal{ NodeType: TYPE_STR, str: str }
}

func init() {
    add_handler( "true",  handle_true  )
    add_handler( "false", handle_false )
    add_handler( "null",  handle_null  )
}

type ParseError struct {
    text string
    state string
}

func Parse( data []byte ) ( JNode, []byte ) {
    root, remainder, err := ParseFull( data )
    if err != nil {
        panic( err.text )
    }
    return root, remainder
}

func ParseFull( data []byte ) ( JNode, []byte, *ParseError ) {
    // This error catching exists to avoid having to check data bounds
    //   every place as the data is iterated through. Instead of checking
    //   the bounds I rely on Go to panic when it occurs and then catch
    //   the panic.
    // This isn't entirely clean, because it doesn't provide the knowledge
    //   of at which point ( in the state machine ) the error occurred.
    // Realistically this is silly and a check should just be added for
    //   every access instead of messing with recover and reflection.
    var err *ParseError
    defer func() {
		if rerr := recover(); rerr != nil {
		    typ := reflect.TypeOf(rerr).Name()
		    if typ == "boundsError" {
		        fmt.Fprintf( os.Stdout, "Typ: %s\n", typ )
		        err = &ParseError{ text: "test" }
		    } else { panic(rerr) }
		}
	}()
	
    var remainder []byte
    var dest int
    var node JNode
    var handler func( []byte, int ) ( int, JNode )
    var typeWord string
    size := len( data )
    endi := size - 1
    if Debug { fmt.Printf("Start of Parse; len=%d\n", size ) }
    
    pos := 0
    keyStart := 0
    strStart := 0
    typeStart := 0
    key := ""
    neg := false
    var let byte
    endc := byte('"')
    str := ""
    finalState := ""
    substrStart := 0
    
    nodeType := uint8( 0 )
    //cur = NewJHash( nil )
    var cur JNode
    var root JNode //:= cur
UnknownRoot:
    if pos > endi { finalState="UnknownRoot"; goto OverEnd }
    let = data[pos]
    if Debug { if let != ' ' && let != '\n' { fmt.Printf("UnknownRoot %c %d\n", let, let ) } }
    pos++
    if pos >= size { goto Done } 
    if let == '{' {
        cur = NewJHash( nil )
        root = cur
        nodeType = TYPE_HASH
        goto Hash
    }
    if let == '[' {
        cur = NewJArr( nil )
        root = cur
        nodeType = TYPE_ARR
        goto AfterColon
    }
    if let == '\'' {
        endc = '\''
        cur = NewJHash( nil )
        root = cur
        nodeType = TYPE_HASH
        goto QKeyName1
    }
    if let == '"' {
        endc = '"'
        cur = NewJHash( nil )
        root = cur
        nodeType = TYPE_HASH
        goto QKeyName1
    }
    if let >= 'a' && let <= 'z' || let >= 'A' && let <= 'Z' {
        pos--
        cur = NewJHash( nil )
        root = cur
        nodeType = TYPE_HASH
        goto KeyName1
    }
    if let == '/' && pos < (size-1) {
        if pos > endi { finalState="UnknownRoot+1"; goto OverEnd }
        if data[pos] == '/' {
            pos++
            goto RootHashComment
        }
        if data[pos] == '*' {
            pos++
            goto RootHashComment2
        }
    }
    goto UnknownRoot
Hash:
    if pos > endi { finalState="Hash"; goto OverEnd }
    let = data[pos]
    if Debug { if let != ' ' && let != '\n' { fmt.Printf("Hash %c %d\n", let, let ) } }
    pos++
    if let == '\'' {
        endc = '\''
        goto QKeyName1
    }
    if let == '"' {
        endc = '"'
        goto QKeyName1
    }
    if let >= 'a' && let <= 'z' || let >= 'A' && let <= 'Z' {
        pos--
        goto KeyName1
    }
    if let == '}' {
        parent, parentType := cur.Parent()
        if Debug { fmt.Printf("Ascend to type %d\n", parentType) }
        if parent != nil {
            cur = parent
            nodeType = parentType
            goto HashOrAfterColon
        }
        remainder = data[pos:]
        goto Done
    }
    if let == '/' && pos < (size-1) {
        if pos > endi { goto OverEnd }
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
    if pos > endi { finalState="HashComment"; goto OverEnd }
    let = data[pos]
    pos++
    if let == 0x0d || let == 0x0a {
        goto Hash
    }
    goto HashComment
HashComment2:
    if pos > endi { finalState="HashComment2"; goto OverEnd }
    let = data[pos]
    pos++
    if pos > endi { goto OverEnd }
    if( let == '*' && pos < (size-1) && data[pos] == '/' ) { pos++; goto Hash; }
    goto HashComment2;
RootHashComment:
    if pos > endi { finalState="RootHashComment"; goto OverEnd }
    let = data[pos]
    pos++
    if let == 0x0d || let == 0x0a {
        goto UnknownRoot
    }
    goto RootHashComment
RootHashComment2:
    if pos > endi { finalState="RootHashComment2"; goto OverEnd }
    let = data[pos]
    pos++
    if pos > endi { goto OverEnd }
    if( let == '*' && pos < (size-1) && data[pos] == '/' ) { pos++; goto UnknownRoot; }
    goto RootHashComment2;
QKeyName1:
    keyStart = pos
    pos++
QKeyNameX:
    if pos > endi { finalState="QKeyNameX"; goto OverEnd }
    let = data[pos]
    pos++
    if let == endc {
        key = string( data[ keyStart : pos - 1 ] )
        goto Colon
    }
    goto QKeyNameX
KeyName1:
	//if Debug { fmt.Printf("KeyName1 %c %d\n", let, let ) }
    keyStart = pos
    pos++
KeyNameX:
    if pos > endi { finalState="KeyNameX"; goto OverEnd }
    let = data[pos]
    if Debug { fmt.Printf("KeyNameX %c %d\n", let, let ) }
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
    if pos > endi { finalState="Colon"; goto OverEnd }
    let = data[pos]
    pos++
    if let == ':' {
        goto AfterColon
    }
    goto Colon
AfterColon:
    if pos > endi { finalState="AfterColon"; goto OverEnd }
    let = data[pos]
    if Debug { if let != ' ' && let != '\n' { fmt.Printf("AfterColon %c %d\n", let, let ) } }
    pos++
    if let == '"' {
        endc = '"'
        goto String1
    }
    if let == '\'' {
        endc = '\''
        goto String1
    }
    if let == '`' {
        endc = '`'
        goto String1
    }
    if let == '{' {
        newJNode := NewJHash( cur )
        cur.Add( key, newJNode )
        cur = newJNode
        nodeType = TYPE_HASH
        goto Hash
    }
    if let >= 'a' && let <= 'z' {
        typeStart = pos - 1
        goto TypeX
    }
    if let == '/' && pos < endi {
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
        newArr := NewJArr( cur )
        cur.Add( key, newArr )
        cur = newArr
        nodeType = TYPE_ARR
        goto AfterColon
    }
    if let == ']' {
        cur, nodeType = cur.Parent()
        if cur == nil {
            if pos <= endi {
                remainder = data[pos:]
            }
            goto Done
        }
        goto HashOrAfterColon
    }
    goto AfterColon
TypeX:
    if pos > endi { finalState="TypeX"; goto OverEnd }
    let = data[pos]
    pos++
    if ( let >= '0' && let <= '9' ) || ( let >= 'a' && let <= 'z' ) { goto TypeX }
    pos--
    typeWord = string( data[ typeStart : pos ] )
    handler = handlers[ typeWord ]
    if handler == nil {
        errText := fmt.Sprintf("unknown type '%s'", typeWord )
        return root,remainder,&ParseError{ text:errText }
    }
    dest, node = handler( data, pos )
    if dest == 0 {
        cur.Add( key, node )
        goto HashOrAfterColon
    }
    fmt.Printf("unknown dest\n")
    os.Exit(1)
    goto TypeX
AC_Comment:
    if pos > endi { finalState="AC_Comment"; goto OverEnd }
    let = data[pos]
    pos++
    if let == 0x0d || let == 0x0a {
        goto AfterColon
    }
    goto AC_Comment
AC_Comment2:
    if pos > endi { finalState="AC_Comment2"; goto OverEnd }
    let = data[pos]
    pos++
    if pos > endi { goto OverEnd }
    if let == '*' && pos < (size-1) && data[pos] == '/' {
        pos++
        goto Hash
    }
    goto AC_Comment2
Number1:
    strStart = pos - 1
NumberX:
    if pos > endi { finalState="NumberX"; goto OverEnd }
    let = data[pos]
    pos++
    if let == '.' { goto AfterDot; }
    if let < '0' || let > '9' {
        str := string( data[ strStart : pos - 1 ] )
        newJNode := JVal{ str: str }
        if neg {
            newJNode.NodeType = TYPE_NEG
        } else {
            newJNode.NodeType = TYPE_POS
        }
        cur.Add( key, &newJNode )
        pos-- // so that ] gets recognized
        goto HashOrAfterColon
    }
    goto NumberX;
AfterDot:
    if pos > endi { finalState="AfterDot"; goto OverEnd }
    let = data[pos]
    pos++
    if let < '0' || let > '9' {
        str := string( data[ strStart : pos - 1 ] )
        newJNode := JVal{ str: str }
        if neg {
            newJNode.NodeType = TYPE_FLOAT_NEG
        } else {
            newJNode.NodeType = TYPE_FLOAT_POS
        }
        cur.Add( key, &newJNode )
        pos-- // so that ] gets recognized
        goto HashOrAfterColon
    }
    goto AfterDot;
String1:
    if pos > endi { finalState="String1"; goto OverEnd }
    let = data[pos]
    if Debug { fmt.Printf("String1 %c %d\n", let, let ) }
    str = ""
    substrStart = pos
    pos++
    if let == endc {
        newJNode := JVal{ NodeType: TYPE_STR, str: str } 
        cur.Add( key, &newJNode )
        goto HashOrAfterColon
    }
    if let == '\\' {
        goto Escape
    }
    strStart = pos - 1
StringX:
    if pos > endi { finalState="StringX"; goto OverEnd }
    let = data[pos]
    if Debug { fmt.Printf("StringX %c %d\n", let, let ) }
    pos++
    if let == endc {
        //str := string( data[ strStart : pos - 1 ] )
        str = str + string( data[ substrStart : pos - 1 ] )
        newJNode := &JVal{ NodeType: TYPE_STR, str: str }
        cur.Add( key, newJNode )
        goto HashOrAfterColon
    }
    if let == '\\' {
        goto Escape
    }
    goto StringX
Escape:
    if pos > endi { finalState="Escape"; goto OverEnd }
    let = data[pos]
    if let == endc {
        str = str + string( data[ substrStart : pos - 1 ] ) + string(endc)
        pos++
        substrStart = pos
        goto StringX
    }
    if let == '\\' {
        pos++
        str = str + string( data[ substrStart : pos - 1 ] ) + "\\"
        substrStart = pos
        goto StringX
    }
    pos++
    //str = str + "\\"
    goto StringX
HashOrAfterColon:
    if Debug { fmt.Printf("HashOrAfterColon; type=%d\n", nodeType ) }
    if nodeType == TYPE_HASH { goto Hash }
    //if nodeType == TYPE_ARR {
    goto AfterColon
    //}
//AfterVal:
    // who cares about commas in between things; we can just ignore them :D
    //goto Hash
OverEnd:
    return root, remainder, &ParseError{ text:"JSON Incomplete", state:finalState }
Done:
    return root, remainder, err
}
