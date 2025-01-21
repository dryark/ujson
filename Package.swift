// swift-tools-version:5.3
import PackageDescription

let package = Package(
    name: "ujson",
    products: [
        // Products define the executables and libraries a package produces, and make them visible to other packages.
        .library(
            name: "ujson",
            type: .static,
            targets: ["ujson","stringtree","redblacktree","sds"]
        ),
    ],
    dependencies: [],
    targets: [
        // Targets are the basic building blocks of a package. A target can define a module or a test suite.
        // Targets can depend on other targets in this package, and on products in packages this package depends on.
        .target(
            name: "ujson",
            dependencies: [],//["stringtree","redblacktree","sds"],
            path: "c",
            exclude: ["test.c"],
            sources: ["ujson.c"],
            publicHeadersPath: "."
        ),
        
        .target(
            name: "stringtree",
            dependencies: [],
            path: "c",
            sources: ["string-tree.c"]
        ),
        .target(
            name: "redblacktree",
            dependencies: [],
            path: "c",
            sources: ["red_black_tree.c"]
        ),
        .target(
            name: "sds",
            dependencies: [],
            path: "c",
            sources: ["sds.c"]
        )
        //.testTarget(
        //    name: "YourLibraryNameTests",
        //    dependencies: ["YourLibraryName"]),
    ]
)
