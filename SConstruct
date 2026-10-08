#!/usr/bin/env python
import os
import sys

# A plain `git clone` leaves godot-cpp/ an empty directory, because it is a
# submodule: the tree is a gitlink, not its contents. `SConscript` then reports
# `missing SConscript file 'godot-cpp/SConstruct'`, which reads like a broken
# checkout rather than the one command that fixes it. Say so here instead.
if not os.path.exists(os.path.join("godot-cpp", "SConstruct")):
    print("godot-cpp/ is not populated - it is a git submodule, so a clone gets")
    print("an empty directory until it is initialised. Run:\n")
    print("    git submodule update --init --recursive\n")
    print("and build again.", file=sys.stderr)
    Exit(1)

env = SConscript("godot-cpp/SConstruct")
env.Append(CPPPATH=["src/"])

sources = Glob("src/*.cpp")
library = env.SharedLibrary(
    "bin/libinfantry{}{}".format(env["suffix"], env["SHLIBSUFFIX"]), source=sources
)
Default(library)