package main

import (
    "fmt"
    "io/ioutil"
    "os"
    "time"
    "strconv"
    uc "github.com/nanoscopic/uclop/v2"
    uj "github.com/dryark/ujsonin/go"
)

func readFile( path string ) (uj.JNode, *uj.ParseError) {
    _, err := os.Stat( path )
    if err != nil {
        panic("Cannot read [" + path + "]")
    }
    
    content, _ := ioutil.ReadFile(path)
    root, _, errp := uj.ParseFull( content )
    return root, errp
}

func runGet( cmd *uc.Cmd ) {
    root, _ := readFile( cmd.Get("-file").String() )
    node := root.Get( cmd.Get("-path").String() )
    fmt.Println( node.String() )
}

func runSet( cmd *uc.Cmd ) {
    filePath := cmd.Get("-file").String()
    _, err := os.Stat( filePath )
    var root uj.JNode
    if err != nil {
        root = uj.NewJHash(nil)
    } else {
        root, _ = readFile( filePath )
    }
    
    strNode := uj.NewString( cmd.Get("-val").String() )
    root.Add( cmd.Get("-path").String(), strNode )
    content := root.DumpSave()
    
    ioutil.WriteFile( filePath, []byte(content), 0644 )
}

func makevarsRecurse( res *string, name string, node uj.JNode ) {
    nType := node.Type()
    switch nType {
    case uj.TYPE_HASH:
        node.ForEachKeyed( func( key string, node2 uj.JNode ) {
            makevarsRecurse( res, name + "_" + key, node2 )
        } )
        break
    case uj.TYPE_ARR:
        i := 1
        node.ForEach( func( node2 uj.JNode ) {
            makevarsRecurse( res, name + "_" + strconv.Itoa(i), node2 )
            i++
        } )
        *res = *res + fmt.Sprintf( "%s_cnt := \"%d\"\n", name, i-1 )        
        break
    default:
        *res = *res + fmt.Sprintf( "%s := \"%s\"\n", name, node.String() )
        break
    }
}

func writeErr( prefix string, cmd *uc.Cmd, err string, srcPath string ) {
    outErrNode := cmd.Get("-outfile")
    if outErrNode != nil {
        outErrPath := outErrNode.String()
        errs := fmt.Sprintf("%s_jsonfail := 1\n", prefix)
        errs = errs + fmt.Sprintf("%s_jsonerr := %s\n", prefix, err )
        
        ioutil.WriteFile( outErrPath, []byte( errs ), 0644 )
        
        srcFile, _ := os.Stat( srcPath )
        srcMod := srcFile.ModTime()
        
        srcMod = srcMod.Add( -time.Second )
        os.Chtimes( outErrPath, srcMod, srcMod )
    }
}

func runMakevars( cmd *uc.Cmd ) {
    var root uj.JNode
    var err *uj.ParseError
    defNode := cmd.Get("-defaults")
    filePath := cmd.Get("-file").String()
    prefix := cmd.Get("-prefix").String()
    if defNode != nil && defNode.String() != "" {
        defPath := defNode.String()
        root, err = readFile( defPath )
        if err != nil {
            e := fmt.Sprintf( "Could not parse %s properly as JSON\n", defPath )
            writeErr( prefix, cmd, e, defPath )
            return
        }
        ontop, err := readFile( filePath )
        if err != nil {
            e := fmt.Sprintf( "Could not parse %s properly as JSON\n", filePath )
            writeErr( prefix, cmd, e, filePath )
            return
        }
        root.Overlay( ontop )        
    } else {
        root, err = readFile( filePath )
        if err != nil {
            e := fmt.Sprintf( "Could not parse %s properly as JSON\n", filePath )
            writeErr( prefix, cmd, e, filePath )
            return
        }
    }
        
    res := ""
    makevarsRecurse( &res, prefix, root )
    res = res + fmt.Sprintf("%s_jsonfail := 0\n", prefix)
    
    outFileNode := cmd.Get("-outfile")
    if outFileNode != nil {
        outFilePath := outFileNode.String()
        ioutil.WriteFile( outFilePath, []byte( res ), 0644 )
    } else {
        fmt.Print( res )
    }
}

func runOverlay( cmd *uc.Cmd ) {
    root,_ := readFile( cmd.Get("-file1").String() )
    ontop,_ := readFile( cmd.Get("-file2").String() )
    root.Overlay( ontop )
    json := cmd.Get("-json").Bool()
    if json {
        root.Json()
    } else {
        root.Dump()
    }
}

func runPretty( cmd *uc.Cmd ) {
    root,_ := readFile( cmd.Get("-file").String() )
    json := cmd.Get("-json").Bool()
    if json {
        root.Json()
    } else {
        root.Dump()
        //fmt.Printf("%s", root.DumpSave())
    }
}

func main() {
    uclop := uc.NewUclop()
    uclop.AddCmd( "get", "Get value from file", runGet, uc.OPTS{
        uc.OPT("-file","JSON file",uc.REQ),
        uc.OPT("-path","Path in JSON to value",uc.REQ),
    } )
    uclop.AddCmd( "set", "Set value in JSON file", runSet, uc.OPTS{
        uc.OPT("-file","JSON file",uc.REQ),
        uc.OPT("-path","Path in JSON to set",uc.REQ),
        uc.OPT("-val","Value to set to",uc.REQ),
    } )
    uclop.AddCmd( "makevars", "Turn JSON into include file for Makefiles", runMakevars, uc.OPTS{
        uc.OPT("-prefix","Prefix for vars",uc.REQ),
        uc.OPT("-file","JSON file",uc.REQ),
        uc.OPT("-defaults","JSON file with defaults",0),
        uc.OPT("-outfile","Makefile include to write to",0),
        uc.OPT("-outerr","Makefile include to write error information to",0),
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
