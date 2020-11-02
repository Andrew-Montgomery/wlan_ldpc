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

#include "wlan_ldpc.h"
#include "wlan_ldpc_def.h"

#include <assert.h>

static void BinaryAdd(const uint8_t *src1, uint8_t *src2Dst, int len)
{
    assert(src1 && src2Dst && len > 0);
    if(!src1 || !src2Dst || len <= 0) {
        return;
    }

    for(int i = 0; i < len; i++) {
        src2Dst[i] = src2Dst[i] ^ src1[i];
    }
}

// Returns 1 for zero, for simplicity
static inline int Sign(float f)
{
    return (f >= 0.0) ? 1 : -1;
}

bool WLANLDPC::LoadMatrix(int blockSize, int rate)
{
    if(blockSize < 0 || blockSize >= 3) {
        assert(false);
        return false;
    }

    if(rate < 0 || rate >= 4) {
        assert(false);
        return false;
    }

    int *ptr;

    if(blockSize == 0) {
        z = 27;
        if(rate == 0) {
            ptr = (int*)H_648_1_2; rows = 12; cols = 24;
        } else if(rate == 1) {
            ptr = (int*)H_648_2_3; rows = 8; cols = 24;
        } else if(rate == 2) {
            ptr = (int*)H_648_3_4; rows = 6; cols = 24;
        } else {
            ptr = (int*)H_648_5_6; rows = 4; cols = 24;
        }
    } else if(blockSize == 1) {
        z = 54;
        if(rate == 0) {
            ptr = (int*)H_1296_1_2; rows = 12; cols = 24;
        } else if(rate == 1) {
            ptr = (int*)H_1296_2_3; rows = 8; cols = 24;
        } else if(rate == 2) {
            ptr = (int*)H_1296_3_4; rows = 6; cols = 24;
        } else {
            ptr = (int*)H_1296_5_6; rows = 4; cols = 24;
        }
    } else {
        z = 81;
        if(rate == 0) {
            ptr = (int*)H_1944_1_2; rows = 12; cols = 24;
        } else if(rate == 1) {
            ptr = (int*)H_1944_2_3; rows = 8; cols = 24;
        } else if(rate == 2) {
            ptr = (int*)H_1944_3_4; rows = 6; cols = 24;
        } else {
            ptr = (int*)H_1944_5_6; rows = 4; cols = 24;
        }
    }

    H.resize(rows);
    for(int i = 0; i < rows; i++) {
        H[i].resize(cols);
        for(int j = 0; j < cols; j++) {
            H[i][j] = ptr[i*cols + j];
        }
    }

    // Create column position and L matrix
    cp.resize(rows * z);
    L.resize(rows * z);
    for(int r = 0; r < rows; r++) {
        for(int c = 0; c < cols; c++) {
            if(H[r][c] != -1) {
                for(int zz = 0; zz < z; zz++) {
                    int pos = c * z + (H[r][c] + zz) % z;
                    cp[r*z + zz].push_back(pos);
                    L[r*z + zz].push_back(0.0);
                }
            }
        }
    }

    return true;
}

bool WLANLDPC::CheckCodeword(const uint8_t *codeWord)
{
    std::vector<uint8_t> work(rows * z, 0);
    std::vector<uint8_t> temp(z);

    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            Mul(&codeWord[j*z], &temp[0], z, H[i][j]);
            BinaryAdd(&temp[0], &work[i*z], z);
        }
    }

    // Return true if all zeros
    for(int i = 0; i < work.size(); i++) {
        if(work[i]) {
            return false;
        }
    }
    return true;
}

void WLANLDPC::Encode(const uint8_t *msg, uint8_t *codeWord)
{
    for(int i = 0; i < CodewordBits(); i++) {
        codeWord[i] = 0;
    }

    const int messageCols = cols - rows;
    const int messageBits = messageCols * z;

    // Copy message bits into codeword
    for(int i = 0; i < messageBits; i++) {
        codeWord[i] = msg[i];
    }

    // Sum all rows to find P1
    std::vector<uint8_t> temp1(z), temp2(z);
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < messageCols; j++) {
            Mul(&msg[j*z], &temp2[0], z, H[i][j]);
            BinaryAdd(&temp2[0], &temp1[0], z);
        }
    }

    // Now we have P1
    for(int i = 0; i < z; i++) {
        codeWord[messageBits + i] = temp1[i];
    }

    // Now solve the rest of the parity bits
    for(int i = 0; i < rows - 1; i++) {
        memset(&temp1[0], 0, temp1.size());
        for(int j = 0; j < messageCols + i + 1; j++) {
            Mul(&codeWord[j*z], &temp2[0], z, H[i][j]);
            BinaryAdd(&temp2[0], &temp1[0], z);
        }

        for(int j = 0; j < z; j++) {
            codeWord[messageBits + (i+1) * z + j] = temp1[j];
        }
    }
}

void WLANLDPC::Decode(const float *llr, int maxIters, bool earlyTerminate, uint8_t *msg)
{
    std::vector<float> R(cols * z);
    for(int i = 0; i < R.size(); i++) {
        R[i] = llr[i];
    }

    // Used to store final decisions and for early termination
    std::vector<uint8_t> codeWordTemp(cols * z);

    // Initialize the L matrix to zero for layered decoding
    for(int i = 0; i < cp.size(); i++) {
        for(int j = 0; j < cp[i].size(); j++) {
            L[i][j] = 0;
        }
    }

    // Main iteration loop
    for(int iter = 0; iter < maxIters; iter++) {
        // See if all parity checks are satisfied, if yes, return immediately
        if(earlyTerminate) {
            for(int i = 0; i < codeWordTemp.size(); i++) {
                codeWordTemp[i] = (R[i] < 0.0) ? 1 : 0;
            }

            if(CheckCodeword(&codeWordTemp[0])) {
                break;
            }
        }

        // Layered decoding, go through each layer
        for(int layer = 0; layer < rows; layer++) {
            // Subtract Layer from LLR, set LLR and layer to this value
            for(int lz = 0; lz < z; lz++) {
                int r = layer * z + lz;
                for(int c = 0; c < cp[r].size(); c++) {
                    L[r][c] = R[cp[r][c]] - L[r][c];
                    R[cp[r][c]] = L[r][c];
                }
            }

            // Row operations, perform minsum for all rows in layer.
            for(int lz = 0; lz < z; lz++) {
                int r = layer * z + lz;
                OffsetMinSum(&L[r][0], L[r].size());
            }

            // Column operations
            for(int lz = 0; lz < z; lz++) {
                int r = layer * z + lz;
                for(int i = 0; i < cp[r].size(); i++) {
                    int pos = cp[r][i];
                    R[pos] += L[r][i];
                }
            }
        }
    }
    // End iteration section

    // Decisions
    const int msgBits = MessageBits();
    for(int i = 0; i < msgBits; i++) {
        msg[i] = (R[i] < 0.0) ? 1 : 0;
    }
}

void WLANLDPC::Mul(const uint8_t *src, uint8_t *dst, int z, int k)
{
    assert(src && dst && k >= -1 && z >= 1);
    if(!src || !dst || k < -1 || z < 1) {
        return;
    }

    if(k == -1) {
        for(int i = 0; i < z; i++) {
            dst[i] = 0;
        }
        return;
    }

    for(int i = 0; i < z; i++) {
        dst[i] = src[(i+k) % z];
    }
}

void WLANLDPC::OffsetMinSum(float *f, int len)
{
    assert(len < 24);

    // Maximum number of values to be processed for any given WLAN LDPC matrix is 24
    // absolute values of f
    float fa[24];
    // signs of f
    int fs[24];

    // Get sign parity
    int s = 1.0;
    for(int i = 0; i < len; i++) {
        fa[i] = fabs(f[i]);
        fs[i] = Sign(f[i]);
        s *= fs[i];
    }

    // Find minimum value and position
    float min1 = fa[0];
    int min1Pos = 0;

    for(int i = 1; i < len; i++) {
        if(fa[i] < min1) {
            min1 = fa[i];
            min1Pos = i;
        }
    }

    fa[min1Pos] = FLT_MAX;
    float min2 = fa[0];

    // Find next minimum
    for(int i = 1; i < len; i++) {
        if(fa[i] < min2) {
            min2 = fa[i];
        }
    }

    // Offset minsum impl.
    min1 -= 0.5;
    if(min1 < 0.0) {
        min1 = 0.0;
    }

    min2 -= 0.5;
    if(min2 < 0.0) {
        min2 = 0.0;
    }

    // Now set all values but the minimum to min1,
    //   then set the min val position to min2
    for(int i = 0; i < len; i++) {
        f[i] = s * fs[i] * min1;
    }
    f[min1Pos] = s * fs[min1Pos] * min2;
}
