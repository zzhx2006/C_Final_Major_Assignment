set_toolchains("gcc")
set_languages("c23")
add_cflags("-O0","-g3","-ggdb","-Wall","-Wextra","-mtune=native","-march=native","-pedantic","-fanalyzer", "-Wpedantic")
set_optimize("none")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })

-- target("client")
--     set_kind("binary")
--     add_includedirs(include)
--     add_files("src/client.c")
    
target("server")
    set_kind("binary")
    add_includedirs(include)
    add_files("src/server.c")
    