//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen and Yang, Jiale. All rights reserved.
//--------------------------------------------------------------------------------------

#include "CoopVec.hlsli"

#ifndef FLOAT_T
#define FLOAT_T T
#endif

#define MATRIX_LAYOUT_VALUE_ROW_MAJOR				0
#define MATRIX_LAYOUT_VALUE_COLUMN_MAJOR			1
#define MATRIX_LAYOUT_VALUE_MUL_OPTIMAL				2
#define MATRIX_LAYOUT_VALUE_OUTER_PRODUCT_OPTIMAL	3

#ifndef MATRIX_LAYOUT_VALUE
#define MATRIX_LAYOUT_VALUE MATRIX_LAYOUT_VALUE_MUL_OPTIMAL
#endif

#if MATRIX_LAYOUT_VALUE == MATRIX_LAYOUT_VALUE_ROW_MAJOR
#define MATRIX_LAYOUT MATRIX_LAYOUT_ROW_MAJOR
#elif MATRIX_LAYOUT_VALUE == MATRIX_LAYOUT_VALUE_COLUMN_MAJOR
#define MATRIX_LAYOUT MATRIX_LAYOUT_COLUMN_MAJOR
#elif MATRIX_LAYOUT_VALUE == MATRIX_LAYOUT_VALUE_MUL_OPTIMAL
#define MATRIX_LAYOUT MATRIX_LAYOUT_MUL_OPTIMAL
#elif MATRIX_LAYOUT_VALUE == MATRIX_LAYOUT_VALUE_OUTER_PRODUCT_OPTIMAL
#define MATRIX_LAYOUT MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL
#else
#error Invalid MATRIX_LAYOUT_VALUE
#endif

#ifndef PI
#define PI 3.1415926535897
#endif

#define VEC_T_M vector<T, M>
#define VEC_T_K vector<T, K>
#define VEC_T_N vector<T, N>
#define VEC_T_HN vector<T, N / 2>

#ifdef _SLANG_
#define S2V(T, N, x) vector<T, N>(T(x))
#else
#define S2V(T, N, x) T(x)
#endif
//#define S2V(T, N, x) vectorFill<T, N>(T(x))

#define RCP(x) T(rcp(x))

#define PROCESS_VEC(y, n, N, statement) VEC_T_N y; FOR_EACH(n, N, const T y_n = statement; vectorWriteToIndex(y, n, y_n)) return y

#define ALIGN(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))

TEMPLATE_FUNC(VEC_T_N, ELU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T alpha = T(1.0))
{
	const VEC_T_N t = (exp(x) - S2V(T, N, 1.0)) * S2V(T, N, alpha);

#ifdef _SM_69_
	return select(x > 0.0, x, t);
#else
	PROCESS_VEC(y, n, N, x[n] > T(0.0) ? x[n] : t[n]);
#endif
}

TEMPLATE_FUNC(VEC_T_N, Hardshrink, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T lambd = T(0.5))
{
#ifdef _SM_69_
	return select(or(x > lambd, x < -lambd), x, 0.0);
#else
	PROCESS_VEC(y, n, N, x[n] > lambd || x[n] < -lambd ? x[n] : T(0.0));
#endif
}

TEMPLATE_FUNC(VEC_T_N, Hardsigmoid, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	const VEC_T_N t = x * RCP(6.0) + S2V(T, N, 0.5);

#ifdef _SM_69_
	return select(x <= -3.0, 0.0, select(x >= 3.0, 1.0, x / 6.0 + 0.5));
#else
	PROCESS_VEC(y, n, N, x[n] <= T(-3.0) ? T(0.0) : (x[n] >= T(3.0) ? T(1.0) : t[n]));
#endif
}

TEMPLATE_FUNC(VEC_T_N, Hardtanh, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T min_val = T(-1.0), T max_val = T(1.0))
{
	return clamp(x, T(min_val), T(max_val));
}

TEMPLATE_FUNC(VEC_T_N, Hardswish, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	const VEC_T_N t = x * (x + S2V(T, N, 3.0)) * RCP(6.0);

#ifdef _SM_69_
	return select(x <= -3.0, 0.0, select(x >= 3.0, x, t));
#else
	PROCESS_VEC(y, n, N, x[n] <= T(-3.0) ? T(0.0) : (x[n] >= T(3.0) ? x[n] : t[n]));
#endif
}

TEMPLATE_FUNC(VEC_T_N, LeakyReLU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T negativeSlope = T(0.01))
{
	const VEC_T_N t = x * negativeSlope;

#ifdef _SM_69_
	return select(x > 0.0, x, t);
#else
	PROCESS_VEC(y, n, N, x[n] > T(0.0) ? x[n] : t[n]);
#endif
}

TEMPLATE_FUNC(VEC_T_N, PReLU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, ByteAddressBuffer paramBuffer, int offset)
{
	const vector<T, N> a = vectorLoad<N, FLOAT_T>(paramBuffer, offset);
	const vector<T, N> t = x * a;

#ifdef _SM_69_
	return select(x >= 0.0, x, t);
#else
	PROCESS_VEC(y, n, N, x[n] >= T(0.0) ? x[n] : t[n]);
#endif
}

TEMPLATE_FUNC(VEC_T_N, ReLU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return max(T(0.0), x);
}

TEMPLATE_FUNC(VEC_T_N, ReLU6, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return min(max(T(0.0), x), T(6.0));
}

TEMPLATE_FUNC(VEC_T_N, RReLU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T lower = T(0.125), T upper = T(0.3333333333333333))
{
	const T a = (lower + upper) * T(0.5);
	const VEC_T_N t = x * a;

#ifdef _SM_69_
	return select(x >= 0.0, x, t);
#else
	PROCESS_VEC(y, n, N, x[n] >= T(0.0) ? x[n] : t[n]);
#endif
}

TEMPLATE_FUNC(VEC_T_N, SELU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	const T a = T(1.6732632423543772848170429916717);
	const T scale = T(1.0507009873554804934193349852946);

	return (max(T(0.0), x) + min(T(0.0), (exp(x) - S2V(T, N, 1.0)) * a)) * scale;
}

TEMPLATE_FUNC(VEC_T_N, CELU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T alpha = T(1.0))
{
	return max(T(0.0), x) + min(T(0.0), (exp(x * rcp(alpha)) - S2V(T, N, 1.0)) * alpha);
}

TEMPLATE_FUNC(VEC_T_N, GELU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return x * T(0.5) * (tanh((x * x * x * T(0.044715) + x) * T(sqrt(2.0 / PI))) + S2V(T, N, 1.0));
}

TEMPLATE_FUNC(VEC_T_N, Sigmoid, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return rcp((exp(negate(x)) + S2V(T, N, 1.0)));
}

TEMPLATE_FUNC(VEC_T_N, LogSigmoid, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return log(Sigmoid(x));
}

TEMPLATE_FUNC(VEC_T_N, SiLU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return x * Sigmoid(x);
}

TEMPLATE_FUNC(VEC_T_N, Softplus, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T beta = T(1.0), T threshold = T(20.0))
{
	const VEC_T_N bx = x * beta;
	const VEC_T_N t = log(exp(bx) + S2V(T, N, 1.0)) * rcp(beta);

#ifdef _SM_69_
	return select(bx > threshold, x, t);
#else
	PROCESS_VEC(y, n, N, bx[n] > threshold ? x[n] : t[n]);
#endif
}

TEMPLATE_FUNC(VEC_T_N, Mish, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return x * tanh(Softplus(x));
}

TEMPLATE_FUNC(VEC_T_N, Softshrink, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T lambd = T(0.5))
{
	const VEC_T_N t1 = x - S2V(T, N, lambd);
	const VEC_T_N t2 = x + S2V(T, N, lambd);

#ifdef _SM_69_
	return select(x > lambd, t1, select(x < -lambd, t2, 0.0));
#else
	PROCESS_VEC(y, n, N, x[n] > lambd ? t1[n] : (x[n] < -lambd ? t2[n] : T(0.0)));
#endif
}

TEMPLATE_FUNC(VEC_T_N, Softsign, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return x / (abs(x) + S2V(T, N, 1.0));
}

#define Tanh(x) tanh(x)

TEMPLATE_FUNC(VEC_T_N, Tanhshrink, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return x - tanh(x);
}

TEMPLATE_FUNC(VEC_T_N, Threshold, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, T threshold, T value)
{
#ifdef _SM_69_
	return select(x > threshold, x, value);
#else
	PROCESS_VEC(y, n, N, x[n] > threshold ? x[n] : value);
#endif
}

TEMPLATE_FUNC(VEC_T_HN, GLU, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	VEC_T_HN a, b;
	INT n;

	for (n = 0; n < N / 2; ++n) vectorWriteToIndex(a, n, x[n]);
	for (n = 0; n < N / 2; ++n) vectorWriteToIndex(b, n, x[n + N / 2]);

	return a * Sigmoid(b);
}

TEMPLATE_FUNC(T, ReducedSum, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	T sum = T(0.0);
	for (INT n = 0; n < N; ++n) sum += x[n];

	return sum;
}

TEMPLATE_FUNC(VEC_T_N, Softmin, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	const VEC_T_N exp_neg_x = exp(negate(x));

	return exp_neg_x * RCP(ReducedSum(exp_neg_x));
}

TEMPLATE_FUNC(VEC_T_N, Softmax, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	const VEC_T_N exp_x = exp(x);

	return exp_x * RCP(ReducedSum(exp_x));
}

TEMPLATE_FUNC(VEC_T_N, LogSoftmax, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x)
{
	return log(Softmax(x));
}

TEMPLATE_FUNC(VEC_T_N, Norm1d, INT N, FLOAT_TYPE_NAME(T))(VEC_T_N x, ByteAddressBuffer paramBuffer, int offset, T eps = T(1e-5))
{
	// g: offset of the weight (gamma) data in the buffer
	// b: offset of the bias (beta) data in the buffer
	// m: offset of the mean (mu) data in the buffer
	// v: offset of the variance (sigma^2) data in the buffer
	const int g = offset;
	const int b = g + sizeof(T) * N;
	const int m = b + sizeof(T) * N;
	const int v = m + sizeof(T) * N;

	const VEC_T_N mu = vectorLoad<N, T>(paramBuffer, m);
	const VEC_T_N var = vectorLoad<N, T>(paramBuffer, v);
	const VEC_T_N gamma = vectorLoad<N, T>(paramBuffer, g);
	const VEC_T_N beta = vectorLoad<N, T>(paramBuffer, b);

#if defined(_SM_69_) || defined(_SLANG_)
	VEC_T_N rsigma;
	for (INT n = 0; n < N; ++n) vectorWriteToIndex(rsigma, n, rsqrt(var[n] + eps));
	//const VEC_T_N rsigma = rsqrt(var + eps);

	const VEC_T_N t = (x - mu) * rsigma;

	return t * gamma + beta;
#else
	VEC_T_N y;
	for (INT n = 0; n < N; ++n)
	{
		const T rsigma_n = rsqrt(var[n] + eps);
		const T t = (x[n] - mu[n]) * rsigma_n;
		vectorWriteToIndex(y, n, t * gamma[n] + beta[n]);
	}

	return y;
#endif
}

TEMPLATE_FUNC(VEC_T_M, Linear, INT M, INT K, FLOAT_TYPE_NAME(T))(VEC_T_K x, ByteAddressBuffer matrixBiasBuffer,
	int matrixOffset, int biasOffset, uint GI : SV_GroupIndex)
{
#ifdef _SM_69_
	const uint matrixStride = MATRIX_LAYOUT == MATRIX_LAYOUT_ROW_MAJOR ? ALIGN(sizeof(FLOAT_T) * K, 16) :
		(MATRIX_LAYOUT == MATRIX_LAYOUT_COLUMN_MAJOR ? ALIGN(sizeof(FLOAT_T) * M, 16) : 0);
#else
	const uint matrixStride = MATRIX_LAYOUT == MATRIX_LAYOUT_ROW_MAJOR ? sizeof(FLOAT_T) * K :
		(MATRIX_LAYOUT == MATRIX_LAYOUT_COLUMN_MAJOR ? sizeof(FLOAT_T) * M : 0);
#endif

	const vector<FLOAT_T, M> y = matVecMulAdd<FLOAT_T, M, DATA_TYPE_FLOAT16, DATA_TYPE_FLOAT16, DATA_TYPE_FLOAT16, MATRIX_LAYOUT, false, matrixStride>(
		x,                // input vector
		matrixBiasBuffer, // input matrix
		matrixOffset,     // matrix offset in byte
		matrixBiasBuffer, // input bias
		biasOffset,       // bias offset in byte
		GI                // group index
	);

	return vectorCopyFrom<T>(y);
}
