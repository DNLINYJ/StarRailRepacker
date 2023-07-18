#pragma once
#include <cstddef>
void Decrypt(std::byte* bytes, unsigned int size);
std::byte* Encrypt(std::byte* bytes, unsigned int size);
std::byte* Encrypt(std::byte* bytes, unsigned int size, uint8_t* key3);