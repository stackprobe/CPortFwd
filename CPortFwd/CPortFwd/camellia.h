/* camellia.h ver 1.2.0
 *
 * Copyright (c) 2006,2007
 * NTT (Nippon Telegraph and Telephone Corporation) . All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer as
 *   the first lines of this file unmodified.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NTT ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NTT BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// #pragma once
// #ifndef HEADER_CAMELLIA_H
// #define HEADER_CAMELLIA_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef unsigned int camellia_KeyTableType[272 / 4];


void camellia_Ekeygen(const int keyBitLength,
		      const unsigned char *rawKey,
		      camellia_KeyTableType keyTable);

void camellia_EncryptBlock(const int keyBitLength,
			   const unsigned char *plaintext,
			   const camellia_KeyTableType keyTable,
			   unsigned char *cipherText);

void camellia_DecryptBlock(const int keyBitLength,
			   const unsigned char *cipherText,
			   const camellia_KeyTableType keyTable,
			   unsigned char *plaintext);


#ifdef  __cplusplus
}
#endif

// #endif /* HEADER_CAMELLIA_H */

// <-- original code

typedef struct camellia_st
{
	camellia_KeyTableType KeyTable;
	int KeyBitLength;
}
camellia_t;

camellia_t *camellia_create(uchar *rawKey, int rawKeySize);
void camellia_release(camellia_t *i);
void camellia_crypt(camellia_t *i, uchar *in, uchar *out, int decMode);
void camellia_crypt(camellia_t *i, uchar *text, int decMode);
void xor16(uchar *block, uchar *subBlock);
void camellia_crypt_rcbc(camellia_t *i, uchar *text, int size, int decMode);

void padding16(autoList<uchar> *block, int fixedPaddingMode = 0);
int unpadding16(autoList<uchar> *block);
void addHash16(autoList<uchar> *block);
int unaddHash16(autoList<uchar> *block);
int camellia_crypt_extend(camellia_t *i[2], autoList<uchar> *text, int decMode, int fixedPaddingMode = 0);
