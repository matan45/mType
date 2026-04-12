#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace packagemanager
{
    class Sha256
    {
    public:
        static std::string hash(const std::string& data);

        static std::string hashFile(const std::string& filePath);

        static std::string hashDirectory(const std::string& dirPath);

    private:
        uint32_t state[8];
        uint64_t bitCount;
        uint8_t buffer[64];
        size_t bufferLen;

        Sha256();
        void update(const uint8_t* data, size_t length);
        std::string finalize();
        void processBlock(const uint8_t block[64]);

        static uint32_t rotr(uint32_t x, int n);
        static uint32_t ch(uint32_t x, uint32_t y, uint32_t z);
        static uint32_t maj(uint32_t x, uint32_t y, uint32_t z);
        static uint32_t sigma0(uint32_t x);
        static uint32_t sigma1(uint32_t x);
        static uint32_t gamma0(uint32_t x);
        static uint32_t gamma1(uint32_t x);
    };
}
