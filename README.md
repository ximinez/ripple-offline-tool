# ripple-offline-tool
 
[![CircleCI](https://circleci.com/gh/ximinez/ripple-offline-tool.svg?style=svg)](https://circleci.com/gh/ximinez/ripple-offline-tool)
[![Build Status](https://travis-ci.org/ximinez/ripple-offline-tool.svg?branch=master)](https://travis-ci.org/ximinez/ripple-offline-tool)
[![Build status](https://ci.appveyor.com/api/projects/status/ypsy8txb79ppe4g0?svg=true)](https://ci.appveyor.com/project/ximinez/ripple-offline-tool)
[![codecov](https://codecov.io/gh/ximinez/ripple-offline-tool/branch/master/graph/badge.svg)](https://codecov.io/gh/ximinez/ripple-offline-tool)

Rippled serialization and transaction signing command-line tool

## Table of contents

* [Dependencies](#dependencies)
  * [ripple-libpp submodule](#ripple-libpp-submodule)
  * [Other dependencies](#other-dependencies)
* [Build and run](#build-and-run)
* [Usage](#guide)
  * [Key File Format](#key-file-format)

## Dependencies

### ripple-libpp submodule

This includes a git submodule to the ripple-libpp source code, which is not cloned by default. To get the ripple-libpp source, either clone this repository using
```
$ git clone --recursive <location>
```
or after cloning, run the following commands
```
$ git submodule update --init --recursive
```

### Other dependencies

* C++14 or greater
* [Boost](http://www.boost.org/)
* [OpenSSL](https://www.openssl.org/)
* [cmake](https://cmake.org)

## Build and run

For linux and other unix-like OSes, run the following commands:

```
$ cd ${YOUR_RIPPLE_SERIALIZE_DIRECTORY}
$ git submodule update --init --recursive  # if you haven't already
$ mkdir -p build/gcc.debug
$ cd build/gcc.debug
$ cmake ../..
$ cmake --build .
$ ./ripple-offline-tool --unittest
$ ./ripple-offline-tool --help
```

For 64-bit Windows, open a MSBuild Command Prompt for Visual Studio
and run the following commands:

```
> cd %YOUR_RIPPLE_SERIALIZE_DIRECTORY%
> git submodule update --init --recursive  # if you haven't already
> mkdir build
> cd build
> cmake -G"Visual Studio 14 2015 Win64" ..
> cmake --build .
> .\Debug\ripple-offline-tool.exe --unittest
> .\Debug\ripple-offline-tool.exe --help
```

32-bit Windows builds are not officially supported.

# Usage

Run `ripple-offline-tool --help` for usage information.

## Key File Format

The key file contains one JSON object. That object has a series of string
name/value pairs. The only required fields are `key_type` and
`master_seed`. Other fields are derived internally as needed. This
simplifies manual keyfile creation for existing ripple accounts. Example:

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
