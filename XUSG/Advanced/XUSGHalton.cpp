//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XUSGAdvanced.h"

using namespace DirectX;
using namespace XUSG;

float XUSG::Halton(uint32_t i, uint32_t b)
{
	auto f = 1.0f;
	auto r = 0.0f;

	for (auto j = i + 1; j; j /= b)
	{
		f /= b;
		r += f * (j % b);
	}

	return r;
}

XMFLOAT2 XUSG::Halton(uint32_t i)
{
	const auto haltonX = Halton(i, 2);
	const auto haltonY = Halton(i, 3);

	return XMFLOAT2(haltonX, haltonY);
}

const XMFLOAT2& XUSG::IncrementalHalton()
{
	static XMUINT2 haltonBase(0, 0);
	static XMFLOAT2 halton(0.0f, 0.0f);

	// Base 2
	{
		// Bottom bit always changes, higher bits
		// Change less frequently.
		auto change = 0.5f;
		auto oldBase = haltonBase.x++;
		auto diff = haltonBase.x ^ oldBase;

		// Diff will be of the form 0*1+, i.e. one bits up until the last carry.
		// Expected iterations = 1 + 0.5 + 0.25 + ... = 2
		do
		{
			halton.x += (oldBase & 1) ? -change : change;
			change *= 0.5f;

			diff = diff >> 1;
			oldBase = oldBase >> 1;
		} while (diff);
	}

	// Base 3
	{
		const auto oneThird = 1.0f / 3.0f;
		auto mask = 0x3u;	// Also the max base 3 digit
		auto add = 0x1u;	// Amount to add to force carry once digit == 3
		auto change = oneThird;
		++haltonBase.y;

		// Expected iterations: 1.5
		while (true)
		{
			if ((haltonBase.y & mask) == mask)
			{
				haltonBase.y += add;	// Force carry into next 2-bit digit
				halton.y -= 2 * change;

				mask = mask << 2;
				add = add << 2;

				change *= oneThird;
			}
			else
			{
				halton.y += change;	// We know digit n has gone from a to a + 1
				break;
			}
		};
	}

	return halton;
}
