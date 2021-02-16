package main

import (
    "fmt"
    "io/ioutil"
    "os"
    "strconv"
    uc "github.com/nanoscopic/uclop/mod"
    uj "github.com/nanoscopic/ujsonin/mod"
)

func readFile( path string ) *uj.JNode {
    _, err := os.Stat( path )
    if err != nil {
        panic("Cannot read [" + path + "]")
    }
    
    content, _ := ioutil.ReadFile(path)
    root, _ := uj.Parse( content )
    return root
}

func runGet( cmd *uc.Cmd ) {
    root := readFile( cmd.Get("-file").String() )
    node := root.Get( cmd.Get("-path").String() )
    fmt.Println( node.String() )
}

func makevarsRecurse( name string, node *uj.JNode ) {
    nType := node.NodeType
    switch nType {
    case uj.TYPE_HASH:
        node.ForEachKeyed( func( key string, node2 *uj.JNode ) {
            makevarsRecurse( name + "_" + key, node2 )
        } )
        break
    case uj.TYPE_ARR:
        i := 1
        node.ForEach( func( node2 *uj.JNode ) {
            makevarsRecurse( name + "_" + strconv.Itoa(i), node2 )
            i++
        } )
        fmt.Printf( "%s_cnt := \"%d\"\n", name, i-1 )        
        break
    default:
        fmt.Printf( "%s := \"%s\"\n", name, node.String() )
        break
    }
}

func runMakevars( cmd *uc.Cmd ) {
    var root *uj.JNode
    defNode := cmd.Get("-defaults")
    filePath := cmd.Get("-file").String()
    if defNode != nil && defNode.String() != "" {
        defPath := defNode.String()
        root = readFile( defPath )
        ontop := readFile( filePath )
        root.Overlay( ontop )        
    } else {
        root = readFile( filePath )
    }
    prefix := cmd.Get("-prefix").String()
    
    makevarsRecurse( prefix, root )
}

func runOverlay( cmd *uc.Cmd ) {
    root := readFile( cmd.Get("-file1").String() )
    ontop := readFile( cmd.Get("-file2").String() )
    root.Overlay( ontop )
    json := cmd.Get("-json").Bool()
    if json {
        root.Json()
    } else {
        root.Dump()
    }
}

func runPretty( cmd *uc.Cmd ) {
    root := readFile( cmd.Get("-file").String() )
    json := cmd.Get("-json").Bool()
    if json {
        root.Json()
    } else {
        root.Dump()
    }
}

func main() {
    uclop := uc.NewUclop()
    uclop.AddCmd( "get", "Get value from file", runGet, uc.OPTS{
        uc.OPT("-file","JSON file",uc.REQ),
        uc.OPT("-path","Path in JSON to value",uc.REQ),
    } )
    uclop.AddCmd( "makevars", "Turn JSON into include file for Makefiles", runMakevars, uc.OPTS{
        uc.OPT("-prefix","Prefix for vars",uc.REQ),
        uc.OPT("-file","JSON file",uc.REQ),
        uc.OPT("-defaults","JSON file with defaults",0),
    } )
    uclop.AddCmd( "overlay", "Overlay one JSON file over another", runOverlay, uc.OPTS{
        uc.OPT("-file1","Base JSON file",uc.REQ),
        uc.OPT("-file2","JSON file to overlay on top",uc.REQ),
        uc.OPT("-json","Print in \"pure\" JSON format",uc.FLAG),
    } )
    uclop.AddCmd( "pretty", "Pretty print JSON", runPretty, uc.OPTS{
        uc.OPT("-file","JSON file",0),
        uc.OPT("-json","Print in \"pure\" JSON format",uc.FLAG),
    } )
    uclop.Run()
}
