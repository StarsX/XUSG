//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen and Yang, Jiale. All rights reserved.
//--------------------------------------------------------------------------------------

#include "CoopVec.hlsli"

#ifndef FLOAT_T
#define FLOAT_T T
#endif

#ifndef MATRIX_LAYOUT
#define MATRIX_LAYOUT MATRIX_LAYOUT_ROW_MAJOR
#endif

uint Align(uint size, uint alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

float Sigmoid(float x)
{
	return 1.0 / (1.0 + exp(-x));
}

TEMPLATE_FUNC(void, ReLU, INT N, FLOAT_TYPE_NAME(T))(out vector<T, N> y, vector<T, N> x)
{
	y = max(x, T(0.0));
}

TEMPLATE_FUNC(int, Linear, INT M, INT K, FLOAT_TYPE_NAME(T))(out vector<T, M> y, vector<T, K> x,
	ByteAddressBuffer matrixBiasBuffer, int offset, uint GI : SV_GroupIndex)
{
	// m: offset of the matrix data in the buffer
	// b: offset of the bias data in the buffer
#ifdef _SM_69_
	const uint matrixStride = MATRIX_LAYOUT == MATRIX_LAYOUT_ROW_MAJOR ? Align(sizeof(FLOAT_T) * K, 16) :
		(MATRIX_LAYOUT == MATRIX_LAYOUT_COLUMN_MAJOR ? Align(sizeof(FLOAT_T) * M, 16) : 0);
	const int matrixOffset = Align(offset, 128);
	int biasOffset = matrixOffset;
	biasOffset += MATRIX_LAYOUT == MATRIX_LAYOUT_ROW_MAJOR ? M * matrixStride :
		(MATRIX_LAYOUT == MATRIX_LAYOUT_COLUMN_MAJOR ? matrixStride * K :
		sizeof(FLOAT_T) * (Align(M, 8) * Align(K, 8)));
	biasOffset = Align(biasOffset, 64);
#else
	const uint matrixStride = MATRIX_LAYOUT == MATRIX_LAYOUT_ROW_MAJOR ? sizeof(FLOAT_T) * K :
		(MATRIX_LAYOUT == MATRIX_LAYOUT_COLUMN_MAJOR ? sizeof(FLOAT_T) * M : 0);
	const int matrixOffset = MATRIX_LAYOUT == MATRIX_LAYOUT_MUL_OPTIMAL ? Align(offset, 128) : offset;
	const int biasOffset = MATRIX_LAYOUT == MATRIX_LAYOUT_MUL_OPTIMAL ?
		Align(matrixOffset + sizeof(FLOAT_T) * (Align(M, 8) * Align(K, 8)), 64) :
		matrixOffset + sizeof(FLOAT_T) * M * K;
#endif

	vector<FLOAT_T, M> f = matVecMulAdd<FLOAT_T, M, DATA_TYPE_FLOAT16, DATA_TYPE_FLOAT16, DATA_TYPE_FLOAT16, MATRIX_LAYOUT, false>(
		x,                // input vector
		matrixBiasBuffer, // input matrix
		matrixOffset,     // matrix offset in byte
		matrixBiasBuffer, // input bias
		biasOffset,       // bias offset in byte
		matrixStride,     // matrix stride
		GI                // group index
	);

	vectorCopyFrom(y, f);

	return biasOffset + sizeof(FLOAT_T) * M;
}

TEMPLATE_FUNC(int, Norm1d, INT N, FLOAT_TYPE_NAME(T))(out vector<T, N> y, vector<T, N> x, ByteAddressBuffer paramBuffer, int offset)
{
	// g: offset of the weight (gamma) data in the buffer
	// b: offset of the bias (beta) data in the buffer
	// m: offset of the mean (mu) data in the buffer
	// v: offset of the variance (sigma^2) data in the buffer
	const int g = offset;
	const int b = g + sizeof(FLOAT_T) * N;
	const int m = b + sizeof(FLOAT_T) * N;
	const int v = m + sizeof(FLOAT_T) * N;
	const FLOAT_T eps = FLOAT_T(1e-5);

	const vector<FLOAT_T, N> mu = vectorLoad<N, FLOAT_T>(paramBuffer, m);
	const vector<FLOAT_T, N> var = vectorLoad<N, FLOAT_T>(paramBuffer, v);
	const vector<FLOAT_T, N> gamma = vectorLoad<N, FLOAT_T>(paramBuffer, g);
	const vector<FLOAT_T, N> beta = vectorLoad<N, FLOAT_T>(paramBuffer, b);

#if defined(_SM_69_) || defined(_SLANG_)
	vector<FLOAT_T, N> rsigma;
	for (INT n = 0; n < N; ++n) vectorWriteToIndex(rsigma, n, rsqrt(var[n] + eps));
	//const vector<FLOAT_T, N> rsigma = rsqrt(var + eps);

	vector<FLOAT_T, N> t = x;
	t = (t - mu) * rsigma;
	y = t * gamma + beta;
#elif 1
	for (INT n = 0; n < N; ++n)
	{
		const FLOAT_T mu_n = vectorReadFromIndex(mu, n);
		const FLOAT_T var_n = vectorReadFromIndex(var, n);
		const FLOAT_T gamma_n = vectorReadFromIndex(gamma, n);
		const FLOAT_T beta_n = vectorReadFromIndex(beta, n);

		const FLOAT_T rsigma_n = rsqrt(var_n + eps);

		const FLOAT_T x_n = vectorReadFromIndex(x, n);
		const FLOAT_T t = (x_n - mu_n) * rsigma_n;
		vectorWriteToIndex(y, n, T(t * gamma_n + beta_n));
	}
#else
	vector<FLOAT_T, N> rsigma;
	for (INT n = 0; n < N; ++n) vectorWriteToIndex(rsigma, n, rsqrt(var[n] + eps));

	vector<FLOAT_T, N> t;
	vectorCopyFrom(t, x);
	t = (t - mu) * rsigma;
	vectorCopyFrom(y, t * gamma + beta);
#endif

	return v + sizeof(FLOAT_T) * N;
}
