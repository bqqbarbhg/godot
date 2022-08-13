 defmodule Godot.BundlexProject do
   use Bundlex.Project

   def project() do
     [
       natives: natives(Bundlex.platform())
     ]
   end

   def natives(_platform) do
     [
       godot_elixir: [
         sources: ["godot_elixir.cpp"],
         includes: [".", "platform/linuxbsd", "thirdparty/zlib", "main"],
         compiler_flags: ["-DUNIX_ENABLED", "-fpermissive", "-DTOOLS_ENABLED", "-Wno-unused-parameter"],
         interface: [:cnode],
         preprocessor: Unifex,
         lib_dirs: [Path.absname("bin")],
         libs: ["godot.linuxbsd.opt.64"],
         language: :cpp,
         linker_flags: ["-Wl,-rpath=_build/dev/lib/godot/priv/"],
       ]
     ]
   end
 end
