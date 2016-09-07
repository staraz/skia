/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkFontDescriptor.h"
#include "SkStream.h"
#include "SkData.h"

enum {
    // these must match the sfnt 'name' enums
    kFontFamilyName = 0x01,
    kFullName       = 0x04,
    kPostscriptName = 0x06,

    // These count backwards from 0xFF, so as not to collide with the SFNT
    // defines for names in its 'name' table.
    kCacheIndex     = 0xFB,
    kFontAxes       = 0xFC,
    kFontIndex      = 0xFD,
    kFontFileName   = 0xFE,  // Remove when MIN_PICTURE_VERSION > 41
    kSentinel       = 0xFF,
};
std::unordered_map<std::string, SkFontDescriptor> SkFontDescriptor::fSerializeCache;

static void read_string(SkStream* stream, SkString* string) {
    const uint32_t length = SkToU32(stream->readPackedUInt());
    if (length > 0) {
        string->resize(length);
        stream->read(string->writable_str(), length);
    }
}

// Remove when MIN_PICTURE_VERSION > 41
static void skip_string(SkStream* stream) {
    const uint32_t length = SkToU32(stream->readPackedUInt());
    if (length > 0) {
        stream->skip(length);
    }
}

static void write_string(SkWStream* stream, const SkString& string, uint32_t id) {
    if (!string.isEmpty()) {
        stream->writePackedUInt(id);
        stream->writePackedUInt(string.size());
        stream->write(string.c_str(), string.size());
    }
}

static size_t read_uint(SkStream* stream) {
    return stream->readPackedUInt();
}

static void write_uint(SkWStream* stream, size_t n, uint32_t id) {
    stream->writePackedUInt(id);
    stream->writePackedUInt(n);
}

SkFontDescriptor::SkFontDescriptor(const SkFontDescriptor& other) {
    *this = other;
}

SkFontDescriptor& SkFontDescriptor::operator=(const SkFontDescriptor& other) {
    this->fFamilyName = other.fFamilyName;
    this->fFullName = other.fFullName;
    this->fPostscriptName = other.fPostscriptName;
    if (other.hasFontData())
        this->fFontData.reset(new SkFontData(*other.fFontData.get()));

    this->fStyle = other.fStyle;
    return *this;
}

bool SkFontDescriptor::Deserialize(SkStream* stream, SkFontDescriptor* result) {
    static std::unordered_map<std::string, SkFontDescriptor> deserializeCache;
    size_t styleBits = stream->readPackedUInt();
    if (styleBits <= 2) {
        // Remove this branch when MIN_PICTURE_VERSION > 45
        result->fStyle = SkFontStyle::FromOldStyle(styleBits);
    } else {
        result->fStyle = SkFontStyle((styleBits >> 16) & 0xFFFF,
                                     (styleBits >> 8 ) & 0xFF,
                                     static_cast<SkFontStyle::Slant>(styleBits & 0xFF));
    }

    SkAutoSTMalloc<4, SkFixed> axis;
    size_t axisCount = 0;
    size_t index = 0;
    for (size_t id; (id = stream->readPackedUInt()) != kSentinel;) {
        switch (id) {
            case kCacheIndex:
            {
                SkString key;
                read_string(stream, &key);
                auto search = deserializeCache.find(std::string(key.c_str()));
                if (search != deserializeCache.end()) {
                    // Retrieve SkFontDescriptor from the cache
                    *result = search->second;
                    return true;
                }
                SkDEBUGFAIL("SkFontDescriptor not found in cache!");
                return false;
                break;
            }
            // Read the SkFontDescptor from stream and add it to cache
            case kFontFamilyName:
                read_string(stream, &result->fFamilyName);
                break;
            case kFullName:
                read_string(stream, &result->fFullName);
                break;
            case kPostscriptName:
                read_string(stream, &result->fPostscriptName);
                break;
            case kFontAxes:
                axisCount = read_uint(stream);
                axis.reset(axisCount);
                for (size_t i = 0; i < axisCount; ++i) {
                    axis[i] = read_uint(stream);
                }
                break;
            case kFontIndex:
                index = read_uint(stream);
                break;
            case kFontFileName:  // Remove when MIN_PICTURE_VERSION > 41
                skip_string(stream);
                break;
            default:
                SkDEBUGFAIL("Unknown id used by a font descriptor");
                printf("Invalid ID!\n");
                return false;
        }
    }
    size_t length = stream->readPackedUInt();
    if (length > 0) {
        sk_sp<SkData> data(SkData::MakeUninitialized(length));
        if (stream->read(data->writable_data(), length) == length) {
            result->fFontData.reset(new SkFontData(new SkMemoryStream(data),
                                                   index, axis, axisCount));
        } else {
            SkDEBUGFAIL("Could not read font data");
            return false;
        }
    }
    // Add result in cache
    deserializeCache[std::string(GetKey(*result, styleBits).c_str())] = *result;
    return true;
}

// TODO(staraz): Find the source of the the anonymous SkFontDescriptors
void SkFontDescriptor::serialize(SkWStream* stream) {
    uint32_t styleBits = (fStyle.weight() << 16) | (fStyle.width() << 8) | (fStyle.slant());
    stream->writePackedUInt(styleBits);
    SkString key = GetKey(*this, styleBits);
    auto search = fSerializeCache.find(key.c_str());
    if (search == fSerializeCache.end()) {
        // Font hasn't been cached yet. Serialize font as usual.
        write_string(stream, fFamilyName, kFontFamilyName);
        write_string(stream, fFullName, kFullName);
        write_string(stream, fPostscriptName, kPostscriptName);
        if (fFontData.get()) {
            if (fFontData->getIndex()) {
                write_uint(stream, fFontData->getIndex(), kFontIndex);
            }
            if (fFontData->getAxisCount()) {
                write_uint(stream, fFontData->getAxisCount(), kFontAxes);
                for (int i = 0; i < fFontData->getAxisCount(); ++i) {
                    stream->writePackedUInt(fFontData->getAxis()[i]);
                }
            }
        }

        stream->writePackedUInt(kSentinel);

        if (fFontData.get() && fFontData->hasStream()) {
            SkAutoTDelete<SkStreamAsset> fontData(fFontData->detachStream());
            size_t length = fontData->getLength();
            stream->writePackedUInt(length);
            stream->writeStream(fontData, length);
        } else {
            stream->writePackedUInt(0);
        }
        // Generate cache key
        fSerializeCache[std::string(key.c_str())] = *this;
    } else {
        // Font is already cached. Serialize the key only
        write_string(stream, key, kCacheIndex);
    }
}

// static
SkString SkFontDescriptor::GetKey(const SkFontDescriptor& desc, uint32_t styleBits) {
    SkString skst;
    skst.append(desc.fFamilyName);
    skst.appendU32(styleBits);

    return skst;
}
