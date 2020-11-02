// Copyright (c) 2020 Andrew Montgomery

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <vector>

class WLANLDPC {
public:
    // blockSize = 0,1,2, for the 3 block lengths 648,1296,1944
    // rate = 0,1,2,3, for the 4 rates, 1/2, 2/3, 3/4, 5/6
    bool LoadMatrix(int blockSize, int rate);

    int Rows() const { return rows; }
    int Columns() const { return cols; }
    // Number of input bits into the encoder
    int MessageBits() const { return (cols - rows) * z; }
    int CodewordBits() const { return cols * z; }

    // Verify that H*C^T = 0, alternatively, that c is a valid codeword
    // z = expansion factor
    // Returns true if c is a valid codeword
    // Codeword should be codeword bits long
    bool CheckCodeword(const uint8_t *codeWord);
    // Input msg should be MessageBits long, codeWord should be CodewordBits long
    void Encode(const uint8_t *msg, uint8_t *codeWord);
    // Uses minsum message passing algorithm
    // llr is the received message LLR
    // Specify number of iterations through message passing algorithm
    // If earlyTerminate, return when all parity checks are satisfied, or when
    //    maxIters is reached
    // msg should be equal to MessageBits()
    void Decode(const float *llr, int maxIters, bool earlyTerminate, uint8_t *msg);

private:
    // Multiplies input bits by identity matrix shifted k
    // z = expansion factor, also the length of src/dst
    // If k == -1, returns all zeros
    // If k == 0, multiplies by identity matrix
    // k >= 1, multiplies by shifted identity matrix with assumed expansion factor
    //   equal to the length of the input
    void Mul(const uint8_t *src, uint8_t *dst, int z, int k);
    // Inplace offset minsum algorithm using 0.5 offset
    void OffsetMinSum(float *f, int len);

    // Matrix prototype
    std::vector<std::vector<int>> H;
    // Prototype matrix dimensions
    int rows;
    int cols;
    // Subblock size
    int z;

    // Column positions, for the decoder
    // One vector for each row (including subblocks)
    // Each value is the column index of all non-zero entries in the expanded matrix
    std::vector<std::vector<int>> cp;
    // L, just the locations that have non-zero values
    // The values corresponding to the indices above in cp
    std::vector<std::vector<float>> L;
};

