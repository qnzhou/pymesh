#!/usr/bin/env python

""" Build and install third party dependencies for PyMesh.
"""

import argparse
import subprocess
import os
import re
import os.path
import tempfile
import shutil
import sys

def get_third_party_dependencies():
    return ["cgal", "cork", "eigen",
        "tetgen", "triangle", "qhull", "clipper", "draco",
        "tbb", "mmg", "json"]

def parse_args():
    parser = argparse.ArgumentParser(__doc__);
    parser.add_argument("--cleanup", action="store_true",
            help="Clean up the build folder after done.");
    parser.add_argument("package",
            choices=["all"] + get_third_party_dependencies());
    return parser.parse_args();

def get_pymesh_dir():
    p = os.path.normpath(os.path.join(sys.path[0], ".."))
    return re.sub(r'\\', '/', p)


def hack_cmake_install(fn, instdir):
    with open(fn) as f:
        old = f.read()
    prefix = f"""if (BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_INSTALL_PREFIX "{instdir}/debug")
else()
    set(CMAKE_INSTALL_PREFIX "{instdir}")
endif()"""
    with open(fn, 'w') as fo:
        fo.write(prefix + "\n" + old)


def build_generic(libname, build_flags="", cleanup=True):
    pymesh_dir = get_pymesh_dir();
    build_dir = os.path.join(pymesh_dir, "third_party", "build", libname);
    if not os.path.exists(build_dir):
        os.makedirs(build_dir);

    # Configure cgal
    cmd = "cmake" + \
            " {}/third_party/{}".format(pymesh_dir, libname) + \
            " -DBUILD_SHARED_LIBS=Off" + \
            " -DCMAKE_POSITION_INDEPENDENT_CODE=On" + \
            build_flags + \
            " -DCMAKE_INSTALL_PREFIX={}/python/pymesh/third_party/".format(pymesh_dir);
    subprocess.check_call(cmd.split(), cwd=build_dir);
    # log cmd for debugging build process
    with open(f"{build_dir}/cmd.txt", "w") as f:
        print(cmd, file=f)

    # Build cgal
    if os.name == "nt":
        hack_cmake_install(f"{build_dir}/cmake_install.cmake", f"{pymesh_dir}/python/pymesh/third_party")
        os.environ['_CL_']='/MDd'
        cmd = f"cmake --build {build_dir} --config Debug"
        subprocess.check_call(cmd.split())
        os.environ['_CL_']='/MD'
        cmd = f"cmake --build {build_dir} --config Release"
        subprocess.check_call(cmd.split())
        os.environ['_CL_']=''
        cmd = f"cmake --build {build_dir} --config Debug --target install"
        subprocess.check_call(cmd.split())
        cmd = f"cmake --build {build_dir} --config Release --target install"
        subprocess.check_call(cmd.split())
    else:
        cmd = "cmake --build {}".format(build_dir);
        subprocess.check_call(cmd.split());
        cmd = "cmake --build {} --target install".format(build_dir);
        subprocess.check_call(cmd.split());

    # Clean up
    if cleanup:
        shutil.rmtree(build_dir)

def build(package, cleanup):
    if package == "all":
        for libname in get_third_party_dependencies():
            build(libname, cleanup);
    elif package == "cgal":
        build_generic("cgal",
                " -DWITH_CGAL_ImageIO=Off -DWITH_CGAL_Qt5=Off",
                cleanup=cleanup);
    elif package == "clipper":
        build_generic("Clipper/cpp", cleanup=cleanup);
    elif package == "tbb":
        build_generic("tbb",
                " -DTBB_BUILD_SHARED=On -DTBB_BUILD_STATIC=Off",
                cleanup=cleanup);
    elif package == "json":
        build_generic("json",
                " -DJSON_BuildTests=Off",
                cleanup=cleanup);
    else:
        build_generic(package, cleanup=cleanup);

def main():
    args = parse_args();
    build(args.package, args.cleanup);

if __name__ == "__main__":
    main();
