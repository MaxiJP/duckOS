#!/bin/bash
export DOWNLOAD_URL="https://ftp.gnu.org/gnu/mpfr/mpfr-4.1.0.tar.xz"
export DOWNLOAD_FILE="mpfr-4.1.0"
export PATCH_FILES=("mpfr.patch")
export USE_CONFIGURE="true"
export CONFIGURE_OPTIONS=("--target=i686-pc-duckos" "--with-sysroot=/")
export DEPENDENCIES=("gmp")
