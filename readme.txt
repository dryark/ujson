This "JSON" parser parses a subset of JSON.

It parses "JSON" with the following features:
* Double quoted string keys ( unquoted keys are not yet allowed )
* Double quoted string values
* Inset hashes
* C styles comments ( both //comment and /*comment*/ )

The following JSON features are not supported currently:
* Booleans ( true/false )
* Numbers
* Arrays
* Escaped double quotes within either keys or values

The following additional "features" exist:
* Commas between things can be added or left out; they have no effect
* The contents of quoted keys/values are entirely untouched. They are not processed.
* Garbage between things is ignored. For example, single line comments without //
  don't affect parsing so long as they don't contain any double quotes.
* Null characters have no special meaning to the parser and are ignored.
* String values can contain carriage returns
* Key "case" does matter

Known bugs / deficiencies:
* There is no json__delete yet to clean up a parsed structure when no longer needed
* Functions aren't prefixed with anything like ujson, nor are any kept private, so
  all of them essentially taint your global namespace and could interfere with other
  functions.
* node_hash__get and node_hash__store could be avoided entirely. They've been left
  in because it is cleaner. They should be optimized away automatically during
  compile anyway. Same with node_hash__new and node_str__new for that matter.
* Error is currently never set to anything. There used to be a single error, but it
  wasn't needed and was pointless so I removed it. If you feed the parser garbage,
  you'll just get garbage back. -shrug-
* No malloc / calloc memory allocations are checked to see if they return null. If
  you run out of memory the program will just crash when it tries to use a null
  pointer. -shrug-
* JSON longer than 'int' size ( of your compiler ) isn't currently supported. If you
  are running on a 16-bit platform, this means 32k limit to JSON on those platforms.
  It depends on both your platform and your compiler really.