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

#include <OfflineTool.h>
#include <RippleKey.h>
#include <Serialize.h>
#include <test/KnownTestData.h>
#include <test/KeyFileGuard.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/SecretKey.h>
#include <boost/format.hpp>

namespace offline {

namespace test {

class OfflineTool_test : public beast::unit_test::suite
{
private:

    // Allow cout and cerr to be redirected to a buffer.
    // Destructor restores old cout and cerr streambufs.
    class CoutRedirect
    {
    public:
        CoutRedirect()
            : oldOut_(std::cout.rdbuf(captureOut.rdbuf()))
            , oldErr_(std::cerr.rdbuf(captureErr.rdbuf()))
        {
        }

        ~CoutRedirect()
        {
            std::cout.rdbuf(oldOut_);
            std::cerr.rdbuf(oldErr_);
        }

        std::string out() const
        {
            return captureOut.str();
        }

        std::string err() const
        {
            return captureErr.str();
        }

    private:
        std::stringstream captureOut;
        std::stringstream captureErr;

        std::streambuf* const oldOut_;
        std::streambuf* const oldErr_;
    };

    class CInRedirect
    {
    public:
        CInRedirect(std::stringstream const& sStream)
            : old_(std::cin.rdbuf(sStream.rdbuf()))
        {
        }

        ~CInRedirect()
        {
            std::cin.rdbuf(old_);
        }

    private:
        std::streambuf* const old_;
    };

    void
    testSerialize()
    {
        testcase("Serialize");

        auto test = [&](TestItem const& testItem)
        {
            {
                CoutRedirect coutRedirect;

                auto const exit = doSerialize(testItem.JsonText);

                BEAST_EXPECT(exit == EXIT_SUCCESS);
                BEAST_EXPECT(coutRedirect.out() == testItem.SerializedText + "\n");
                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
            }
            {
                std::stringstream jsoninput(testItem.JsonText);
                CInRedirect cinRedirect{ jsoninput };
                CoutRedirect coutRedirect;

                auto const exit = runCommand("serialize", {}, {}, {},
                    InputType::readstdin);

                BEAST_EXPECT(exit == EXIT_SUCCESS);
                BEAST_EXPECT(coutRedirect.out() == testItem.SerializedText + "\n");
                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
            }
        };

        test(getKnownTxSigned());
        test(getKnownTxUnsigned());
        test(getKnownMetadata());
        {
            CoutRedirect coutRedirect;

            // Send it nonsense
            auto const exit = doSerialize("Hello, world!");

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECT(coutRedirect.out().empty());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to serialize \"Hello, world!\"\n");
        };

    }

    void
    testDeserialize()
    {
        testcase("Deserialize");

        auto test = [&](TestItem const& testItem,
            std::function<std::string(std::string)> modifySerialized = nullptr,
            std::function<void(Json::Value&)> modifyKnownJson = nullptr)
        {
            {
                CoutRedirect coutRedirect;

                try
                {
                    auto const exit = doDeserialize(modifySerialized ?
                        modifySerialized(testItem.SerializedText) :
                        testItem.SerializedText);
                    BEAST_EXPECT(exit == EXIT_SUCCESS);
                }
                catch (...)
                {
                    fail();
                }

                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                auto const captured = parseJson(coutRedirect.out());
                auto known = parseJson(testItem.JsonText);
                if (modifyKnownJson)
                    modifyKnownJson(known);
                BEAST_EXPECT(captured == known);
            }
            {
                std::stringstream serinput(modifySerialized ?
                    modifySerialized(testItem.SerializedText) :
                    testItem.SerializedText);
                CInRedirect cinRedirect{ serinput };

                CoutRedirect coutRedirect;

                auto const exit = runCommand("deserialize", {}, {}, {}, InputType::readstdin);

                BEAST_EXPECT(exit == EXIT_SUCCESS);
                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                auto const captured = parseJson(coutRedirect.out());
                auto known = parseJson(testItem.JsonText);
                if (modifyKnownJson)
                    modifyKnownJson(known);
                BEAST_EXPECT(captured == known);
            }
        };
        test(getKnownTxSigned(),
            [](auto serialized)
            {
                // include some extra whitespace, since deserialization
                // is sensitive to that.
                return "  " + serialized + "\n\n";
            },
            [](auto& known)
            {
                // The hash field is STTx-specific (and computed),
                // so it won't be in the generic output.
                known.removeMember("hash");
            });
        test(getKnownTxUnsigned());
        test(getKnownMetadata());
        {
            CoutRedirect coutRedirect;

            // Send it nonsense
            auto const exit = doDeserialize("Hello, world!");

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to deserialize \"Hello, world!\"\n"
                "Is this valid serialized data?\n");
        }
        {
            CoutRedirect coutRedirect;

            // Send it an incomplete Tx
            auto shortTx = getKnownTxUnsigned().SerializedText.substr(0, 192);
            auto const exit = doDeserialize(shortTx);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to deserialize \"" + shortTx + "\"\n"
                "Is this valid serialized data?\n"
                "\tDetail: invalid SerialIter getBitString\n");
        }
    }

    void
    testSingleSign()
    {
        testcase("Single Sign");

        using namespace boost::filesystem;

        std::string const subdir = "test_key_file";
        KeyFileGuard g(*this, subdir);
        path const keyFile = subdir / ".ripple" / "secret-key.txt";

        {
            RippleKey const key;
            key.writeToFile(keyFile);
        }

        auto const& knownTx = getKnownTxSigned();
        auto const origTx = offline::deserialize(knownTx.SerializedText);

        auto test = [&](std::string const& testData)
        {
            using namespace ripple;

            auto go = [&](std::string const& json)
            {
                auto const tx = make_sttx(json);
                BEAST_EXPECT(tx.checkSign(
                    STTx::RequireFullyCanonicalSig::yes).first);
                BEAST_EXPECT(tx[sfSigningPubKey] !=
                    (*origTx)[sfSigningPubKey]);
                BEAST_EXPECT(tx[sfTxnSignature] !=
                    (*origTx)[sfTxnSignature]);
                BEAST_EXPECT(!tx.isFieldPresent(sfSigners));
            };
            {
                CoutRedirect coutRedirect;

                try
                {
                    auto const exit = doSingleSign(testData, keyFile);
                    BEAST_EXPECT(exit == EXIT_SUCCESS);
                }
                catch (...)
                {
                    fail();
                    return;
                }

                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                go(coutRedirect.out());
            }
            {
                std::stringstream serinput(testData);
                CInRedirect cinRedirect{ serinput };
                CoutRedirect coutRedirect;

                auto const exit = runCommand("sign", {}, keyFile, {},
                    InputType::readstdin);

                BEAST_EXPECT(exit == EXIT_SUCCESS);
                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                go(coutRedirect.out());
            }
        };
        test(knownTx.SerializedText);
        test(knownTx.JsonText);
        auto const& knownTxUnsigned = getKnownTxUnsigned();
        test(knownTxUnsigned.SerializedText);
        test(knownTxUnsigned.JsonText);
        {
            CoutRedirect coutRedirect;

            // Send it nonsense
            auto const exit = doSingleSign("Hello, world!", keyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to sign \"Hello, world!\"\n"
                "Detail: invalid JSON\n");
        }
        {
            CoutRedirect coutRedirect;

            // Send it an incomplete Tx
            auto shortTx = getKnownTxUnsigned().SerializedText.substr(0, 192);
            auto const exit = doSingleSign(shortTx, keyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to sign \"" + shortTx + "\"\n"
                "Detail: unable to deserialize (internal: invalid SerialIter getBitString)\n");
        }
        {
            CoutRedirect coutRedirect;

            // Send it a Tx with missing field
            auto json = offline::parseJson(knownTxUnsigned.JsonText);
            BEAST_EXPECT(json && json.isObject());
            json.removeMember("Sequence");
            auto const shortTx = json.toStyledString();
            auto const exit = doSingleSign(shortTx, keyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECTS(coutRedirect.err() ==
                "Unable to sign \"" + shortTx + "\"\n"
                "Detail: Field 'Sequence' is required but missing.\n",
                coutRedirect.err() + "\n\n" + shortTx);
        }
        {
            CoutRedirect coutRedirect;

            // Use an invalid key file
            auto const badKeyFile = subdir / "invalid.txt";
            auto const exit = doSingleSign(knownTx.SerializedText,
                badKeyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to sign \"" + knownTx.SerializedText + "\"\n"
                "Reason: Failed to open key file: " +
                badKeyFile.string() + "\n");
        }
    }

    void
    testMultiSign()
    {
        testcase("Multi Sign");

        using namespace boost::filesystem;

        std::string const subdir = "test_key_file";
        KeyFileGuard g(*this, subdir);
        path const keyFile = subdir / ".ripple" / "secret-key.txt";

        {
            RippleKey const key;
            key.writeToFile(keyFile);
        }

        auto test = [&](std::string const& testData)
        {
            using namespace ripple;
            {
                CoutRedirect coutRedirect;

                try
                {
                    auto const exit = doMultiSign(testData, keyFile);

                    BEAST_EXPECT(exit == EXIT_SUCCESS);
                    BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                    auto const tx = make_sttx(coutRedirect.out());
                    BEAST_EXPECT(tx.checkSign(
                        STTx::RequireFullyCanonicalSig::yes).first);
                    BEAST_EXPECT(tx.isFieldPresent(sfSigningPubKey));
                    BEAST_EXPECT(tx[sfSigningPubKey].empty());
                    BEAST_EXPECT(!tx.isFieldPresent(sfTxnSignature));
                    BEAST_EXPECT(tx.isFieldPresent(sfSigners));
                }
                catch (...)
                {
                    fail();
                }
            }
            {
                std::stringstream serinput(testData);
                CInRedirect cinRedirect{ serinput };
                CoutRedirect coutRedirect;

                auto const exit = runCommand("multisign", {}, keyFile, {},
                    InputType::readstdin);

                BEAST_EXPECT(exit == EXIT_SUCCESS);
                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                auto const tx = make_sttx(coutRedirect.out());
                BEAST_EXPECT(tx.checkSign(
                    STTx::RequireFullyCanonicalSig::yes).first);
                BEAST_EXPECT(tx.isFieldPresent(sfSigningPubKey));
                BEAST_EXPECT(tx[sfSigningPubKey].empty());
                BEAST_EXPECT(!tx.isFieldPresent(sfTxnSignature));
                BEAST_EXPECT(tx.isFieldPresent(sfSigners));
            }
        };
        auto const& knownTxSigned = getKnownTxSigned();
        test(knownTxSigned.SerializedText);
        test(knownTxSigned.JsonText);
        auto const& knownTxUnsigned = getKnownTxUnsigned();
        test(knownTxUnsigned.SerializedText);
        test(knownTxUnsigned.JsonText);
        {
            CoutRedirect coutRedirect;

            // Send it nonsense
            auto const exit = doMultiSign("Hello, world!", keyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to sign \"Hello, world!\"\n"
                "Detail: invalid JSON\n");
        }
        {
            CoutRedirect coutRedirect;

            // Send it an incomplete Tx
            auto shortTx = getKnownTxUnsigned().SerializedText.substr(0, 192);
            auto const exit = doMultiSign(shortTx, keyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to sign \"" + shortTx + "\"\n"
                "Detail: unable to deserialize (internal: invalid SerialIter getBitString)\n");
        }
        {
            CoutRedirect coutRedirect;

            // Send it a Tx with missing field
            auto json = offline::parseJson(knownTxUnsigned.JsonText);
            BEAST_EXPECT(json && json.isObject());
            json.removeMember("Sequence");
            auto const shortTx = json.toStyledString();
            auto const exit = doSingleSign(shortTx, keyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECTS(coutRedirect.err() ==
                "Unable to sign \"" + shortTx + "\"\n"
                "Detail: Field 'Sequence' is required but missing.\n",
                coutRedirect.err() + "\n\n" + shortTx);
        }
        {
            CoutRedirect coutRedirect;

            // Use an invalid key file
            auto const badKeyFile = subdir / "invalid.txt";
            auto const exit = doMultiSign(knownTxUnsigned.SerializedText,
                badKeyFile);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() ==
                "Unable to sign \"" + knownTxUnsigned.SerializedText + "\"\n"
                "Reason: Failed to open key file: " +
                badKeyFile.string() + "\n");
        }
    }

    void testCreateKeyfile()
    {
        testcase("Create keyfile");

        using namespace boost::filesystem;
        using namespace ripple;

        std::string const subdir = "test_key_file";
        KeyFileGuard g(*this, subdir);
        path const keyFile = subdir / ".ripple" / "secret-key.txt";

        auto test = [&](boost::optional<std::string> const& kt,
            boost::optional<std::string> const& seed)
        {
            auto const go = [&](bool useCommand)
            {
                CoutRedirect coutRedirect;

                if (useCommand)
                {
                    std::vector<std::string> args;
                    InputType inputType = InputType::none;
                    if (seed)
                    {
                        args.push_back(*seed);
                        inputType = InputType::commandline;
                    }
                    auto const exit = runCommand("createkeyfile", args,
                        keyFile, kt, inputType);
                    BEAST_EXPECT(exit == EXIT_SUCCESS);
                }
                else
                {
                    auto const exit = doCreateKeyfile(keyFile, kt, seed);
                    BEAST_EXPECT(exit == EXIT_SUCCESS);
                }

                auto const key = RippleKey::make_RippleKey(keyFile);

                auto const known = boost::str(
                    boost::format("New ripple key created in %s\n"
                    "Key type is %s, and account ID is %s\n"
                    "\nThis file should be stored securely and not shared\n\n")
                    % keyFile.string()
                    % to_string(key.keyType())
                    % toBase58(calcAccountID(key.publicKey())));

                // Test that the function will not overwrite
                try
                {
                    if (useCommand)
                        runCommand("createkeyfile", {}, keyFile, {}, {});
                    else
                        doCreateKeyfile(keyFile, {}, {});
                    fail();
                }
                catch (std::exception const& e)
                {
                    BEAST_EXPECT(e.what() ==
                        std::string{ "Refusing to overwrite existing key file: " +
                        keyFile.string()});
                }

                remove(keyFile);

                BEAST_EXPECTS(coutRedirect.err().empty(), coutRedirect.err());
                BEAST_EXPECT(coutRedirect.out() == known);
            };
            go(false);
            go(true);
        };

        test(boost::none, boost::none);
        test(boost::none, std::string{ "masterpassphrase" });
        test(std::string(to_string(KeyType::ed25519)), boost::none);
        test(std::string(to_string(KeyType::secp256k1)),
            std::string{ "alice" });

        // edge cases
        {
            // invalid keytype
            CoutRedirect coutRedirect;

            auto const exit = doCreateKeyfile(keyFile,
                std::string{ "NSA special" }, boost::none);

            BEAST_EXPECT(exit == EXIT_FAILURE);
            BEAST_EXPECT(!exists(keyFile));
            BEAST_EXPECTS(coutRedirect.out().empty(), coutRedirect.out());
            BEAST_EXPECT(coutRedirect.err() == "Invalid key type: \"NSA special\"\n");
        }
        {
            // empty seed
            try
            {
                doCreateKeyfile(keyFile, std::string{ "ed25519" },
                    std::string{ "" });
                fail();
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(e.what() ==
                    std::string{ "Unable to parse seed: " });
            }

            BEAST_EXPECT(!exists(keyFile));
        }
    }

    void
    testRunCommand ()
    {
        testcase ("Run Command");

        CoutRedirect coutRedirect;

        using namespace boost::filesystem;

        std::string const subdir = "test_key_file";
        KeyFileGuard g(*this, subdir);
        path const keyFile = subdir / ".ripple" / "secret-key.txt";

        auto testCommand = [&](
            std::string const& command,
            std::vector <std::string> const& args,
            std::string const& expectedError,
            int const expectedExit = EXIT_FAILURE)
        {
            try
            {
                auto const inputType = args.empty() ? InputType::none :
                    InputType::commandline;
                auto const exit = runCommand(command, args, keyFile, {},
                    inputType);
                BEAST_EXPECT(exit == expectedExit);
                BEAST_EXPECT(expectedError.empty());
            }
            catch (std::exception const& e)
            {
                BEAST_EXPECT(e.what() == expectedError);
            }
        };

        std::stringstream emptyinput;
        CInRedirect cinRedirect{ emptyinput };

        std::vector <std::string> const noArgs;
        std::vector <std::string> const oneArg = { "some data" };
        std::vector <std::string> const twoArgs = { "data", "more data" };
        std::string const noError = "";
        std::string const argError = "Syntax error: Wrong number of arguments";
        {
            std::string const command = "unknown";
            std::string const expectedError = "Unknown command: " + command;
            testCommand(command, noArgs, expectedError);
            testCommand(command, oneArg, expectedError);
            testCommand(command, twoArgs, expectedError);
        }
        {
            std::string const command = "serialize";
            testCommand(command, noArgs, argError);
            testCommand(command, oneArg, noError);
            testCommand(command, twoArgs, argError);
        }
        {
            std::string const command = "deserialize";
            testCommand(command, noArgs, argError);
            testCommand(command, oneArg, noError);
            testCommand(command, twoArgs, argError);
        }
        {
            std::string const command = "sign";
            testCommand(command, noArgs, argError);
            testCommand(command, oneArg, noError);
            testCommand(command, twoArgs, argError);
        }
        {
            std::string const command = "multisign";
            testCommand(command, noArgs, argError);
            testCommand(command, oneArg, noError);
            testCommand(command, twoArgs, argError);
        }
        {
            std::string const command = "createkeyfile";
            testCommand(command, noArgs, noError, EXIT_SUCCESS);
            remove(keyFile);
            testCommand(command, oneArg, noError, EXIT_SUCCESS);
            remove(keyFile);
            testCommand(command, twoArgs, argError);
        }
    }

public:
    void
    run() override
    {
        BEAST_EXPECT(!getVersionString().empty());

        testSerialize();
        testDeserialize();
        testSingleSign();
        testMultiSign();
        testCreateKeyfile();
        testRunCommand ();
    }
};

BEAST_DEFINE_TESTSUITE(OfflineTool, keys, serialize);

} // test

} // serialize
