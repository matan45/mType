#include "Sha256.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace packagemanager
{
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t Sha256::rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
    uint32_t Sha256::ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    uint32_t Sha256::maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    uint32_t Sha256::sigma0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    uint32_t Sha256::sigma1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    uint32_t Sha256::gamma0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    uint32_t Sha256::gamma1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    Sha256::Sha256()
    {
        state[0] = 0x6a09e667; state[1] = 0xbb67ae85;
        state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
        state[4] = 0x510e527f; state[5] = 0x9b05688c;
        state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
        bitCount = 0;
        bufferLen = 0;
        std::memset(buffer, 0, sizeof(buffer));
    }

    void Sha256::processBlock(const uint8_t block[64])
    {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
        {
            w[i] = (uint32_t(block[i * 4]) << 24) |
                   (uint32_t(block[i * 4 + 1]) << 16) |
                   (uint32_t(block[i * 4 + 2]) << 8) |
                   uint32_t(block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i)
        {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; ++i)
        {
            uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    void Sha256::update(const uint8_t* data, size_t length)
    {
        for (size_t i = 0; i < length; ++i)
        {
            buffer[bufferLen++] = data[i];
            if (bufferLen == 64)
            {
                processBlock(buffer);
                bitCount += 512;
                bufferLen = 0;
            }
        }
    }

    std::string Sha256::finalize()
    {
        bitCount += bufferLen * 8;

        buffer[bufferLen++] = 0x80;
        if (bufferLen > 56)
        {
            while (bufferLen < 64) buffer[bufferLen++] = 0;
            processBlock(buffer);
            bufferLen = 0;
        }
        while (bufferLen < 56) buffer[bufferLen++] = 0;

        for (int i = 7; i >= 0; --i)
        {
            buffer[56 + (7 - i)] = static_cast<uint8_t>(bitCount >> (i * 8));
        }
        processBlock(buffer);

        std::ostringstream result;
        result << std::hex << std::setfill('0');
        for (int i = 0; i < 8; ++i)
        {
            result << std::setw(8) << state[i];
        }
        return result.str();
    }

    std::string Sha256::hash(const std::string& data)
    {
        Sha256 hasher;
        hasher.update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        return hasher.finalize();
    }

    std::string Sha256::hashFile(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            return "";
        }

        Sha256 hasher;
        char buf[4096];
        while (file.read(buf, sizeof(buf)))
        {
            hasher.update(reinterpret_cast<const uint8_t*>(buf), file.gcount());
        }
        if (file.gcount() > 0)
        {
            hasher.update(reinterpret_cast<const uint8_t*>(buf), file.gcount());
        }

        return hasher.finalize();
    }

    std::string Sha256::hashDirectory(const std::string& dirPath)
    {
        namespace fs = std::filesystem;

        // Collect all file paths, sorted for determinism
        std::vector<std::string> files;
        for (const auto& entry : fs::recursive_directory_iterator(dirPath))
        {
            if (entry.is_regular_file())
            {
                files.push_back(fs::relative(entry.path(), dirPath).string());
            }
        }
        std::sort(files.begin(), files.end());

        Sha256 hasher;
        for (const auto& relPath : files)
        {
            // Hash filename + content for each file
            hasher.update(reinterpret_cast<const uint8_t*>(relPath.data()), relPath.size());

            std::ifstream file(fs::path(dirPath) / relPath, std::ios::binary);
            char buf[4096];
            while (file.read(buf, sizeof(buf)))
            {
                hasher.update(reinterpret_cast<const uint8_t*>(buf), file.gcount());
            }
            if (file.gcount() > 0)
            {
                hasher.update(reinterpret_cast<const uint8_t*>(buf), file.gcount());
            }
        }

        return hasher.finalize();
    }
}
