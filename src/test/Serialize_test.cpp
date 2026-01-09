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

#include <test/KeyFileGuard.h>
#include <test/KnownTestData.h>

#include <RippleKey.h>
#include <Serialize.h>

#include <ripple/basics/base64.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/TxFlags.h>

namespace offline {

namespace test {

class Serialize_test : public beast::unit_test::suite
{
private:
    void
    verifyKnownTx(ripple::STTx const& obj)
    {
        using namespace ripple;
        BEAST_EXPECT(
            to_string(obj.getTransactionID()) ==
            "F2D008D2AABBABD2A882F9049AA873210908EC3EA1EB0A2044A66093C7ACD2B1");
        BEAST_EXPECT(obj[sfTransactionType] == ttPAYMENT);
        BEAST_EXPECT(obj[sfFlags] == tfFullyCanonicalSig);
        BEAST_EXPECT(
            toBase58(obj[sfAccount]) == "rG1QQv2nh2gr7RCZ1P8YYcBUKCCN633jCn");
        BEAST_EXPECT(obj[sfSequence] == 18);
        BEAST_EXPECT(obj[sfFee] == STAmount(100));
        BEAST_EXPECT(
            strHex(obj[sfSigningPubKey]) ==
            "0388935426E0D08083314842EDFBB2D517BD47699F9A4527318A8E10468C97C05"
            "2");
        BEAST_EXPECT(
            strHex(obj[sfTxnSignature]) ==
            "3044022030425DB6A46B5B57BDA85E5B8455B90DC4EC57BA1A707AF0C"
            "28DC9383E09643D0220195B9FDBE383B813A539F3B70E130482E92D1E"
            "1210B0F85551E11B3F81EB98BB");
        BEAST_EXPECT(
            toBase58(obj[sfDestination]) ==
            "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh");
        auto const amountAccount =
            parseBase58<AccountID>("rhub8VRN55s94qWKDv6jmDy1pUykJzF3wq");
        if (BEAST_EXPECT(amountAccount))
        {
            BEAST_EXPECT(
                obj[sfAmount] ==
                STAmount(Issue(to_currency("USD"), *amountAccount), 123400000));
        }
        auto const sendMaxAccount =
            parseBase58<AccountID>("razqQKzJRdB4UxFPWf5NEpEG3WMkmwgcXA");
        if (BEAST_EXPECT(sendMaxAccount))
        {
            // Need a variable here instead of a literal to make
            // some compilers happy
            const std::uint64_t mantissa = 5678900000000000LLU;
            BEAST_EXPECT(
                obj[sfSendMax] ==
                STAmount(
                    Issue(to_currency("CNY"), *sendMaxAccount), mantissa, -4));
        }
    }

    void
    testParseJson()
    {
        testcase("ParseJson");

        auto const& testTx = getKnownTxSigned();
        auto json = parseJson(testTx.JsonText);
        BEAST_EXPECT(json);

        // Don't need to test the `JsonReader`, just a case to ensure the
        // wrapper handles failures
        json = parseJson("{ asjlfkjs");
        BEAST_EXPECT(!json);
    }

    void
    testMakeObject()
    {
        testcase("Make Object");

        auto const& testTx = getKnownTxSigned();
        auto json = parseJson(testTx.JsonText);
        auto obj = makeObject(json);
        if (BEAST_EXPECT(obj))
        {
            ripple::STTx tx{std::move(*obj)};
            verifyKnownTx(tx);
        }
    }

    void
    testSerialize()
    {
        testcase("Serialize");

        auto test = [&](TestItem const& testItem) {
            auto const json = parseJson(testItem.JsonText);
            auto const obj = makeObject(json);
            if (BEAST_EXPECT(obj))
            {
                auto const serialized = offline::serialize(*obj);
                BEAST_EXPECT(testItem.SerializedText == serialized);
            }
        };

        test(getKnownTxSigned());
        test(getKnownTxUnsigned());
        test(getKnownMetadata());
    }

    void
    testDeserialize()
    {
        testcase("Deserialize");

        using namespace ripple;

        auto test =
            [&](TestItem const& testItem,
                std::function<bool(ripple::STObject&, Json::Value const& known)>
                    extra = nullptr) {
                try
                {
                    auto obj = deserialize(testItem.SerializedText);

                    if (BEAST_EXPECT(obj))
                    {
                        auto check = true;
                        auto const known = parseJson(testItem.JsonText);
                        if (extra)
                            check = extra(*obj, known);
                        if (check)
                        {
                            BEAST_EXPECT(
                                obj->getJson(JsonOptions::none) == known);
                        }
                    }
                }
                catch (...)
                {
                    fail();
                }
            };

        test(getKnownTxSigned(), [&](auto& obj, auto const& known) {
            ripple::STTx tx{std::move(obj)};
            this->verifyKnownTx(tx);
            this->BEAST_EXPECT(tx.getJson(JsonOptions::none) == known);
            return false;
        });
        test(getKnownTxUnsigned());
        test(getKnownMetadata());
    }

    void
    testMakeSttx()
    {
        testcase("Make Sttx");

        using namespace boost::filesystem;
        using namespace ripple;

        {
            // golden path
            auto const& known = getKnownTxSigned();
            auto const origTx = offline::deserialize(known.SerializedText);
            if (BEAST_EXPECT(origTx))
            {
                std::unordered_set<uint256, beast::uhash<>> const presets{
                    ripple::featureExpandedSignerList};
                Rules const rules{presets};
                BEAST_EXPECT(rules.enabled(ripple::featureExpandedSignerList));
                {
                    auto const tx = offline::make_sttx(known.SerializedText);
                    BEAST_EXPECT(
                        tx[sfSigningPubKey] == (*origTx)[sfSigningPubKey]);
                    BEAST_EXPECT(
                        tx[sfTxnSignature] == (*origTx)[sfTxnSignature]);
                    BEAST_EXPECT(tx.checkSign(
                        STTx::RequireFullyCanonicalSig::yes, rules));
                }
                {
                    auto const tx = offline::make_sttx(known.JsonText);
                    BEAST_EXPECT(
                        tx[sfSigningPubKey] == (*origTx)[sfSigningPubKey]);
                    BEAST_EXPECT(
                        tx[sfTxnSignature] == (*origTx)[sfTxnSignature]);
                    BEAST_EXPECT(tx.checkSign(
                        STTx::RequireFullyCanonicalSig::yes, rules));
                }
            }
        }
        {
            // send sensible data that does not make a Tx
            auto const& known = getKnownMetadata();
            try
            {
                offline::make_sttx(known.SerializedText);
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECTS(
                    std::string{e.what()} == "Field not found: TransactionType",
                    e.what());
            }
            try
            {
                offline::make_sttx(known.JsonText);
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECTS(
                    std::string{e.what()} == "Field not found: TransactionType",
                    e.what());
            }
        }
        {
            // send nonsense
            try
            {
                auto const tx = offline::make_sttx("{ txtype = noop");
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(std::string{e.what()} == "invalid JSON");
            }
        }
    }

    void
    testBad()
    {
        testcase("Bad input");

        // This transaction has a DestinationTag larger than 32-bits
        std::string const bad{
            R"({
                "Account" : "rDAE53VfMvftPB4ogpWGWvzkQxfht6JPxr",
                "Amount" : "89031976",
                "Destination" : "rU2mEJSLqBRkYLVTv55rFTgQajkLTnT6mA",
                "DestinationTag" : 641505641505,
                "Fee" : "10000",
                "Flags" : 0,
                "LastLedgerSequence" : 68743734,
                "Sequence" : 68133057,
                "TransactionType" : "Payment"
            })"};
        {
            auto json = parseJson(bad);
            BEAST_EXPECT(json);
            try
            {
                auto obj = makeObject(json);
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(
                    e.what() ==
                    std::string(
                        "invalidParamsField '.DestinationTag' has bad type."));
            }
        }
    }

public:
    void
    run() override
    {
        testParseJson();
        testMakeObject();
        testSerialize();
        testDeserialize();
        testMakeSttx();
        testBad();
    }
};

BEAST_DEFINE_TESTSUITE(Serialize, keys, serialize);

}  // namespace test

}  // namespace offline
