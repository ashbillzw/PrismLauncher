// SPDX-License-Identifier: GPL-3.0-only
#include "FtbMapMerge.h"
#include "archive/ArchiveReader.h"
#include "archive/ArchiveWriter.h"
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QMap>
#include <QtEndian>
#include <array>
#include <optional>

namespace {
using Bytes = QByteArray;
using Remap = QHash<int, int>;

std::optional<Bytes> read(const QString& path)
{
    QFile f(path);
    if (!f.exists()) return Bytes{};
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto data = f.readAll();
    if (f.error() != QFileDevice::NoError) return {};
    return data;
}

// All writes here target the caller's disposable staging directory.
bool write(const QString& path, const Bytes& data)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(data) == data.size() && f.flush();
}

bool mergePalette(const QString& from, const QString& to, int digits, int maximum, Remap& remap)
{
    QMap<int, Bytes> names;
    QHash<Bytes, int> ids;
    auto target = read(to), source = read(from);
    if (!source || !target) return false;
    Bytes additions;
    int freeId = 1;
    for (const auto* table : {&*target, &*source}) {
        for (auto line : table->split('\n')) {
            line = line.trimmed();
            if (line.isEmpty()) continue;
            bool ok;
            int id = line.mid(1, digits).toInt(&ok, 16);
            if (!ok || line[0] != '#' || line.size() <= digits + 2 || line[digits+1] != ' ' || id > maximum) return false;
            auto name = line.mid(digits+2);
            if (table == &*target) {
                names.insert(id, name);
                ids.insert(name, id);
                continue;
            }
            int mapped = ids.value(name, -1);
            if (mapped < 0) {
                mapped = id;
                if (names.contains(id)) {
                    while (names.contains(freeId)) ++freeId;
                    if (freeId > maximum) return false;
                    mapped = freeId++;
                }
                names.insert(mapped, name);
                ids.insert(name, mapped);
                additions += '#' + Bytes::number(mapped,16).toUpper().rightJustified(digits,'0') + ' ' + name + '\n';
            }
            if (mapped != id) remap.insert(id, mapped);
        }
    }
    if (additions.isEmpty()) return true;
    if (!target->isEmpty() && !target->endsWith('\n')) target->append('\n');
    return write(to, *target + additions);
}

constexpr quint32 noHeight = 0x8001;
constexpr quint32 biomeMask = 0x7ff;
constexpr quint32 blockMask = 0xffffff;
constexpr std::array<const char*,5> layers{"data.png","foliage.png","grass.png","water.png","blocks.png"};
struct Region {
    std::array<QImage,5> images;
    QMap<int,Bytes> chunks;
};

bool loadRegion(const QString& path, Region& region)
{
    MMCZip::ArchiveReader zip(path, true);
    auto chunks = zip.readFile("chunks.dat");
    if (!chunks || chunks->size() < 4 || chunks->at(0) != 0) return false;
    int version = chunks->at(1);
    if (version < 1 || version > 3) return false;
    const int length = version == 1 ? 10 : 11;
    int count = qFromBigEndian<quint16>(chunks->constData()+2);
    if (count > 1024 || chunks->size() != 4 + count*length) return false;
    for (int i=0; i<count; ++i) {
        auto record = chunks->mid(4+i*length, length);
        if (version == 1) record.prepend(char(0));
        const int x = quint8(record[1]), z = quint8(record[2]);
        if (x >= 32 || z >= 32) return false;
        region.chunks.insert(z*32+x, record);
    }
    for (size_t i=0; i<layers.size(); ++i) {
        auto bytes = zip.readFile(layers[i]);
        if (!bytes) return false;
        auto img = QImage::fromData(*bytes,"PNG");
        if (img.size() != QSize(512,512)) return false;
        region.images[i] = img.convertToFormat(i == 0 ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    }
    if (version < 3) {
        // The FTB reader upgrades the original height-zero sentinel on load.
        for (int y=0; y<512; ++y) {
            auto* row = reinterpret_cast<QRgb*>(region.images[0].scanLine(y));
            for (int x=0; x<512; ++x)
                if ((row[x] >> 16) == 0) row[x] |= noHeight << 16;
        }
    }
    return true;
}

bool saveRegion(const QString& path, const Region& region)
{
    MMCZip::ArchiveWriter zip(path);
    if (!zip.open()) return false;
    Bytes records(4, 0);
    records[1] = 3;
    qToBigEndian<quint16>(region.chunks.size(), records.data()+2);
    for (const auto& rec : region.chunks) records += rec;
    if (!zip.addFile("chunks.dat",records)) return false;
    for (size_t i=0; i<layers.size(); ++i) {
        QBuffer bytes;
        if (!bytes.open(QIODevice::WriteOnly) || !region.images[i].save(&bytes,"PNG") || !zip.addFile(layers[i],bytes.data())) return false;
    }
    return bool(zip.close());
}

bool mergeRegion(const QString& from, const QString& to, const Remap& blocks, const Remap& biomes)
{
    const bool overlap = QFile::exists(to);
    if (!overlap && blocks.isEmpty() && biomes.isEmpty()) return QFile::copy(from,to);
    Region old, dest;
    if (!loadRegion(from,old) || (overlap && !loadRegion(to,dest))) return false;
    if (!overlap) dest = old;
    bool changed = !overlap;
    for (auto it=old.chunks.cbegin(); it!=old.chunks.cend(); ++it) {
        if (!dest.chunks.contains(it.key())) {
            dest.chunks.insert(it.key(), it.value());
            changed = true;
        }
        const int left = (it.key()%32)*16, top = (it.key()/32)*16;
        for (int y=top; y<top+16; ++y) {
            for (int x=left; x<left+16; ++x) {
                auto data = old.images[0].pixel(x,y);
                if (data >> 16 == noHeight || (overlap && dest.images[0].pixel(x,y) >> 16 != noHeight)) continue;
                const int biome = data & biomeMask;
                dest.images[0].setPixel(x,y,(data & ~biomeMask) | biomes.value(biome,biome));
                for (size_t i=1; i<4; ++i) dest.images[i].setPixel(x,y,old.images[i].pixel(x,y));
                int block = old.images[4].pixel(x,y) & blockMask;
                dest.images[4].setPixel(x,y,0xff000000u | blocks.value(block,block));
                changed = true;
            }
        }
    }
    return !changed || saveRegion(to,dest);
}

bool mergeRegionIndex(const QString& from, const QString& to)
{
    if (!QFile::exists(to)) return QFile::copy(from,to);
    QSet<Bytes> seen;
    Bytes records;
    for (const auto& path : {to, from}) {
        auto packed = read(path);
        if (!packed) return false;
        // FTB uses zlib without Qt's four-byte size prefix.
        auto data = qUncompress(Bytes::fromHex("00010002") + *packed);
        if (data.size() < 4 || data.first(2) != Bytes::fromHex("0001") ||
            data.size() != 4 + 2*qFromBigEndian<quint16>(data.constData()+2)) return false;
        for (int pos=4; pos<data.size(); pos+=2) {
            auto coord = data.mid(pos,2);
            if (!seen.contains(coord)) {
                seen.insert(coord);
                records += coord;
            }
        }
    }
    if (seen.size() > 32767) return false;
    Bytes header = Bytes::fromHex("00010000");
    qToBigEndian<quint16>(seen.size(),header.data()+2);
    return write(to,qCompress(header+records).mid(4));
}
}

bool Niijima::mergeFtbWorld(const QDir& old, const QDir& dest)
{
    Remap blocks, biomes;
    const auto entries = QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot;
    for (const auto& item : old.entryInfoList(entries))
        if (item.isSymLink() || item.isJunction()) return false;
    if (!mergePalette(old.filePath("block_map.txt"), dest.filePath("block_map.txt"),6,blockMask,blocks) ||
        !mergePalette(old.filePath("biome_map.txt"), dest.filePath("biome_map.txt"),3,biomeMask,biomes)) return false;
    for (const auto& item : old.entryInfoList(entries)) {
        const auto name = item.fileName(), target = dest.filePath(name);
        if (name == "dimensions.txt") {
            auto a = read(target), b = read(item.filePath());
            if (!a || !b) return false;
            auto lines = a->split('\n');
            for (auto& line : lines) line = line.trimmed();
            lines.removeAll({});
            for (auto line : b->split('\n')) {
                line = line.trimmed();
                if (!line.isEmpty() && !lines.contains(line)) lines.append(line);
            }
            if (!write(target,lines.join('\n')+'\n')) return false;
        } else if (item.isDir()) {
            if (!dest.mkpath(name)) return false;
            const QDir dimension(item.filePath()), out(target);
            for (const auto& f : dimension.entryInfoList(entries)) {
                const auto t = out.filePath(f.fileName());
                if (!f.isFile() || f.isSymLink() || f.isJunction()) return false;
                if (f.suffix() == "zip") {
                    if (!mergeRegion(f.filePath(),t,blocks,biomes)) return false;
                } else if (f.fileName() == "dimension.regions") {
                    if (!mergeRegionIndex(f.filePath(),t)) return false;
                } else if (f.fileName() == "waypoints.json" || !QFile::exists(t)) {
                    if (QFile::exists(t) && !QFile::remove(t)) return false;
                    if (!QFile::copy(f.filePath(),t)) return false;
                }
            }
        } else if (name != "block_map.txt" && name != "biome_map.txt" && !QFile::exists(target) && !QFile::copy(item.filePath(),target)) {
            return false;
        }
    }
    return true;
}
