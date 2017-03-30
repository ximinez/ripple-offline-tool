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

#ifndef SERIALIZE_TESTS_KNOWNTX_H_INCLUDED
#define SERIALIZE_TESTS_KNOWNTX_H_INCLUDED

#include <string>

namespace offline
{
namespace test
{

struct TestItem
{
    std::string JsonText;
    std::string SerializedText;
};

inline
TestItem
const& getKnownTxSigned()
{
    // From the ripplelibppdemo output
    static TestItem testTx
    {
        R"({
            "Account" : "rG1QQv2nh2gr7RCZ1P8YYcBUKCCN633jCn",
            "Amount" :
            {
                    "currency" : "USD",
                    "issuer" : "rhub8VRN55s94qWKDv6jmDy1pUykJzF3wq",
                    "value" : "123400000"
            },
            "Destination" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
            "Fee" : "100",
            "Flags" : 2147483648,
            "SendMax" :
            {
                    "currency" : "CNY",
                    "issuer" : "razqQKzJRdB4UxFPWf5NEpEG3WMkmwgcXA",
                    "value" : "5678900000000000e-4"
            },
            "Sequence" : 18,
            "SigningPubKey" : "0388935426E0D08083314842EDFBB2D517BD47699F9A4527318A8E10468C97C052",
            "TransactionType" : "Payment",
            "TxnSignature" : "3044022030425DB6A46B5B57BDA85E5B8455B90DC4EC57BA1A707AF0C28DC9383E09643D0220195B9FDBE383B813A539F3B70E130482E92D1E1210B0F85551E11B3F81EB98BB",
            "hash" : "F2D008D2AABBABD2A882F9049AA873210908EC3EA1EB0A2044A66093C7ACD2B1"
            })",
        "1200002280000000240000001261D684625103A72000000"
        "00000000000000000000055534400000000002ADB0B3959D60A6E6991F729"
        "E1918B716392523068400000000000006469D7542CEDF1370800000000000"
        "000000000000000434E59000000000041C8BE2C0A6AA17471B9F6D0AF92AA"
        "B1C94D5A2573210388935426E0D08083314842EDFBB2D517BD47699F9A452"
        "7318A8E10468C97C05274463044022030425DB6A46B5B57BDA85E5B8455B9"
        "0DC4EC57BA1A707AF0C28DC9383E09643D0220195B9FDBE383B813A539F3B"
        "70E130482E92D1E1210B0F85551E11B3F81EB98BB8114AE123A8556F3CF91"
        "154711376AFB0F894F832B3D8314B5F762798A53D543A014CAF8B297CFF8F"
        "2F937E8"
    };
    return testTx;
}

inline
TestItem
const& getKnownTxUnsigned()
{
    // From the ripplelibppdemo output
    static TestItem testTx
    {
        R"({
           "Account" : "r9mC1zjD9u5SJXw56pdPhxoDSHaiNcisET",
           "Amount" : {
              "currency" : "USD",
              "issuer" : "rhub8VRN55s94qWKDv6jmDy1pUykJzF3wq",
              "value" : "123400000"
           },
           "Destination" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Fee" : "100",
           "Flags" : 2147483648,
           "SendMax" : {
              "currency" : "CNY",
              "issuer" : "razqQKzJRdB4UxFPWf5NEpEG3WMkmwgcXA",
              "value" : "5678900000000000e-4"
           },
           "Sequence" : 18,
           "TransactionType" : "Payment",
            })",
        "1200002280000000240000001261D684625103A72000000000000000000000"
        "00000055534400000000002ADB0B3959D60A6E6991F729E1918B7163925230"
        "68400000000000006469D7542CEDF137080000000000000000000000000043"
        "4E59000000000041C8BE2C0A6AA17471B9F6D0AF92AAB1C94D5A2581146033"
        "C369F0723DAE44A22957D7EF492CC5F80A2D8314B5F762798A53D543A014CA"
        "F8B297CFF8F2F937E8"
    };
    return testTx;
}

inline
TestItem
const& getKnownMetadata()
{
    // From tx FC8CB8BFE0BEE91BCC39BBB31827230BEDF273C300EC2F6DB212A31CA9CE7E94
    // in ledger 28812538 (chosen arbitrarily)
    static TestItem testMeta{
        R"({
            "AffectedNodes" : [
                {
                "CreatedNode" : {
                    "LedgerEntryType" : "Offer",
                    "LedgerIndex" : "7EAEE1B418DC7C00FC41E8DE6BA4FC0D79CD6F7476D44B3D99B2346F3A78FE96",
                    "NewFields" : {
                        "Account" : "rH3uSRUJYoJhK4kL9x1mzUhDimKE2n3oT6",
                        "BookDirectory" : "BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3BAC8E0468B",
                        "OwnerNode" : "00000000000000FB",
                        "Sequence" : 4330215,
                        "TakerGets" : "80000000000",
                        "TakerPays" : {
                            "currency" : "EUR",
                            "issuer" : "rhub8VRN55s94qWKDv6jmDy1pUykJzF3wq",
                            "value" : "2902.472875273122"
                        }
                    }
                }
                },
                {
                "DeletedNode" : {
                    "FinalFields" : {
                        "Account" : "rH3uSRUJYoJhK4kL9x1mzUhDimKE2n3oT6",
                        "BookDirectory" : "BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3CBADF66AFE",
                        "BookNode" : "0000000000000000",
                        "Flags" : 0,
                        "OwnerNode" : "00000000000000FB",
                        "PreviousTxnID" : "70F1E60F21B2A49ECBFE2D19E2867E28D7C8F210E111A58F1C68541F71FBCFE3",
                        "PreviousTxnLgrSeq" : 28812537,
                        "Sequence" : 4330209,
                        "TakerGets" : "80000000000",
                        "TakerPays" : {
                            "currency" : "EUR",
                            "issuer" : "rhub8VRN55s94qWKDv6jmDy1pUykJzF3wq",
                            "value" : "2902.530925601381"
                        }
                    },
                    "LedgerEntryType" : "Offer",
                    "LedgerIndex" : "8C2BEAAC384B373313F4E3E736A1C933B40E1AACB9D78B9F363BBF6D9A4CCA60"
                }
                },
                {
                "ModifiedNode" : {
                    "FinalFields" : {
                        "Account" : "rH3uSRUJYoJhK4kL9x1mzUhDimKE2n3oT6",
                        "Balance" : "104439515445",
                        "Flags" : 0,
                        "OwnerCount" : 23,
                        "Sequence" : 4330216
                    },
                    "LedgerEntryType" : "AccountRoot",
                    "LedgerIndex" : "94BAA26006FDE92FFDFD1EBBD162F38532C411004A04842C9DAC0E37FF24F371",
                    "PreviousFields" : {
                        "Balance" : "104439515731",
                        "Sequence" : 4330215
                    },
                    "PreviousTxnID" : "9CD12991D25C792CA14DE4FAFCA1E55A59CA82BA7F1E045BCC212807CCBCC928",
                    "PreviousTxnLgrSeq" : 28812537
                }
                },
                {
                "ModifiedNode" : {
                    "FinalFields" : {
                        "Flags" : 0,
                        "IndexPrevious" : "0000000000000000",
                        "Owner" : "rH3uSRUJYoJhK4kL9x1mzUhDimKE2n3oT6",
                        "RootIndex" : "FF060B902D93027D3C91161AB9534D1144ABAA309C2FD848783F5A3B9A11A80C"
                    },
                    "LedgerEntryType" : "DirectoryNode",
                    "LedgerIndex" : "A283077E7F53AEBE1EBE2FA6A5302C0594B34B6B6C04CB4972678B46F688756A"
                }
                },
                {
                "ModifiedNode" : {
                    "FinalFields" : {
                        "ExchangeRate" : "4D0CE3BAC8E0468B",
                        "Flags" : 0,
                        "RootIndex" : "BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3BAC8E0468B",
                        "TakerGetsCurrency" : "0000000000000000000000000000000000000000",
                        "TakerGetsIssuer" : "0000000000000000000000000000000000000000",
                        "TakerPaysCurrency" : "0000000000000000000000004555520000000000",
                        "TakerPaysIssuer" : "2ADB0B3959D60A6E6991F729E1918B7163925230"
                    },
                    "LedgerEntryType" : "DirectoryNode",
                    "LedgerIndex" : "BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3BAC8E0468B"
                }
                },
                {
                "DeletedNode" : {
                    "FinalFields" : {
                        "ExchangeRate" : "4D0CE3CBADF66AFE",
                        "Flags" : 0,
                        "RootIndex" : "BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3CBADF66AFE",
                        "TakerGetsCurrency" : "0000000000000000000000000000000000000000",
                        "TakerGetsIssuer" : "0000000000000000000000000000000000000000",
                        "TakerPaysCurrency" : "0000000000000000000000004555520000000000",
                        "TakerPaysIssuer" : "2ADB0B3959D60A6E6991F729E1918B7163925230"
                    },
                    "LedgerEntryType" : "DirectoryNode",
                    "LedgerIndex" : "BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3CBADF66AFE"
                }
                }
            ],
            "TransactionIndex" : 21,
            "TransactionResult" : "tesSUCCESS"
        })",
        "201C00000015F8E311006F567EAEE1B418DC7C00FC41E8DE6BA4FC0D79"
        "CD6F7476D44B3D99B2346F3A78FE96E824004212E73400000000000000"
        "FB5010BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0C"
        "E3BAC8E0468B64D54A4FC8A0B36BA20000000000000000000000004555"
        "5200000000002ADB0B3959D60A6E6991F729E1918B7163925230654000"
        "0012A05F20008114B100B28F60C3A425467387913CC0B297D3DA702CE1"
        "E1E411006F568C2BEAAC384B373313F4E3E736A1C933B40E1AACB9D78B"
        "9F363BBF6D9A4CCA60E7220000000024004212E12501B7A4F933000000"
        "00000000003400000000000000FB5570F1E60F21B2A49ECBFE2D19E286"
        "7E28D7C8F210E111A58F1C68541F71FBCFE35010BC05A0B94DB6C7C0B2"
        "D9E04573F0463DC15DB8033ABA85624D0CE3CBADF66AFE64D54A4FD624"
        "C5226500000000000000000000000045555200000000002ADB0B3959D6"
        "0A6E6991F729E1918B71639252306540000012A05F20008114B100B28F"
        "60C3A425467387913CC0B297D3DA702CE1E1E51100612501B7A4F9559C"
        "D12991D25C792CA14DE4FAFCA1E55A59CA82BA7F1E045BCC212807CCBC"
        "C9285694BAA26006FDE92FFDFD1EBBD162F38532C411004A04842C9DAC"
        "0E37FF24F371E624004212E7624000001851148A53E1E7220000000024"
        "004212E82D000000176240000018511489358114B100B28F60C3A42546"
        "7387913CC0B297D3DA702CE1E1E511006456A283077E7F53AEBE1EBE2F"
        "A6A5302C0594B34B6B6C04CB4972678B46F688756AE722000000003200"
        "0000000000000058FF060B902D93027D3C91161AB9534D1144ABAA309C"
        "2FD848783F5A3B9A11A80C8214B100B28F60C3A425467387913CC0B297"
        "D3DA702CE1E1E511006456BC05A0B94DB6C7C0B2D9E04573F0463DC15D"
        "B8033ABA85624D0CE3BAC8E0468BE72200000000364D0CE3BAC8E0468B"
        "58BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3BA"
        "C8E0468B0111000000000000000000000000455552000000000002112A"
        "DB0B3959D60A6E6991F729E1918B716392523003110000000000000000"
        "0000000000000000000000000411000000000000000000000000000000"
        "0000000000E1E1E411006456BC05A0B94DB6C7C0B2D9E04573F0463DC1"
        "5DB8033ABA85624D0CE3CBADF66AFEE72200000000364D0CE3CBADF66A"
        "FE58BC05A0B94DB6C7C0B2D9E04573F0463DC15DB8033ABA85624D0CE3"
        "CBADF66AFE011100000000000000000000000045555200000000000211"
        "2ADB0B3959D60A6E6991F729E1918B7163925230031100000000000000"
        "0000000000000000000000000004110000000000000000000000000000"
        "000000000000E1E1F1031000"
    };
    return testMeta;
}

} // test
} // serialize

#endif // !SERIALIZE_TESTS_KNOWNTX_H_INCLUDED
