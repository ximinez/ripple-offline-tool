# ripple-offline-tool
 
[![Build Status](https://travis-ci.org/ximinez/ripple-offline-tool.svg?branch=master)](https://travis-ci.org/ximinez/ripple-offline-tool)
[![Build status](https://ci.appveyor.com/api/projects/status/ypsy8txb79ppe4g0?svg=true)](https://ci.appveyor.com/project/ximinez/ripple-offline-tool)
[![codecov](https://codecov.io/gh/ximinez/ripple-offline-tool/branch/master/graph/badge.svg)](https://codecov.io/gh/ximinez/ripple-offline-tool)

Rippled serialization and transaction signing command-line tool

## Table of contents

* [Dependencies](#dependencies)
  * [rippled inclusion](#rippled-inclusion)
  * [Other dependencies](#other-dependencies)
* [Build and run](#build-and-run)
* [Usage](#guide)
  * [Key File Format](#key-file-format)

## Dependencies

### rippled inclusion

This project depends on the [rippled](https://github.com/ripple/rippled.git)
repository for core signing functionality. If you have built and installed
rippled, you can point this project at your installation using
`CMAKE_PREFIX_PATH` (if you have installed in a standard system search path,
this is not needed), e.g.:

```
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/rippled/installation/root ../..
```

Alternatively, if you do not have a local installation of rippled development
files that you want to use, then this project will fetch an appropriate
version of the source code using CMake's FetchContent.


### Other dependencies

* C++14 or greater
* [Boost](http://www.boost.org/) - 1.70+ required
* [OpenSSL](https://www.openssl.org/)
* [cmake](https://cmake.org) - 3.11+ required

## Build and run

For linux and other unix-like OSes, run the following commands:

```
$ cd ${YOUR_RIPPLE_SERIALIZE_DIRECTORY}
$ mkdir -p build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --parallel
$ ./ripple-offline-tool --unittest
$ ./ripple-offline-tool --help
```

For 64-bit Windows, open a MSBuild Command Prompt for Visual Studio
and run the following commands:

```
> cd %YOUR_RIPPLE_SERIALIZE_DIRECTORY%
> mkdir build
> cd build
> cmake -G"Visual Studio 15 2017 Win64" ..
> cmake --build . --config Release --parallel
> .\Release\ripple-offline-tool.exe --unittest
> .\Release\ripple-offline-tool.exe --help
```

32-bit Windows builds are not officially supported.

# Usage

Run `ripple-offline-tool --help` for usage information.

## Key File Format

The key file contains one JSON object. That object has a series of string
name/value pairs. The only required fields are `key_type` and
`master_seed`. Other fields are derived internally as needed. This
simplifies manual keyfile creation for existing XRPL accounts. Example:

```
{
      "key_type" : "ed25519",
      "master_seed" : "sPUTYOURSECRETKEYHERE"
}
```

For user convenience, the `createkeyfile` operation will write a new keyfile
containing the same fields returned by `rippled`'s `wallet_propose` RPC command
plus `secret_key` and `secret_key_hex`. While not needed for signing operations,
this allows the user to easily retrieve or confirm their `account_id` for later
use. It also removes the risk of allowing a potentially untrusted server to
generate a secret key.
