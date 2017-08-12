#include "all.h"

int getZero(void)
{
	return 0;
}
int s2i(char *line)
{
	int result = 0;
	int sign = 1;

	for(char *p = line; *p; p++)
	{
		int digit = -1;

		if(*p == '-')
		{
			sign = -1;
		}
		else if(*p == '\x82') // "‚O" == { '\x82', '\x4f' }
		{
			p++;

			if(!*p)
				break;

			if('\x4f' <= *p && *p <= '\x58')
			{
				digit = *p - 0x4f;
			}
		}
		else if('0' <= *p && *p <= '9')
		{
			digit = *p - '0';
		}
		if(0 <= digit)
		{
			if((INT_MAX - 9) / 10 < result)
				break;

			result *= 10;
			result += digit;
		}
	}
	return result * sign;
}

int compInt(int a, int b)
{
	if(a < b) return -1;
	if(b < a) return 1;
	return 0;
}
int compI2D(i2D_t a, i2D_t b)
{
	int retval = compInt(a.X, b.X);

	if(!retval)
		retval = compInt(a.Y, b.Y);

	return retval;
}

int isHitKey(int key)
{
	int hit = 0;

	while(_kbhit())
	{
		cout("stdin %d\n", key);

		if(_getch() == key)
		{
			hit = 1;
		}
		cout("hit: %d\n", hit);
	}
	return hit;
}

void addValue(autoList<uchar> *block, uint64 value)
{
	block->AddElement(value >> 8 * 0 & 0xff);
	block->AddElement(value >> 8 * 1 & 0xff);
	block->AddElement(value >> 8 * 2 & 0xff);
	block->AddElement(value >> 8 * 3 & 0xff);
	block->AddElement(value >> 8 * 4 & 0xff);
	block->AddElement(value >> 8 * 5 & 0xff);
	block->AddElement(value >> 8 * 6 & 0xff);
	block->AddElement(value >> 8 * 7 & 0xff);
}
uint64 unaddValue64(autoList<uchar> *block)
{
	uint64 value = (uint64)block->UnaddElement() << 8 * 7;

	value |= (uint64)block->UnaddElement() << 8 * 6;
	value |= (uint64)block->UnaddElement() << 8 * 5;
	value |= (uint64)block->UnaddElement() << 8 * 4;
	value |= (uint64)block->UnaddElement() << 8 * 3;
	value |= (uint64)block->UnaddElement() << 8 * 2;
	value |= (uint64)block->UnaddElement() << 8 * 1;
	value |= (uint64)block->UnaddElement() << 8 * 0;

	return value;
}
