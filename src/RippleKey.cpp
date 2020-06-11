//------------------------------------------------------------------------------
/*
This file is part of ripple-offline-tool:
https://github.com/ximinez/ripple-offline-tool
Copyright (c) 2017 Ripple Labs Inc.

Permission to use, copy, modify, and/or distribute this software for any
purpose  with  or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <RippleKey.h>
#include <ripple/basics/strHex.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/jss.h>
#include <ripple/protocol/Sign.h>
#include <boost/filesystem.hpp>
#include <fstream>

namespace offline {

RippleKey::RippleKey(ripple::KeyType const& keyType,
    ripple::Seed const& seed)
    : keyType_(keyType)
    , seed_(seed)
{
    using namespace ripple;
    std::tie(publicKey_, secretKey_) = generateKeyPair(
        keyType_, seed_);
}

RippleKey
RippleKey::make_RippleKey(boost::optional<ripple::KeyType> const& keyType,
    boost::optional<std::string> const& rawseed)
{
    if (keyType && rawseed)
    {
        auto const seed = ripple::parseGenericSeed(*rawseed);

        if (!seed)
            throw std::runtime_error(
                "Unable to parse seed: " + *rawseed);

        return RippleKey{ *keyType, *seed };
    }
    if (rawseed)
        return RippleKey::make_RippleKey(RippleKey::defaultKeyType(),
            *rawseed);
    if (keyType)
        return RippleKey{ *keyType };
    return RippleKey{};
}

RippleKey
RippleKey::make_RippleKey(
    boost::filesystem::path const& keyFile)
{
    using namespace ripple;

    std::ifstream ifsKeys (keyFile.string (), std::ios::in);

    if (! ifsKeys)
        throw std::runtime_error (
            "Failed to open key file: " + keyFile.string());

    Json::Reader reader;
    Json::Value jKeys;
    if (! reader.parse (ifsKeys, jKeys))
    {
        throw std::runtime_error (
            "Unable to parse json key file: " + keyFile.string());
    }

    static std::array<Json::StaticString, 2> const requiredFields {{
        jss::key_type,
        jss::master_seed
    }};

    for (auto field : requiredFields)
    {
        if (! jKeys.isMember(field))
        {
            throw std::runtime_error (
                std::string{ "Field '" } + field.c_str() +
                "' is missing from key file: " +
                keyFile.string());
        }
    }

    auto const keyType = keyTypeFromString (jKeys[jss::key_type].asString());
    if (!keyType)
    {
        throw std::runtime_error (
            "Invalid 'key_type' field \"" +
            jKeys[jss::key_type].asString() +
            "\" found in key file: " +
            keyFile.string());
    }

    return RippleKey::make_RippleKey (
        *keyType, jKeys[jss::master_seed].asString());
}

void
RippleKey::writeToFile(boost::filesystem::path const& keyFile) const
{
    using namespace ripple;
    using namespace boost::filesystem;

    Json::Value jv(Json::objectValue);
    jv[jss::key_type] = to_string(keyType_);
    jv[jss::master_seed] = toBase58(seed_);
    jv[jss::master_seed_hex] = strHex(seed_);
    jv[jss::master_key] = seedAs1751(seed_);
    jv[jss::account_id] = toBase58(calcAccountID(publicKey_));
    jv[jss::public_key] = toBase58(TokenType::AccountPublic, publicKey_);
    jv[jss::public_key_hex] = strHex(publicKey_);
    jv["secret_key"] = toBase58(TokenType::AccountSecret, secretKey_);
    jv["secret_key_hex"] = strHex(secretKey_);

    if (!keyFile.parent_path().empty())
    {
        boost::system::error_code ec;
        if (!exists(keyFile.parent_path()))
            boost::filesystem::create_directories(keyFile.parent_path(), ec);

        if (ec || !is_directory(keyFile.parent_path()))
            throw std::runtime_error("Cannot create directory: " +
                keyFile.parent_path().string());
    }

    std::ofstream o(keyFile.string(), std::ios::trunc);
    if (o.fail())
        throw std::runtime_error("Cannot open key file: " +
            keyFile.string());

    o << jv.toStyledString();
}

void
RippleKey::singleSign(boost::optional<ripple::STTx>& tx) const
{
    if (! tx)
    {
        throw std::runtime_error ("Internal error.  "
            "Empty std::optional passed to RippleKey::singleSign.");
    }
    using namespace ripple;
    tx->setFieldVL(sfSigningPubKey, publicKey_.slice());
    tx->makeFieldAbsent(sfSigners);
    tx->sign(publicKey_, secretKey_);
}

void
RippleKey::multiSign(boost::optional<ripple::STTx>& tx) const
{
    if (! tx)
    {
        throw std::runtime_error ("Internal error.  "
            "Empty std::optional passed to RippleKey::multiSign.");
    }
    using namespace ripple;
    tx->setFieldVL(sfSigningPubKey, Slice{ nullptr, 0 });
    tx->makeFieldAbsent(sfTxnSignature);

    auto const accountID = calcAccountID(publicKey_);
    Serializer s1 = buildMultiSigningData(*tx, accountID);

    auto const multisig = ripple::sign(
        publicKey_, secretKey_, s1.slice());

    // Build an entry for this signer
    STObject signer(sfSigner);
    signer[sfAccount] = accountID;
    signer[sfSigningPubKey] = publicKey_;
    signer[sfTxnSignature] = multisig;

    // Insert the signer into the array of signers
    if (!tx->isFieldPresent(sfSigners))
        tx->setFieldArray(sfSigners, {});
    STArray& signers{ tx->peekFieldArray(sfSigners) };
    signers.emplace_back(std::move(signer));

    // Sort the Signers array by Account.  If it is not sorted when submitted
    // to the network then it will be rejected.
    std::sort (signers.begin(), signers.end(),
        [](STObject const& a, STObject const& b)
    {
        return (a[sfAccount] < b[sfAccount]);
    });

    // Re-serialize this signed and sorted STTx so the hash is freshly computed.
    Serializer s2;
    tx->add (s2);
    Blob txBlob = s2.getData ();
    SerialIter sit {makeSlice(txBlob)};
    tx.emplace (sit);
}

}   // serialize
