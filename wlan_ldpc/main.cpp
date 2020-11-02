#include <iostream>

#include <cassert>

#include "wlan_ldpc.h"

int main(int argc, char **argv)
{
    // Codec
    WLANLDPC ldpc;

    // Load 648 block size, rate 1/2
    ldpc.LoadMatrix(0, 0);

    std::vector<uint8_t> msg(ldpc.MessageBits());
    std::vector<uint8_t> encoded(ldpc.CodewordBits());

    // Generate message
    for(uint8_t &b : msg) {
        b = rand() & 1;
    }

    // Generate codeword/encode
    ldpc.Encode(msg.data(), encoded.data());

    // Convert to soft decision BPSK
    std::vector<float> llr;
    for(uint8_t b : encoded) {
        llr.push_back(b == 0 ? 1.0 : -1.0);
    }

    // Flip some bits, create some errors
    for(int i = 0; i < 10; i++) {
        llr[rand() % llr.size()] *= -1.0;
    }

    // Decode codeword
    std::vector<uint8_t> decoded(ldpc.MessageBits());
    ldpc.Decode(llr.data(), 8, true, decoded.data());

    // Check for bit errors
    for(int i = 0; i < msg.size(); i++) {
        assert(msg[i] == decoded[i]);
    }

    return 0;
}
