#!/usr/bin/env python

import os

# This project expects godot-cpp to be available at ../../godot-cpp
# (shared at the root level as a git submodule)

if not os.path.isdir("../../godot-cpp"):
    print("ERROR: Missing ../../godot-cpp. Ensure godot-cpp is set up at the repository root.")
    Exit(1)

# Workaround for Windows MSVC: If building with MSVC and include paths are empty,
# manually set them from the environment (which should be set by vcvarsall.bat)
if os.name == 'nt':
    include_env = os.environ.get('INCLUDE', '')
    lib_env = os.environ.get('LIB', '')
    if include_env:
        os.environ['INCLUDE'] = include_env
    if lib_env:
        os.environ['LIB'] = lib_env

# Build godot-cpp and reuse its configured env (platform/target/arch/etc).

godot_env = SConscript("../../godot-cpp/SConstruct")

env = godot_env.Clone()

# Optional daScript submodule location.
# Expected layout (as submodule): extensions/dascript/thirdparty/daScript
# or provided via: scons daslang_path=...
daslang_path = ARGUMENTS.get("daslang_path", "thirdparty/daScript")
daslang_abs = os.path.join(os.getcwd(), daslang_path)

has_daslang = os.path.isdir(daslang_abs)
if has_daslang:
    env.AppendUnique(CPPPATH=[
        os.path.join(daslang_path, "include"),
    ])
    env.AppendUnique(CPPDEFINES=["DASCRIPT_HAS_DASLANG=1"])
else:
    env.AppendUnique(CPPDEFINES=["DASCRIPT_HAS_DASLANG=0"])

# Project sources
sources = [
    "src/dascript_language.cpp",
    "src/dascript_script.cpp",
    "src/dascript_instance.cpp",
    "src/dascript_variant_glue.cpp",
    "src/register_types.cpp",
]

env.AppendUnique(CPPPATH=[
    "src",
])

suffix = env.get("suffix", "")
lib_basename = "dascript" + suffix

out_dir = "../../addons/dascript/bin"
if not os.path.isdir(out_dir):
    os.makedirs(out_dir)

if env["platform"] == "windows":
    env["SHLIBPREFIX"] = ""

target_path = os.path.join(out_dir, lib_basename + env["SHLIBSUFFIX"])

lib = env.SharedLibrary(
    target=env.File(target_path),
    source=sources,
)

Default(lib)
