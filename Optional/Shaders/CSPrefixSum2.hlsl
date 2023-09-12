//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define GROUP_SIZE 1024

//--------------------------------------------------------------------------------------
// Buffer
//--------------------------------------------------------------------------------------
RWBuffer<uint> g_rwData;

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	const uint gIdx = DTid % GROUP_SIZE;
	const uint gid = DTid / GROUP_SIZE;

	if (gIdx > 0) g_rwData[DTid] += g_rwData[GROUP_SIZE * gid];
}
