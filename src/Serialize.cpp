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

#include <Serialize.h>

#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/strHex.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/Sign.h>
#include <boost/filesystem.hpp>
#include <fstream>

namespace offline {

Json::Value
parseJson(std::string const& raw)
{
    using namespace ripple;

    Json::Value jv;
    Json::Reader{}.parse(raw, jv);

    return jv;
}

std::optional<ripple::STObject>
makeObject(Json::Value const& json)
{
    using namespace ripple;

    STParsedJSONObject parsed("", json);

    return parsed.object;
}

std::string
serialize(ripple::STObject const& object)
{
    using namespace ripple;

    return strHex(object.getSerializer().peekData());
}

std::optional<ripple::STObject>
deserialize(std::string const& blob)
{
    using namespace ripple;

    auto const unhex{strUnHex(blob)};

    if (!unhex || !unhex->size())
        return {};

    SerialIter sitTrans{makeSlice(*unhex)};
    // Can Throw
    return STObject{std::ref(sitTrans), sfGeneric};
}

ripple::STTx
make_sttx(std::string const& data)
{
    std::optional<ripple::STObject> obj;
    try
    {
        obj = deserialize(data);
    }
    catch (std::exception const& e)
    {
        auto msg =
            std::string{"unable to deserialize (internal: "} + e.what() + ")";
        throw std::runtime_error(msg);
    }
    if (!obj)
    {
        auto const json = offline::parseJson(data);
        if (json)
            obj = offline::makeObject(json);
        else
            throw std::runtime_error("invalid JSON");
    }

    using namespace ripple;

    obj->makeFieldPresent(sfSigningPubKey);
    // Can Throw
    return STTx{std::move(*obj)};
}

}  // namespace offline
