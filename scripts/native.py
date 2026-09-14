import sys

Import("env")

# Apple Clang's coverage files must be read with llvm-cov gcov.
if sys.platform == "darwin":
    env.Replace(CC="clang", CXX="clang++")
