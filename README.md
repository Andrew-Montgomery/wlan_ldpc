# wlan_ldpc
Reference implementation of an encoder/decoder for WLAN LDPC codes.

* Written in C++ with example usage an QtCreator project file.
* Encoding and decoding.
* Decoding features
  * Soft decision
  * Offset minsum approximation
  * Layered
  * Early termination 
* Supports all 12 base matrices defined in Annex F in 80211-2016.
* Compiled on Windows with the VS2012 x64 compiler.

Performance
![BPSK BER Performance](./bpsk.png)

References
* LDPC and Polar Codes in 5G Standard, NPTEL-NOC_IITM, https://www.youtube.com/playlist?list=PLyqSpQzTE6M81HJ26ZaNv0V3ROBrcv-Kc
* IEEE Std 802.11-2016

