#pragma once
#include <xxhash.h>
#include <zstd.h>

#include "../Dependecies/Luau/Compiler.h"
#include "../Dependecies/Luau/BytecodeBuilder.h"
#include "../Dependecies/Luau/BytecodeUtils.h"

extern "C" {
#include "../Dependecies/blake3/blake3.h"
}

class BytecodeEncoder : public Luau::BytecodeEncoder {
    inline void encode(uint32_t* Dataaa, size_t Counntt) override {
        for (auto i = 0u; i < Counntt;) {
            auto& opcode = *reinterpret_cast<uint8_t*>(Dataaa + i);
            i += Luau::getOpLength(LuauOpcode(opcode));
            opcode *= 227;
        }
    }
};

static constexpr uint32_t MAGIC_A = 0x4C464F52;
static constexpr uint32_t MAGIC_B = 0x946AC432;
static constexpr uint8_t  KEY_BYTES[4] = { 0x52, 0x4F, 0x46, 0x4C };

static inline uint8_t rotl8(uint8_t value, int shift) {
    shift &= 7;
    return (value << shift) | (value >> (8 - shift));
}

namespace Bytecode {
    inline std::vector<char> Compress(const std::string& bytecode, size_t& byte_size) {
        const auto data_size = bytecode.size();
        const auto max_size = ZSTD_compressBound(data_size);
        auto buffer = std::vector<char>(max_size + 8);

        buffer[0] = 'R'; buffer[1] = 'S'; buffer[2] = 'B'; buffer[3] = '1';
        std::memcpy(&buffer[4], &data_size, sizeof(data_size));

        const auto compressed_size = ZSTD_compress(&buffer[8], max_size, bytecode.data(), data_size, ZSTD_maxCLevel());
        const auto size = compressed_size + 8;
        const auto key = XXH32(buffer.data(), size, 42u);
        const auto bytes = reinterpret_cast<const uint8_t*>(&key);

        for (auto i = 0u; i < size; ++i) {
            buffer[i] ^= bytes[i % 4] + i * 41u;
        }

        byte_size = size;
        buffer.resize(size);

        return buffer;
    }

    inline std::string Compile(const std::string& Source)
    {
        static BytecodeEncoder encoder = BytecodeEncoder();
        const std::string bytecode = Luau::compile(Source, {}, {}, &encoder);
        if (bytecode[0] == '\0') {
            std::string bytecodeP = bytecode;
            bytecodeP.erase(std::remove(bytecodeP.begin(), bytecodeP.end(), '\0'), bytecodeP.end());
            return "";
        }
        return bytecode;
    }

    inline std::string NormalCompile(const std::string& Source)
    {
        const std::string bytecode = Luau::compile(Source, {}, {}, nullptr);
        if (bytecode[0] == '\0') {
            std::string bytecodeP = bytecode;
            bytecodeP.erase(std::remove(bytecodeP.begin(), bytecodeP.end(), '\0'), bytecodeP.end());
            return "";
        }
        return bytecode;
    }

    inline static std::vector<char> Sign(const std::string& bytecode, size_t& s) {
        if (bytecode.empty()) {
            return std::vector<char>();
        }
        constexpr uint32_t FOOTER_SIZE = 40u;
        std::vector<uint8_t> blake3_hash(32);
        {
            blake3_hasher hasher;
            blake3_hasher_init(&hasher);
            blake3_hasher_update(&hasher, bytecode.data(), bytecode.size());
            blake3_hasher_finalize(&hasher, blake3_hash.data(), blake3_hash.size());
        }
        std::vector<uint8_t> transformed_hash(32);
        for (int i = 0; i < 32; ++i) {
            uint8_t byte = KEY_BYTES[i & 3];
            uint8_t hash_byte = blake3_hash[i];
            uint8_t combined = byte + i;
            uint8_t result;
            switch (i & 3) {
            case 0: {
                int shift = ((combined & 3) + 1);
                result = rotl8(hash_byte ^ ~byte, shift);
                break;
            }
            case 1: {
                int shift = ((combined & 3) + 2);
                result = rotl8(byte ^ ~hash_byte, shift);
                break;
            }
            case 2: {
                int shift = ((combined & 3) + 3);
                result = rotl8(hash_byte ^ ~byte, shift);
                break;
            }
            case 3: {
                int shift = ((combined & 3) + 4);
                result = rotl8(byte ^ ~hash_byte, shift);
                break;
            }
            }
            transformed_hash[i] = result;
        }
        std::vector<uint8_t> footer(FOOTER_SIZE, 0);
        uint32_t first_hash_dword = *reinterpret_cast<uint32_t*>(transformed_hash.data());
        uint32_t footer_prefix = first_hash_dword ^ MAGIC_B;
        memcpy(&footer[0], &footer_prefix, 4);
        uint32_t xor_ed = first_hash_dword ^ MAGIC_A;
        memcpy(&footer[4], &xor_ed, 4);
        memcpy(&footer[8], transformed_hash.data(), 32);
        std::string signed_bytecode = bytecode;
        signed_bytecode.append(reinterpret_cast<const char*>(footer.data()), footer.size());
        return Compress(signed_bytecode, s);
    }
}