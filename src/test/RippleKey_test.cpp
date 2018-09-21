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
#include <Serialize.h>
#include <test/KnownTestData.h>
#include <test/KeyFileGuard.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/unit_test.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/JsonFields.h>

namespace offline {

namespace test {

class RippleKey_test : public beast::unit_test::suite
{
private:
    std::string const passphrase = "masterpassphrase";

    void
    testRandom(ripple::KeyType const kt)
    {
        using namespace ripple;

        testcase("Random key");
        RippleKey const key1(kt);
        RippleKey const key2(kt);
        // Not much you can check with a random key
        BEAST_EXPECT(key1.keyType() == kt);
        BEAST_EXPECT(key2.keyType() == kt);
        BEAST_EXPECT(key1.publicKey() != key2.publicKey());
        auto const pubkey1 = toBase58(TokenType::AccountPublic, key1.publicKey());
        auto const pubkey2 = toBase58(TokenType::AccountPublic, key2.publicKey());
        BEAST_EXPECT(pubkey1.length() == 52);
        BEAST_EXPECT(pubkey2.length() == 52);
        BEAST_EXPECT(pubkey1 != pubkey2);
    }

    void
    testSeed(ripple::KeyType const kt)
    {
        using namespace ripple;

        testcase("Known seed");

        // Cases to check: string passphrase, string seed, and Seed
        auto const seed = generateSeed(passphrase);

        RippleKey const key(kt, seed);
        auto const pubkey = toBase58(TokenType::AccountPublic, key.publicKey());
        BEAST_EXPECT(key.keyType() == kt);
        BEAST_EXPECT(pubkey == (kt == KeyType::secp256k1 ?
            "aBQG8RQAzjs1eTKFEAQXr2gS4utcDiEC9wmi7pfUPTi27VCahwgw" :
            "aKGheSBjmCsKJVuLNKRAKpZXT6wpk2FCuEZAXJupXgdAxX5THCqR"));

        auto const otherkeys = {
            RippleKey::make_RippleKey(kt, passphrase),
            RippleKey::make_RippleKey(kt, toBase58(seed)),
            RippleKey::make_RippleKey(kt, strHex(seed.data(), seed.size())),
            RippleKey::make_RippleKey(kt, seedAs1751(seed))
        };
        for (auto const& other : otherkeys)
        {
            BEAST_EXPECT(other.keyType() == kt);
            BEAST_EXPECT(other.publicKey() == key.publicKey());
        }
    }

    void
    testFile(ripple::KeyType const kt)
    {
        using namespace boost::filesystem;

        auto const key = RippleKey::make_RippleKey(kt, passphrase);

        std::string const subdir = "test_key_file";
        KeyFileGuard g(*this, subdir);
        path const keyFile = subdir / ".ripple" / "secret-key.txt";

        // Try some failure cases before writing the file
        auto badFile = [&](const char* toWrite,
            std::string const& expectedException)
        {
            path const badKeyFile = subdir / "bad-key.txt";
            if(toWrite)
            {
                std::ofstream o(badKeyFile.string(), std::ios_base::trunc);
                if (BEAST_EXPECT(!o.fail()))
                {
                    o << toWrite;
                }
            }
            try
            {
                auto const keyBad = RippleKey::make_RippleKey(badKeyFile);
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(e.what() ==
                    std::string{ expectedException +
                    badKeyFile.string() });
            }
        };
        // No file
        badFile(nullptr, "Failed to open key file: ");
        // Write some nonsense to the file
        badFile("{ seed = \"Hello, world\" }",
            "Unable to parse json key file: ");
        // Write valid but incomplete json to the file
        badFile(R"({ "ponies": ["sparkleberry"] })",
            "Field 'key_type' is missing from key file: ");
        // Write a valid seed with an invalid keytype
        badFile(R"({ "key_type": "sha1", "master_seed": "masterpassphrase" })",
            R"(Invalid 'key_type' field "sha1" found in key file: )");
        {
            // Write a file over keyFile's directory
            auto const badPath = keyFile.parent_path();
            {
                std::ofstream o(badPath.string(),
                    std::ios_base::trunc);
            }
            try
            {
                key.writeToFile(keyFile);
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(e.what() ==
                    "Cannot create directory: " + badPath.string());
            }
            remove(badPath);
            create_directories(keyFile);
            try
            {
                key.writeToFile(keyFile);
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(e.what() ==
                    "Cannot open key file: " + keyFile.string());
            }
            remove_all(badPath);
        }

        key.writeToFile(keyFile);

        auto const key2 = RippleKey::make_RippleKey(keyFile);
        BEAST_EXPECT(key.keyType() == key2.keyType());
        BEAST_EXPECT(key.publicKey() == key2.publicKey());

        // Read the keyfile as a Json object to ensure it wrote
        // what we expected
        auto const jKeys = [&]
        {
            std::ifstream ifsKeys(keyFile.c_str(), std::ios::in);

            if (BEAST_EXPECT(ifsKeys))
            {

                Json::Reader reader;
                Json::Value jKeys;
                BEAST_EXPECT(reader.parse(ifsKeys, jKeys));
                return jKeys;
            }
            return Json::Value{};
        }();

        using namespace ripple;
        auto const seed = generateSeed(passphrase);
        auto const secretKey = generateKeyPair(kt, seed).second;

        // Make sure there are no extra fields
        BEAST_EXPECT(jKeys.size() == 9);
        BEAST_EXPECT(jKeys[jss::account_id] ==
            toBase58(calcAccountID(key.publicKey())));
        BEAST_EXPECT(jKeys[jss::key_type] == to_string(kt));
        BEAST_EXPECT(jKeys[jss::master_key] == seedAs1751(seed));
        BEAST_EXPECT(jKeys[jss::master_seed] == toBase58(seed));
        BEAST_EXPECT(jKeys[jss::master_seed_hex] ==
            strHex(seed.data(), seed.size()));
        BEAST_EXPECT(jKeys[jss::public_key] ==
            toBase58(TokenType::AccountPublic, key.publicKey()));
        BEAST_EXPECT(jKeys[jss::public_key_hex] ==
            strHex(key.publicKey().data(), key.publicKey().size()));
        BEAST_EXPECT(jKeys["secret_key"] ==
            toBase58(TokenType::AccountSecret, secretKey));
        BEAST_EXPECT(jKeys["secret_key_hex"] ==
            strHex(secretKey.data(), secretKey.size()));
    }

    void
    testSign(ripple::KeyType const kt)
    {
        using namespace offline;
        using namespace ripple;

        auto obj = deserialize(getKnownTxSigned().SerializedText);

        if (!BEAST_EXPECT(obj))
            return;

        boost::optional<ripple::STTx> tx {std::move(*obj)};
        // The hard-coded version is signed
        auto check = tx->checkSign(true);
        BEAST_EXPECT(check.first);
        {
            auto const key = RippleKey::make_RippleKey(kt,
                std::string("alice"));
            auto const expectedSignature = kt == KeyType::secp256k1 ?
                tx->getFieldVL(sfTxnSignature) :
                strUnHex("0751E8D38C26E8B6C953766A8A58570CA0CB93E57B86047F1FEF8DA3D7"
                    "9DFB97E78F4E59365C88EEE0E94EF7C1A2155A828B239AC00F3E95802D"
                    "851ABB113F06").first;
            auto const expectedSigningKey = kt == KeyType::secp256k1 ?
                tx->getFieldVL(sfSigningPubKey) :
                strUnHex("ED4A9D72F2557B714713DC8BA7C6F9576BCC06117A52F6C32"
                    "F1E26FEEF9819EC8E").first;

            // Remove the signature
            tx->makeFieldAbsent(sfTxnSignature);
            tx->setAccountID(sfAccount, calcAccountID(key.publicKey()));
            check = tx->checkSign(true);
            BEAST_EXPECT(!check.first);
            BEAST_EXPECT(check.second == "Invalid signature.");

            // Now re-sign it
            key.singleSign(tx);
            BEAST_EXPECT(tx->checkSign(true).first);
            // Same signature
            BEAST_EXPECT(tx->getFieldVL(sfSigningPubKey) == expectedSigningKey);
            BEAST_EXPECT(tx->getFieldVL(sfTxnSignature) == expectedSignature);
            BEAST_EXPECT(!tx->isFieldPresent(sfSigners));
        }

        {
            // Use a different key to multisign, because an account can't
            // multisign its own transaction.
            auto const key = RippleKey::make_RippleKey(kt, std::string("bob"));

            // Now multisign it with the test key
            key.multiSign(tx);
            BEAST_EXPECT(tx->checkSign(true).first);
            // No single signature
            BEAST_EXPECT(!tx->isFieldPresent(sfTxnSignature));
            BEAST_EXPECT(tx->getFieldVL(sfSigningPubKey).empty());
            if (BEAST_EXPECT(tx->isFieldPresent(sfSigners)))
            {
                auto const& signers = tx->getFieldArray(sfSigners);
                BEAST_EXPECT(signers.size() == 1);
                auto const& signer = signers[0];
                auto const expectedAccount = kt == KeyType::secp256k1 ?
                    "rPMh7Pi9ct699iZUTWaytJUoHcJ7cgyziK" :
                    "rJy554HmWFFJQGnRfZuoo8nV97XSMq77h7";
                auto const expectedPubKey = kt == KeyType::secp256k1 ?
                    "02691AC5AE1C4C333AE5DF8A93BDC495F0EEBFC6DB0DA7EB6"
                    "EF808F3AFC006E3FE" :
                    "ED3CC3D14FD80C213BC92A98AFE13A405A030F845EDCFD5E3"
                    "95286A6E9E62BA638";
                auto const expectedSignature = kt == KeyType::secp256k1 ?
                    "304402200719B97DA805D72C51100ECFEA86F73B7AC787559E"
                    "1AB34285C82CD0C7EC0A1402206EDDE8077DB49F808ED1BFC6"
                    "6CC06B944A11F05B58D59247B027B40F04E95412" :
                    "D12E9335B9AADAB917E65F5E3DB4B8A37DB0F5F5DC2E7333FF"
                    "26A8E5FEEC203D1F65ACADE6E6D0BD8E01D21C1838DF005E66"
                    "9AC1C8E57CA41405374CEDBB2309";
                auto const expectedHash = kt == KeyType::secp256k1 ?
                    "D955B668EF36A0E100D283CD8186F6B686EC140F10F3E5680E3"
                    "E53C1166DDBAB" :
                    "3CBBC2E5BA25609BC71B6380C1853CA73F39BC1E094232B3CBB"
                    "B7B2FBBC0347E";

                BEAST_EXPECT(to_string(signer.getAccountID(sfAccount)) ==
                    expectedAccount);
                BEAST_EXPECT(strHex(signer.getFieldVL(sfSigningPubKey)) ==
                    expectedPubKey);
                BEAST_EXPECT(strHex(signer.getFieldVL(sfTxnSignature)) ==
                    expectedSignature);
                BEAST_EXPECT(to_string (tx->getTransactionID()) ==
                    expectedHash);
            }

            // Sign with a second key
            auto const key2 = RippleKey::make_RippleKey(kt,
                passphrase);
            key2.multiSign(tx);
            BEAST_EXPECT(tx->checkSign(true).first);
            // No single signature
            BEAST_EXPECT(!tx->isFieldPresent(sfTxnSignature));
            BEAST_EXPECT(tx->getFieldVL(sfSigningPubKey).empty());
            if (BEAST_EXPECT(tx->isFieldPresent(sfSigners)))
            {
                auto const& signers = tx->getFieldArray(sfSigners);
                BEAST_EXPECT(signers.size() == 2);
                BEAST_EXPECT(signers[0].getAccountID(sfAccount) <
                    signers[1].getAccountID(sfAccount));
                // Because the masterpassphrase accountid happens to
                // sort before "bob" for both keytypes, the new
                // signature is inserted up front.
                auto const& signer = signers[0];
                auto const expectedAccount = kt == KeyType::secp256k1 ?
                    "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh" :
                    "rGWrZyQqhTp9Xu7G5Pkayo7bXjH4k4QYpf";
                auto const expectedPubKey = kt == KeyType::secp256k1 ?
                    "0330E7FC9D56BB25D6893BA3F317AE5BCF33B3291BD63DB32"
                    "654A313222F7FD020" :
                    "EDAAC3F98BB94F451804EF5993C847DAAA4E6154F45563565"
                    "9D88AA5C80F156303";
                auto const expectedSignature = kt == KeyType::secp256k1 ?
                    "3045022100C2496C05E17E3239837D7404F715A1C932FE286A"
                    "0540460D13E8BF4C9E4A7E3802205A3CED19AB8D924E8BDBD3"
                    "F14D74B6AB35BEDD62CEC936F138C35AC4EAFDBD83" :
                    "95103211B25FD07976C76D1BD0B205B37887F9F3799BA91402"
                    "1B40A6906723F47A78B66E141204E0123660F8C9D0B3F1263A"
                    "8119F4523EDB3FE6C594BFBA3603";
                auto const expectedHash = kt == KeyType::secp256k1 ?
                    "49D28003A776A7099EEEF64C35646AE4338E3D9065AE6A6A5DB"
                    "FFE4BDAEB260E" :
                    "3FE5058B1D802309DF7360A2155A97EF7A5E4213976E4E21D1F"
                    "B154FDFC0BCCF";

                BEAST_EXPECT(to_string(signer.getAccountID(sfAccount)) ==
                    expectedAccount);
                BEAST_EXPECT(strHex(signer.getFieldVL(sfSigningPubKey)) ==
                    expectedPubKey);
                BEAST_EXPECT(strHex(signer.getFieldVL(sfTxnSignature)) ==
                    expectedSignature);
                BEAST_EXPECT(to_string (tx->getTransactionID()) ==
                    expectedHash);
            }
        }
    }

    void
    testFaults()
    {
        using namespace std::string_literals;

        RippleKey key;
        boost::optional<ripple::STTx> tx;
        try
        {
            key.singleSign(tx);
            fail();
        }
        catch (std::runtime_error const& e)
        {
            BEAST_EXPECT(e.what() == "Internal error.  "
                "Empty std::optional passed to RippleKey::singleSign."s);
        }
        try
        {
            key.multiSign(tx);
            fail();
        }
        catch (std::runtime_error const& e)
        {
            BEAST_EXPECT(e.what() == "Internal error.  "
                "Empty std::optional passed to RippleKey::multiSign."s);
        }
    }


public:
    void
    run() override
    {
        using namespace ripple;

        std::array<KeyType, 2> constexpr keyTypes{ {
                KeyType::secp256k1,
                KeyType::ed25519 } };

        for (auto const& kt : keyTypes)
        {
            testRandom(kt);
            testSeed(kt);
            testFile(kt);
            testSign(kt);
        }

        testFaults();
    }
};

BEAST_DEFINE_TESTSUITE(RippleKey, keys, serialize);

} // test

} // serialize
