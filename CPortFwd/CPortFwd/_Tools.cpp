#include "all.h"

static int IsHex128(char *str)
{
	if(strlen(str) != 128)
		return 0;

	for(char *p = str; *p; p++)
		if(!m_isHex(*p))
			return 0;

	return 1;
}
uchar *StrToRawKey(char *str)
{
	static uchar rawKey[64];

	cout("prm: %s\n", str);

	if(IsHex128(str))
	{
		for(int index = 0; index < 64; index++)
			rawKey[index] = m_c2i(str[index * 2]) * 16 + m_c2i(str[index * 2 + 1]);
	}
	else
	{
		sha512_t *ctx = sha512_create();

		sha512_update(ctx, str, strlen(str));
		sha512_makeHash(ctx);

		memcpy(rawKey, sha512_hash, 64);

		sha512_release(ctx);
	}
	cout("key: ");

	for(int index = 0; index < 64; index++)
		cout("%02x", rawKey[index]);

	cout("\n");
	return rawKey;
}
