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
# Expected layout (as submodule): extensions/dascript/lib/daScript
# or provided via: scons daslang_path=...
daslang_path = ARGUMENTS.get("daslang_path", "lib/daScript")
daslang_abs = os.path.join(os.getcwd(), daslang_path)

has_daslang = os.path.isdir(daslang_abs)
if has_daslang:
    env.AppendUnique(CPPPATH=[
        os.path.join(daslang_path, "include"),
        os.path.join(daslang_path, "3rdparty", "fmt", "include"),
        os.path.join(daslang_path, "3rdparty", "uriparser", "include"),
        os.path.join(daslang_path, "include", "modules"),
    ])
    env.AppendUnique(CPPDEFINES=[
        "DASCRIPT_HAS_DASLANG=1",
        # Keep daScript as a static-in-the-extension library.
        "DAS_ENABLE_DLL=0",
        # uriparser is built into the extension
        "URI_STATIC_BUILD",
        "URIPARSER_BUILD_CHAR",
    ])
    # daScript has some very large TU's on MSVC.
    if not env.get("use_mingw", False):
        env.AppendUnique(CCFLAGS=["/bigobj", "/EHsc"])
else:
    env.AppendUnique(CPPDEFINES=["DASCRIPT_HAS_DASLANG=0"])

# Project sources
sources = [
    "src/dascript_language.cpp",
    "src/dascript_script.cpp",
    "src/dascript_instance.cpp",
    "src/dascript_variant_glue.cpp",
    "src/dascript_resource_loader.cpp",
    "src/register_types.cpp",
]

if has_daslang:
    # Build daScript directly into this extension (minimal: core + default modules).
    # We avoid optional external modules and tools.
    def _glob(rel):
        return [str(x) for x in Glob(os.path.join(daslang_path, rel))]

    da_sources = []
    # Parser (already generated .cpp/.hpp are committed in the submodule)
    da_sources += _glob(os.path.join("src", "parser", "*.cpp"))
    # AST
    da_sources += _glob(os.path.join("src", "ast", "*.cpp"))
    # Builtin modules required by daScript C API default init
    da_sources += _glob(os.path.join("src", "builtin", "*.cpp"))
    # Misc + runtime glue
    da_sources += _glob(os.path.join("src", "misc", "*.cpp"))
    # HAL
    da_sources += _glob(os.path.join("src", "hal", "*.cpp"))
    # Simulator/runtime
    da_sources += _glob(os.path.join("src", "simulate", "*.cpp"))
    # Formatter helpers (used by daScript core)
    da_sources += _glob(os.path.join("utils", "dasFormatter", "fmt.cpp"))
    da_sources += _glob(os.path.join("utils", "dasFormatter", "formatter.cpp"))
    da_sources += _glob(os.path.join("utils", "dasFormatter", "helpers.cpp"))
    da_sources += _glob(os.path.join("utils", "dasFormatter", "ds_parser.cpp"))

    # Exclude optional pieces that pull in extra third-party deps we don't ship.
    # (aot_stub pulls in generated AOT symbols which aren't part of this vendored snapshot).
    def _exclude(path):
        bn = os.path.basename(path).lower()
        return bn in [
            "aot_stub.cpp",
        ]

    da_sources = [p for p in da_sources if not _exclude(p)]

    # uriparser C sources (required by daScript core misc/uric.cpp)
    uriparser_sources = _glob(os.path.join("3rdparty", "uriparser", "src", "*.c"))
    sources += uriparser_sources

    sources += da_sources

env.AppendUnique(CPPPATH=[
    "src",
])

suffix = env.get("suffix", "")
lib_basename = "dascript" + suffix

out_dir = "../../godot_project/addons/dascript/bin"
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
