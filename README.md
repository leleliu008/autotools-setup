# autotools-setup
a command-line tool to setup GNU Autotools and relevant build environment.

following packages will be installed:
- [automake](https://www.gnu.org/software/automake/)
- [autoconf](https://www.gnu.org/software/autoconf/)
- [libtool](https://www.gnu.org/software/libtool/)
- [pkgconf](https://github.com/pkgconf/pkgconf/)
- [perl](https://www.perl.org/)
- [gm4](https://www.gnu.org/software/m4/)
- [gmake](https://www.gnu.org/software/make/)

## dependences
|dependency|required?|purpose|
|----|---------|-------|
|[cmake](https://cmake.org/)|required |for generating `build.ninja`|
|[ninja](https://ninja-build.org/)|required |for doing jobs that read from `build.ninja`|
|[pkg-config>=0.18](https://www.freedesktop.org/wiki/Software/pkg-config/)|required|for finding libraries.|
||||
|[jansson](https://github.com/akheron/jansson)|required|for parsing and creating JSON.|
|[libyaml](https://github.com/yaml/libyaml/)|required|for parsing formulas whose format is YAML.|
|[libcurl](https://curl.se/)|required|for http requesting support.|
|[openssl](https://www.openssl.org/)|required|for https requesting support and SHA-256 sum checking support.|
|[libarchive](https://www.libarchive.org/)|required|for uncompressing .zip and .tar.* files.|
|[zlib](https://www.zlib.net/)|required|for compressing and uncompressing.|
|[pcre2](https://www.pcre.org/)||for Regular Expressions support.<br>only required on OpenBSD.|


## install `autotools-setup` prebuild binary
go to https://github.com/leleliu008/autotools-setup/releases

## build and install autotools-setup via [ppkg](https://github.com/leleliu008/ppkg)
```bash
ppkg install autotools-setup
```

## build and install `autotools-setup` using [vcpkg](https://github.com/microsoft/vcpkg)

**Note:** This is the recommended way to build and install autotools-setup.

```bash
# install g++ curl zip unzip tar git

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg install curl openssl libgit2 libarchive libyaml jansson

cd -

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build   build.d
cmake --install build.d
```

## build and install `autotools-setup` using your system's default package manager

**[Ubuntu](https://ubuntu.com/)**

```bash
apt -y update
apt -y install git cmake ninja-build pkg-config gcc libcurl4 libcurl4-openssl-dev libgit2-dev libarchive-dev libyaml-dev libjansson-dev

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[Fedora](https://getfedora.org/)**

```bash
dnf -y update
dnf -y install git cmake ninja-build pkg-config gcc libcurl-devel libgit2-devel libarchive-devel libyaml-devel jansson-devel

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[ArchLinux](https://archlinux.org/)**

```bash
pacman -Syyuu --noconfirm
pacman -S     --noconfirm git cmake ninja pkg-config gcc curl openssl libgit2 libarchive libyaml jansson

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[AlpineLinux](https://www.alpinelinux.org/)**

```bash
apk add git cmake ninja pkgconf gcc libc-dev curl-dev openssl-dev libgit2-dev libarchive-dev yaml-dev jansson-dev

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[VoidLinux](https://voidlinux.org/)**

```bash
xbps-install -Suy xbps
xbps-install -Suy cmake ninja gcc pkg-config libcurl-devel libgit2-devel libarchive-devel libyaml-devel jansson-devel

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[Gentoo Linux](https://www.gentoo.org/)**

```bash
emerge dev-vcs/git cmake dev-util/ninja gcc pkg-config net-misc/curl dev-libs/libgit2 libarchive dev-libs/libyaml dev-libs/jansson

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[openSUSE](https://www.opensuse.org/)**

```bash
zypper update  -y  
zypper install -y git cmake ninja gcc pkg-config libcurl-devel libgit2-devel libarchive-devel libyaml-devel libjansson-devel

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[macOS](https://www.apple.com/macos/)**

```bash
brew update
brew install git cmake pkg-config ninja curl jansson libyaml libgit2 libarchive

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/opt/openssl@1.1/lib/pkgconfig:/usr/local/opt/curl/lib/pkgconfig:/usr/local/opt/libarchive/lib/pkgconfig"

CMAKE_EXE_LINKER_FLAGS='-L/usr/local/lib -L/usr/local/opt/openssl@1.1/lib -lssl -liconv -framework CoreFoundation -framework Security'
CMAKE_FIND_ROOT_PATH="$(brew --prefix openssl@1.1);$(brew --prefix curl);$(brew --prefix libarchive)"

cmake \
    -S . \
    -B build.d \
    -G Ninja \
    -DCMAKE_INSTALL_PREFIX=./output \
    -DCMAKE_EXE_LINKER_FLAGS="$CMAKE_EXE_LINKER_FLAGS" \
    -DCMAKE_FIND_ROOT_PATH="$CMAKE_FIND_ROOT_PATH"

cmake --build   build.d
cmake --install build.d
```

**[FreeBSD](https://www.freebsd.org/)**

```bash
pkg install -y git cmake ninja pkgconf gcc curl openssl libgit2 libarchive libyaml jansson

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[OpenBSD](https://www.openbsd.org/)**

```bash
pkg_add git cmake ninja pkgconf llvm curl libgit2 libarchive libyaml jansson

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```

**[NetBSD](https://www.netbsd.org/)**

```bash
pkgin -y install git mozilla-rootcerts cmake ninja-build pkg-config clang curl openssl libgit2 libarchive libyaml jansson

mozilla-rootcerts install

git clone https://github.com/leleliu008/autotools-setup
cd autotools-setup

cmake -S . -B   build.d -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build   build.d
cmake --install build.d
```


## ~/.autotools-setup
all relevant dirs and files are located in `~/.autotools-setup` directory.

**Note**: Please do NOT place your own files in `~/.autotools-setup` directory, as `autotools-setup` will change files in `~/.autotools-setup` directory without notice.


## autotools-setup command usage
*   **show help of this command**
        
        autotools-setup -h
        autotools-setup --help
        
*   **show version of this command**

        autotools-setup -V
        autotools-setup --version
        
*   **show your system's information**

        autotools-setup sysinfo

*   **show your system's information and other information**

        autotools-setup env
        
*   **integrate `zsh-completion` script**

        autotools-setup integrate zsh
        autotools-setup integrate zsh --output-dir=/usr/local/share/zsh/site-functions
        autotools-setup integrate zsh -v
        
    I provide a zsh-completion script for `autotools-setup`. when you've typed `autotools-setup` then type `TAB` key, the rest of the arguments will be automatically complete for you.

    **Note**: to apply this feature, you may need to run the command `autoload -U compinit && compinit` in your terminal (your current running shell must be zsh).

*   **setup autotools environment**

        autotools-setup setup
        autotools-setup setup --prefix=.autotools
        autotools-setup setup --prefix=.autotools --jobs=8
        autotools-setup setup --prefix=.autotools -v
 
    **Note**: C and C++ compiler should be installed by yourself using your system's default package manager before running this command.

*   **extra common used utilities**
        
        autotools-setup util zlib-deflate -L 6 < input/file/path
        autotools-setup util zlib-inflate      < input/file/path

        autotools-setup util base16-encode "string to be encoded with base16 algorithm"
        autotools-setup util base16-encode < input/file/path

        autotools-setup util base16-decode ABCD
        autotools-setup util base16-decode ABCD > output/file/path

        autotools-setup util base64-encode "string to be encoded with base64 algorithm"
        autotools-setup util base64-encode < input/file/path

        autotools-setup util base64-decode YQ==
        autotools-setup util base64-decode YQ== > output/file/path

        autotools-setup util sha256sum   input/file/path
        autotools-setup util sha256sum < input/file/path

        autotools-setup util which tree
        autotools-setup util which tree -a

## environment variables

*   **HOME**

    this environment variable already have been set on most systems, if not set or set a empty string, you will receive an error message.

*   **PATH**

    this environment variable already have been set on most systems, if not set or set a empty string, you will receive an error message.

*   **SSL_CERT_FILE**

    ```bash
    curl -LO https://curl.se/ca/cacert.pem
    export SSL_CERT_FILE="$PWD/cacert.pem"
    ```

    In general, you don't need to set this environment variable, but, if you encounter the reporting `the SSL certificate is invalid`, trying to run above commands in your terminal will do the trick.
