# ripple-offline-tool
 
Rippled serialization and transaction signing command-line tool

## Build

If you do not have package `xrpl` in your local Conan cache,
you can add the Ripple remote to download it:

```
conan remote add ripple http://18.143.149.228:8081/artifactory/api/conan/conan-non-prod
```

The build requirements and commands are the exact same as
[those](https://github.com/XRPLF/rippled/blob/develop/BUILD.md) for rippled.
In short:

```
mkdir .build
cd .build
conan install .. --output-folder . --build missing
cmake -DCMAKE_TOOLCHAIN_FILE:FILEPATH=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./ripple-offline-tool --unittest
./ripple-offline-tool
```

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
