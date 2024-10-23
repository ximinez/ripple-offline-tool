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

#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/HashPrefix.h>

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/program_options.hpp>
#ifdef BOOST_MSVC
#ifndef WIN32_LEAN_AND_MEAN  // VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif
#endif

//------------------------------------------------------------------------------
char const* const versionString =

    //--------------------------------------------------------------------------
    //  The build version number. You must edit this for each release
    //  and follow the format described at http://semver.org/
    //
    "0.4.0"

#if defined(DEBUG) || defined(SANITIZER)
    "+"
#ifdef DEBUG
    "DEBUG"
#ifdef SANITIZER
    "."
#endif
#endif

#ifdef SANITIZER
    BOOST_PP_STRINGIZE(SANITIZER)
#endif
#endif

    //--------------------------------------------------------------------------
    ;

static int
runUnitTests()
{
    using namespace beast::unit_test;
    reporter r;
    bool const anyFailed = r.run_each(global_suites());
    if (anyFailed)
        return EXIT_FAILURE;  // LCOV_EXCL_LINE
    return EXIT_SUCCESS;
}

int
doSerialize(std::string const& data)
{
    auto const tx = [&] {
        auto const json = offline::parseJson(data);
        return json ? offline::makeObject(json) : std::nullopt;
    }();
    if (!tx)
    {
        std::cerr << "Unable to serialize \"" << data << "\"" << std::endl;
        return EXIT_FAILURE;
    }

    auto const result = offline::serialize(*tx);

    std::cout << result << std::endl;
    return EXIT_SUCCESS;
}

int
doDeserialize(std::string const& data)
{
    using namespace ripple;

    auto const fail = [&] {
        std::cerr << "Unable to deserialize \"" << data << "\"" << std::endl;
    };
    try
    {
        auto const result = offline::deserialize(boost::trim_copy(data));

        if (result)
        {
            std::cout << result->getJson(JsonOptions::none).toStyledString()
                      << std::endl;
            return EXIT_SUCCESS;
        }
        else
        {
            fail();
            std::cerr << "Is this valid serialized data?" << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (std::exception const& e)
    {
        fail();
        std::cerr << "Is this valid serialized data?" << std::endl;
        std::cerr << "\tDetail: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

int
doHash(ripple::HashPrefix prefix, std::string const& data)
{
    auto const obj = offline::make_stobject(data);

    auto const result = obj.getHash(prefix);

    std::cout << result << std::endl;
    return EXIT_SUCCESS;
}

int
doSign(
    std::string const& data,
    boost::filesystem::path const& keyFile,
    std::function<
        void(offline::RippleKey const& key, std::optional<ripple::STTx>& tx)>
        signingOp)
{
    using namespace ripple;
    using namespace offline;
    auto const fail = [&]() {
        std::cerr << "Unable to sign \"" << data << "\"" << std::endl;
    };
    std::optional<ripple::STTx> tx;
    try
    {
        tx.emplace(make_sttx(boost::trim_copy(data)));
    }
    catch (std::exception const& e)
    {
        fail();
        std::cerr << "Detail: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    try
    {
        BOOST_ASSERT(tx);
        auto const rippleKey = RippleKey::make_RippleKey(keyFile);

        signingOp(rippleKey, tx);

        std::cout << tx->getJson(JsonOptions::none).toStyledString()
                  << std::endl;
        return EXIT_SUCCESS;
    }
    catch (std::exception const& e)
    {
        fail();
        std::cerr << "Reason: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

int
doSingleSign(std::string const& data, boost::filesystem::path const& keyFile)
{
    return doSign(
        data,
        keyFile,
        [](offline::RippleKey const& key, std::optional<ripple::STTx>& tx) {
            key.singleSign(tx);
        });
}

int
doMultiSign(std::string const& data, boost::filesystem::path const& keyFile)
{
    return doSign(
        data,
        keyFile,
        [](offline::RippleKey const& key, std::optional<ripple::STTx>& tx) {
            key.multiSign(tx);
        });
}

int
doArbitrarySign(std::string const& data, boost::filesystem::path const& keyFile)
{
    using namespace ripple;
    using namespace offline;
    auto const fail = [&]() {
        std::cerr << "Unable to sign \"" << data << "\"" << std::endl;
    };
    std::optional<ripple::STObject> obj;
    try
    {
        obj.emplace(make_stobject(boost::trim_copy(data)));
    }
    catch (std::exception const& e)
    {
        fail();
        std::cerr << "Detail: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    try
    {
        assert(obj);
        auto const rippleKey = RippleKey::make_RippleKey(keyFile);

        if (!obj)
        {
            throw std::runtime_error("Internal error.");
        }

        rippleKey.arbitrarySign(/*HashPrefix::manifest*/ std::nullopt, *obj);

        std::cout << obj->getJson(JsonOptions::none).toStyledString()
                  << std::endl;
        return EXIT_SUCCESS;
    }
    catch (std::exception const& e)
    {
        fail();
        std::cerr << "Reason: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

int
doCreateKeyfile(
    boost::filesystem::path const& keyFile,
    std::optional<std::string> const& keytype,
    std::optional<std::string> const& seed)
{
    using namespace ripple;
    using namespace offline;

    if (exists(keyFile))
        throw std::runtime_error(
            "Refusing to overwrite existing key file: " + keyFile.string());

    auto const kt = keytype ? keyTypeFromString(*keytype) : std::nullopt;
    if (keytype && !kt)
    {
        std::cerr << "Invalid key type: \"" << *keytype << "\"" << std::endl;
        return EXIT_FAILURE;
    }

    auto const key = RippleKey::make_RippleKey(kt, seed);

    key.writeToFile(keyFile);

    std::cout << "New ripple key created "
              << "in " << keyFile.string() << "\n"
              << "Key type is " << to_string(key.keyType()) << ", and "
              << "account ID is " << toBase58(calcAccountID(key.publicKey()))
              << "\n"
              << "\nThis file should be stored securely and not shared\n\n";

    return EXIT_SUCCESS;
}

std::string
getStdin()
{
    std::ostringstream stdinput;
    stdinput << std::cin.rdbuf();
    return stdinput.str();
}

int
runCommand(
    const std::string& command,
    std::vector<std::string> const& args,
    boost::filesystem::path const& keyFile,
    std::optional<std::string> const& keyType,
    InputType const& inputType)
{
    using namespace std;

    struct commandParams
    {
        bool const allowNoInput;
        std::function<int(
            std::optional<std::string> const& input,
            boost::filesystem::path const& keyFile,
            std::optional<std::string> const& keyType)> const action;
    };
    /* TODO: VC compiler doesn't like
            std::function<void(std::string const& input)> const action;
        with each of the lamdas capturing other local variables.
    */
    auto const serialize = [](auto const& input, auto const&, auto const&) {
        BOOST_ASSERT(input);
        return doSerialize(*input);
    };
    auto const deserialize = [](auto const& input, auto const&, auto const&) {
        BOOST_ASSERT(input);
        return doDeserialize(*input);
    };
    auto const txhash = [](auto const& input, auto const&, auto const&) {
        BOOST_ASSERT(input);
        return doHash(ripple::HashPrefix::transactionID, *input);
    };
    auto const sign = [](auto const& input, auto const& keyFile, auto const&) {
        BOOST_ASSERT(input);
        return doSingleSign(*input, keyFile);
    };
    auto const multisign =
        [](auto const& input, auto const& keyFile, auto const&) {
            BOOST_ASSERT(input);
            return doMultiSign(*input, keyFile);
        };
    auto const asign = [](auto const& input, auto const& keyFile, auto const&) {
        BOOST_ASSERT(input);
        return doArbitrarySign(*input, keyFile);
    };
    auto const createkeyfile =
        [](auto const& seed, auto const& keyFile, auto const& keyType) {
            return doCreateKeyfile(keyFile, keyType, seed);
        };
    auto const argumenterror = []() {
        throw std::runtime_error("Syntax error: Wrong number of arguments");
    };
    static map<string, commandParams> const commandArgs = {
        {"serialize", {false, serialize}},
        {"deserialize", {false, deserialize}},
        {"sign", {false, sign}},
        {"multisign", {false, multisign}},
        {"asign", {false, asign}},
        {"txhash", {false, txhash}},
        {"createkeyfile", {true, createkeyfile}},
    };

    auto const iArgs = commandArgs.find(command);

    if (iArgs == commandArgs.end())
        throw std::runtime_error("Unknown command: " + command);

    // getInputType has already resolved conflicts
    std::optional<std::string> input;
    switch (inputType)
    {
        case InputType::readstdin:
            input = boost::trim_copy(getStdin());
            break;

        case InputType::commandline:
            if (args.size() != 1)
                argumenterror();
            input = args[0];
            break;

        default:
            if (!iArgs->second.allowNoInput)
                argumenterror();
            break;
    }

    BOOST_ASSERT(iArgs->second.action);
    return iArgs->second.action(input, keyFile, keyType);
}

// LCOV_EXCL_START
static std::string
getEnvVar(char const* name)
{
    std::string value;

    auto const v = getenv(name);

    if (v != nullptr)
        value = v;

    return value;
}

void
printHelp(
    const boost::program_options::options_description& desc,
    boost::filesystem::path const& defaultKeyfile)
{
    static std::string const name = "ripple-offline";

    std::cerr << name << " [options] <command> [<argument> ...]\n"
              << desc << std::endl
              <<
        R"(Commands:
  Serialization:
    serialize <argument>|--stdin        Serialize from JSON.
    deserialize <argument>|--stdin      Deserialize to JSON.
  Transaction signing:
    sign <argument>|--stdin             Sign for submission.
    multisign <argument>|--stdin        Apply a multi-signature.
      Signing commands require a valid keyfile.
      Input is serialized or unserialized JSON.
      Output is unserialized JSON.
  Arbitrary signing:
    asign <argument>|--stdin            Sign arbitrary data.
  Hashing:
    txhash <argument>|--stdin           Hash a transaction.
  Key Management:
    createkeyfile [<key>|--stdin]       Create keyfile. A random
      seed will be used if no <key> is provided on the command line
      or from standard input using --stdin.

      Default keyfile is: )"
              << defaultKeyfile << "\n";
    // In progress:
    // hash <prefix> <argument>|--stdin             Hash an XRPL object.
}

InputType
getInputType(boost::program_options::variables_map const& vm)
{
    namespace po = boost::program_options;

    bool const readstdin = vm.count("stdin") && !vm["stdin"].defaulted();
    bool const commandline =
        !vm["arguments"].empty() && !vm["arguments"].defaulted();

    if (readstdin && commandline)
        throw std::runtime_error(
            "Conflicting inputs: May only specify one of \"--stdin\" "
            "and command line parameters.");
    if (readstdin)
        return InputType::readstdin;
    if (commandline)
        return InputType::commandline;
    return InputType::none;
}
// LCOV_EXCL_STOP

std::string const&
getVersionString()
{
    static std::string const value = [] {
        std::string const s = versionString;
        beast::SemanticVersion v;
        if (!v.parse(s) || v.print() != s)
            throw std::logic_error(
                s + ": Bad version string");  // LCOV_EXCL_LINE
        return s;
    }();
    return value;
}

int
main(int argc, char** argv)
{
#if defined(__GNUC__) && !defined(__clang__)
    auto constexpr gccver =
        (__GNUC__ * 100 * 100) + (__GNUC_MINOR__ * 100) + __GNUC_PATCHLEVEL__;

    static_assert(
        gccver >= 50100,
        "GCC version 5.1.0 or later is required to compile "
        "ripple-offline-tool.");
#endif

    static_assert(
        BOOST_VERSION >= 105700,
        "Boost version 1.57 or later is required to compile "
        "ripple-offline-tool");

    namespace po = boost::program_options;

    po::variables_map vm;

    // Set up option parsing.
    //
    po::options_description general("General Options");
    general.add_options()("help,h", "Display this message.")(
        "unittest,u", "Perform unit tests.")(
        "version", "Display the build version.")(
        "keyfile,f", po::value<std::string>(), "Specify the key file.")(
        "stdin,i", "Read input (private key or argument) from stdin.");

    po::options_description key("Key File Creation Options");
    key.add_options()(
        "keytype,t",
        po::value<std::string>(),
        "Valid keytypes are secp256k1 and ed25519. Default is secp256k1.");

    // Interpret positional arguments as --parameters.
    po::options_description hidden("Hidden options");
    hidden.add_options()("command", po::value<std::string>(), "Command.")(
        "arguments",
        po::value<std::vector<std::string>>()->default_value(
            std::vector<std::string>(), "empty"),
        "Arguments.");
    po::positional_options_description p;
    p.add("command", 1).add("arguments", -1);

    po::options_description help_options;
    help_options.add(general).add(key);
    po::options_description cmdline_options;
    cmdline_options.add(help_options).add(hidden);

    // Parse options, if no error.
    try
    {
        po::store(
            po::command_line_parser(argc, argv)
                .options(cmdline_options)  // Parse options.
                .positional(p)             // Remainder as --parameters.
                .run(),
            vm);
        po::notify(vm);  // Invoke option notify functions.
    }
    // LCOV_EXCL_START
    catch (std::exception const&)
    {
        std::cerr << "ripple-offline: Incorrect command line syntax."
                  << std::endl;
        std::cerr << "Use '--help' for a list of options." << std::endl;
        return EXIT_FAILURE;
    }
    // LCOV_EXCL_STOP

    // Run the unit tests if requested.
    // The unit tests will exit the application with an appropriate return code.
    if (vm.count("unittest"))
        return runUnitTests();

    // LCOV_EXCL_START
    if (vm.count("version"))
    {
        std::cout << "ripple-offline version " << getVersionString()
                  << std::endl;
        return EXIT_SUCCESS;
    }

    boost::filesystem::path const homeDir = getEnvVar("HOME");
    auto const defaultKeyfile =
        (homeDir.empty() ? boost::filesystem::current_path() : homeDir) /
        ".ripple" / "secret-key.txt";

    if (vm.count("help") || !vm.count("command"))
    {
        printHelp(help_options, defaultKeyfile);
        return EXIT_SUCCESS;
    }

    try
    {
        using namespace boost::filesystem;

        path const keyFile = vm.count("keyfile")
            ? vm["keyfile"].as<std::string>()
            : defaultKeyfile;
        auto const keyType = vm.count("keytype")
            ? std::optional<std::string>(vm["keytype"].as<std::string>())
            : std::nullopt;
        auto const inputType = getInputType(vm);

        return runCommand(
            vm["command"].as<std::string>(),
            vm["arguments"].as<std::vector<std::string>>(),
            keyFile,
            keyType,
            inputType);
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    // LCOV_EXCL_STOP
}
