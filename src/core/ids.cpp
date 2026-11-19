// SPDX-License-Identifier: GPL-3.0-or-later
#include "tratto/core/ids.h"

#include <QDateTime>
#include <QRandomGenerator>

#include <array>

namespace tratto::core {

QUuid newId()
{
    // 48-bit Unix time in ms, version 7, then 74 random bits with the RFC variant.
    const auto ms = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    std::array<quint32, 3> random { };
    QRandomGenerator::system()->fillRange(random.data(), random.size());

    QUuid::Id128Bytes bytes { };
    for (int i = 0; i < 6; ++i)
        bytes.data[i] = static_cast<quint8>(ms >> (8 * (5 - i)));
    for (int i = 6; i < 16; ++i)
        bytes.data[i] = static_cast<quint8>(random[static_cast<std::size_t>((i - 6) / 4)]
                                            >> (8 * ((i - 6) % 4)));
    bytes.data[6] = static_cast<quint8>(0x70 | (bytes.data[6] & 0x0F)); // version 7
    bytes.data[8] = static_cast<quint8>(0x80 | (bytes.data[8] & 0x3F)); // RFC 9562 variant
    return QUuid(bytes);
}

} // namespace tratto::core
