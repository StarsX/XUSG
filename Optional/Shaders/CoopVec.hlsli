//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen and Yang, Jiale. All rights reserved.
//--------------------------------------------------------------------------------------

#define WAVE_SIZE 32

#ifndef GROUP_SIZE
#define GROUP_SIZE WAVE_SIZE
#endif

#ifndef MLOP_MAKE_HALF
#define MLOP_MAKE_HALF(N) float16_t##N
#endif

#ifndef HALF
#define HALF MLOP_MAKE_HALF()
#endif

#ifndef FLOAT
#define FLOAT HALF
#endif

#define FOR_EACH(n, N, statement) for (INT n = 0; n < N; ++n) { statement; }

#ifdef _SLANG_

#define __SHADER_TARGET_MINOR 9
#define INT int
#define FLOAT_TYPE_NAME(T) T : __BuiltinFloatingPointType
#define TEMPLATE_FUNC(retType, func, ...) retType func<__VA_ARGS__>

#define vectorLoad	coopVecLoad
#define vector		CoopVec

#define DATA_TYPE_SINT16 CoopVecComponentType::SignedInt16                 // ComponentType::I16
#define DATA_TYPE_UINT16 CoopVecComponentType::UnsignedInt16               // ComponentType::U16
#define DATA_TYPE_SINT32 CoopVecComponentType::SignedInt32                 // ComponentType::I32
#define DATA_TYPE_UINT32 CoopVecComponentType::UnsignedInt32               // ComponentType::U32
#define DATA_TYPE_FLOAT16 CoopVecComponentType::Float16                    // ComponentType::F16
#define DATA_TYPE_FLOAT32 CoopVecComponentType::Float32                    // ComponentType::F32
#define DATA_TYPE_SINT8_T4_PACKED CoopVecComponentType::SignedInt8Packed   // ComponentType::PackedS8x32
#define DATA_TYPE_UINT8_T4_PACKED CoopVecComponentType::UnsignedInt8Packed // ComponentType::PackedU8x32
#define DATA_TYPE_UINT8 CoopVecComponentType::UnsignedInt8                 // ComponentType::U8
#define DATA_TYPE_SINT8 CoopVecComponentType::UnsignedInt8                 // ComponentType::I8
#define DATA_TYPE_FLOAT8_E4M3 CoopVecComponentType::FloatE4M3              // ComponentType::F8_E4M3
																		   // (1 sign, 4 exp, 3 mantissa bits)
#define DATA_TYPE_FLOAT8_E5M2 CoopVecComponentType::FloatE5M2              // ComponentType::F8_E5M2
																		   // (1 sign, 5 exp, 2 mantissa bits)

#define MATRIX_LAYOUT_ROW_MAJOR CoopVecMatrixLayout::RowMajor
#define MATRIX_LAYOUT_COLUMN_MAJOR CoopVecMatrixLayout::ColumnMajor
#define MATRIX_LAYOUT_MUL_OPTIMAL CoopVecMatrixLayout::InferencingOptimal
#define MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL CoopVecMatrixLayout::TrainingOptimal

vector<T, M> matVecMulAdd<T : __BuiltinArithmeticType, INT M, CoopVecComponentType inputDataType,
	CoopVecComponentType matrixDataType, CoopVecComponentType biasDataType,
	CoopVecMatrixLayout matrixLayout, bool transpose, uint matrixStride,
	U : __BuiltinArithmeticType, INT K>(
	vector<U, K> input,
	ByteAddressBuffer matrixBuffer,
	int matrixOffset,
	ByteAddressBuffer biasBuffer,
	int biasOffset,
	uint gi : SV_GroupIndex)
{
	return coopVecMatMulAdd<T, M>(input, inputDataType, matrixBuffer, matrixOffset, matrixDataType,
		biasBuffer, biasOffset, biasDataType, matrixLayout, transpose, matrixStride);
}

vector<T, N> vectorCopyFrom<T : __BuiltinArithmeticType, U : __BuiltinArithmeticType, INT N>(vector<U, N> x)
{
	vector<T, N> y;
	y.copyFrom(x);

	return y;
}

vector<T, N> vectorFill<T : __BuiltinFloatingPointType, INT N>(T x)
{
	vector<T, N> y;
	y.fill(x);

	return y;
}

vector<T, N> max<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x, T y)
{
	return max(x, vector<T, N>(y));
}

vector<T, N> min<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x, T y)
{
	return min(x, vector<T, N>(y));
}

vector<T, N> max<T : __BuiltinFloatingPointType, INT N>(T x, vector<T, N> y)
{
	return max(vector<T, N>(x), y);
}

vector<T, N> min<T : __BuiltinFloatingPointType, INT N>(T x, vector<T, N> y)
{
	return min(vector<T, N>(x), y);
}

vector<T, N> abs<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x)
{
	FOR_EACH(n, N, x[n] = abs(x[n]))

	return x;
}

vector<T, N> negate<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x)
{
	return x.neg();
}

vector<T, N> rcp<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x)
{
	return vector<T, N>(T(1.0)) / x;
}

vector<T, N> clamp<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x, vector<T, N> a, T b)
{
	return clamp(x, a, vector<T, N>(b));
}

vector<T, N> clamp<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x, T a, vector<T, N> b)
{
	return clamp(x, vector<T, N>(a), b);
}

vector<T, N> clamp<T : __BuiltinFloatingPointType, INT N>(vector<T, N> x, T a, T b)
{
	return clamp(x, vector<T, N>(a), vector<T, N>(b));
}

#else

#include "linalg.h"

#define FLOAT_TYPE_NAME(T) typename T
#define TEMPLATE_FUNC(retType, func, ...) template<__VA_ARGS__> retType func

#if ((__SHADER_TARGET_MAJOR > 6) || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 9)) && (__HLSL_VERSION >= 2021)
#define _SM_69_
#define INT int
using namespace dx::linalg;
#else

#define INT uint

enum DataType : uint
{
	DATA_TYPE_SINT16 = 6,          // ComponentType::I16
	DATA_TYPE_UINT16 = 3,          // ComponentType::U16
	DATA_TYPE_SINT32 = 7,          // ComponentType::I32
	DATA_TYPE_UINT32 = 4,          // ComponentType::U32
	DATA_TYPE_FLOAT16 = 0,         // ComponentType::F16
	DATA_TYPE_FLOAT32 = 1,         // ComponentType::F32
	DATA_TYPE_SINT8_T4_PACKED = 8, // ComponentType::PackedS8x32
	DATA_TYPE_UINT8_T4_PACKED = 9, // ComponentType::PackedU8x32
	DATA_TYPE_UINT8 = 2,           // ComponentType::U8
	DATA_TYPE_SINT8 = 5,           // ComponentType::I8
	DATA_TYPE_FLOAT8_E4M3 = 10,    // ComponentType::F8_E4M3
								   // (1 sign, 4 exp, 3 mantissa bits)
	DATA_TYPE_FLOAT8_E5M2 = 11,    // ComponentType::F8_E5M2
								   // (1 sign, 5 exp, 2 mantissa bits)
};

enum MatrixLayout : uint
{
	MATRIX_LAYOUT_ROW_MAJOR = 0,
	MATRIX_LAYOUT_COLUMN_MAJOR = 1,
	MATRIX_LAYOUT_MUL_OPTIMAL = 2,
	MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL = 3
};

#endif

#ifdef _SM_69_
#define Vector vector
#else
template<typename T, INT N>
struct Vector
{
	T operator [](int index)
	{
		return Data[index];
	}

	void writeToIndex(int index, T value)
	{
		Data[index] = value;
	}

	void fill(T x)
	{
		FOR_EACH(n, N, Data[n] = x)
	}

#define DEFINE_OPERATOR(op, xType, yType, x, y, n, statement) yType operator op(xType x) { yType y; FOR_EACH(n, N, statement) return y; }
#define VEC_T_N Vector<T, N>
	DEFINE_OPERATOR(+, VEC_T_N, VEC_T_N, x, y, n, y.Data[n] = Data[n] + x[n])
	DEFINE_OPERATOR(-, VEC_T_N, VEC_T_N, x, y, n, y.Data[n] = Data[n] - x[n])
	DEFINE_OPERATOR(*, VEC_T_N, VEC_T_N, x, y, n, y.Data[n] = Data[n] * x[n])
	DEFINE_OPERATOR(/, VEC_T_N, VEC_T_N, x, y, n, y.Data[n] = Data[n] / x[n])

	DEFINE_OPERATOR(+, T, VEC_T_N, x, y, n, y.Data[n] = Data[n] + x)
	DEFINE_OPERATOR(-, T, VEC_T_N, x, y, n, y.Data[n] = Data[n] - x)
	DEFINE_OPERATOR(*, T, VEC_T_N, x, y, n, y.Data[n] = Data[n] * x)
	DEFINE_OPERATOR(/, T, VEC_T_N, x, y, n, y.Data[n] = Data[n] / x)

#define VEC_B_N Vector<bool, N>
	DEFINE_OPERATOR(>, VEC_T_N, VEC_B_N, x, y, n, y.Data[n] = Data[n] > x[n])
	DEFINE_OPERATOR(>=, VEC_T_N, VEC_B_N, x, y, n, y.Data[n] = Data[n] >= x[n])
	DEFINE_OPERATOR(<, VEC_T_N, VEC_B_N, x, y, n, y.Data[n] = Data[n] < x[n])
	DEFINE_OPERATOR(<=, VEC_T_N, VEC_B_N, x, y, n, y.Data[n] = Data[n] <= x[n])

	DEFINE_OPERATOR(>, T, VEC_B_N, x, y, n, y.Data[n] = Data[n] > x)
	DEFINE_OPERATOR(>=, T, VEC_B_N, x, y, n, y.Data[n] = Data[n] >= x)
	DEFINE_OPERATOR(<, T, VEC_B_N, x, y, n, y.Data[n] = Data[n] < x)
	DEFINE_OPERATOR(<=, T, VEC_B_N, x, y, n, y.Data[n] = Data[n] <= x)
#undef VEC_B_N
#undef VEC_T_N

	T Data[N];
};
#endif

#if __SHADER_TARGET_MINOR == 8

#define WM_M 16
#define WM_N 16
#define WM_K_MAX 32
#define N WAVE_SIZE

#define NUM_GROUP_WAVES (GROUP_SIZE / WAVE_SIZE)
#define WAVE_TENSOR_SIZE (WM_K_MAX * N)
#define WAVE_MATRIX_C_SIZE (WM_M * N)
#define WAVE_MATRIX_A_SIZE (WM_M * WM_K_MAX)

groupshared HALF g_tensor[WAVE_TENSOR_SIZE * NUM_GROUP_WAVES];
groupshared FLOAT g_matrixC[WAVE_MATRIX_C_SIZE * NUM_GROUP_WAVES];
groupshared HALF g_matrixA[WAVE_MATRIX_A_SIZE * NUM_GROUP_WAVES];

template<typename T, bool transpose, typename U>
void LoadMatrix(inout WaveMatrixLeft<U, WM_M, WM_N> A, ByteAddressBuffer matrixBuffer, uint offset, uint pitch, uint waveId)
{
	const uint WM_K = A.MatrixDepth();
	const uint m = WaveGetLaneIndex();
	const uint p = offset + pitch * (m % WM_M);
	const uint b = (WM_K / 2) * (m / WM_M);

	if (WM_K == 16)
	{
		const Vector<T, 8> v = matrixBuffer.Load< Vector<T, 8> >(p + sizeof(T) * b);
		for (uint k = 0; k < WM_K / 2; ++k) g_matrixA[WAVE_MATRIX_A_SIZE * waveId + (WM_K * (m % WM_M) + b + k)] = U(v[k]);
	}
	else if (WM_K == 32)
	{
		const Vector<T, 16> v = matrixBuffer.Load< Vector<T, 16> >(p + sizeof(T) * b);
		for (uint k = 0; k < WM_K / 2; ++k) g_matrixA[WAVE_MATRIX_A_SIZE * waveId + (WM_K * (m % WM_M) + b + k)] = U(v[k]);
	}
	else for (uint k = 0; k < WM_K / 2; ++k)
	{
		const T a = matrixBuffer.Load<T>(p + sizeof(T) * (b + k));
		g_matrixA[WAVE_MATRIX_A_SIZE * waveId + (WM_K * (m % WM_M) + b + k)] = U(a);
	}

	GroupMemoryBarrierWithGroupSync();

	A.Load(g_matrixA, WAVE_MATRIX_A_SIZE * waveId, WM_K, transpose);
}

template<typename T, INT M>
void GetTensor(out Vector<T, M> y, uint offsetOut, uint offsetIn, uint laneMask, uint waveId)
{
	const uint n = WaveGetLaneIndex();
	for (uint m = 0; m < WM_M; ++m)
		if ((1u << n) & laneMask)
			y.Data[offsetOut + m] = T(g_matrixC[WAVE_MATRIX_C_SIZE * waveId + (offsetIn + WM_M * n + m)]);
}

template<typename T, INT M, DataType inputDataType, DataType matrixDataType, DataType biasDataType,
	MatrixLayout matrixLayout, bool transpose, uint matrixStride, typename U, INT K>
Vector<T, M> matVecMulAdd(
	Vector<U, K> input,
	ByteAddressBuffer matrixBuffer,
	int matrixOffset,
	ByteAddressBuffer biasBuffer,
	int biasOffset,
	uint gi : SV_GroupIndex)
{
	Vector<T, M> output;

	WaveMatrixLeft<U, WM_M, WM_N> A;
	WaveMatrixRight<U, WM_M, WM_N> X0, X1;
	WaveMatrixLeftColAcc<T, WM_M, WM_N> B;
	WaveMatrixAccumulator<T, WM_M, WM_N> C0, C1;

	const uint waveId = WaveReadLaneAt(gi / WaveGetLaneCount(), 0);
	const uint n = WaveGetLaneIndex();
	const uint WM_K = A.MatrixDepth();
	const uint c = sizeof(T) * min(WM_M, M); // Aligned column offset
	const uint s = sizeof(T) * min(WM_K, K); // Aligned stride

#if 0
	output = biasBuffer.Load< Vector<T, M> >(biasOffset);

	// Waterfall loop for varying offset values in a wave without branching
	for (uint laneMask, exec = WaveActiveBallot(true).x; exec; exec &= ~laneMask) // Remove the lanes same to the first lane
	{
		// mOffset: offset of the matrix data in the buffer
		const int mOffset = WaveReadLaneAt(matrixOffset, firstbitlow(exec));
		laneMask = WaveActiveBallot(mOffset == matrixOffset).x;

		//[unroll]
		for (uint k = 0; k < K; k += WM_K)
		{
			// Store x into group-shared memory
			for (uint i = 0; i < WM_K; ++i)
				g_tensor[WAVE_TENSOR_SIZE * waveId + (WM_K * n + i)] = input.Data[k + i];
			GroupMemoryBarrierWithGroupSync();

			// Load x from group-shared memory
			X0.Load(g_tensor, WAVE_TENSOR_SIZE * waveId, WM_K, !transpose);
			X1.Load(g_tensor, WAVE_TENSOR_SIZE * waveId + WM_M * WM_K, WM_K, !transpose);

			//[unroll]
			for (uint m = 0; m < M; m += WM_M)
			{
				// Store y into group-shared memory
				for (uint i = 0; i < WM_M && m + i < M; ++i)
					g_matrixC[WAVE_MATRIX_C_SIZE * waveId + (WM_M * n + i)] = output.Data[m + i];
				GroupMemoryBarrierWithGroupSync();

				// Load y from group-shared memory
				C0.Load(g_matrixC, WAVE_MATRIX_C_SIZE * waveId, WM_M, !transpose);
				C1.Load(g_matrixC, WAVE_MATRIX_C_SIZE * waveId + WM_M * WM_N, WM_M, !transpose);

				// Row offset
				const uint r = mOffset + sizeof(T) * m * K;
#if defined(_LOAD_FP16_)
				//A.Load(matrixBuffer, r + c * k, s, transpose);
				LoadMatrix<T, transpose>(A, matrixBuffer, r + c * k, s, waveId);
#else
				LoadMatrix<T, false>(A, matrixBuffer, r + sizeof(T) * k, sizeof(T) * K, waveId);
#endif

				C0.MultiplyAccumulate(A, X0);
				C1.MultiplyAccumulate(A, X1);

				C0.Store(g_matrixC, WAVE_MATRIX_C_SIZE * waveId, WM_M, !transpose);
				C1.Store(g_matrixC, WAVE_MATRIX_C_SIZE * waveId + WM_M * WM_N, WM_M, !transpose);
				GetTensor(output, m, 0, laneMask, waveId);
			}
		}
	}
#else
	// Waterfall loop for varying offset values in a wave without branching
	for (uint laneMask, exec = WaveActiveBallot(true).x; exec; exec &= ~laneMask) // Remove the lanes same to the first lane
	{
		// mOffset: offset of the matrix data in the buffer
		// bOffset: offset of the bias data in the buffer
		const int mOffset = WaveReadLaneAt(matrixOffset, firstbitlow(exec));
		const int bOffset = WaveReadLaneAt(biasOffset, firstbitlow(exec));
		laneMask = WaveActiveBallot(mOffset == matrixOffset && bOffset == biasOffset).x;

		//[unroll]
		for (uint m = 0; m < M; m += WM_M)
		{
			B.Load(biasBuffer, bOffset + sizeof(T) * m, sizeof(T));
			C0.Fill(0.0);
			C0.Add(B);
			C1.Fill(0.0);
			C1.Add(B);

			// Row offset
			const uint r = mOffset + sizeof(T) * m * K;

			//[unroll]
			for (uint k = 0; k < K; k += WM_K)
			{
				// Store x into group-shared memory
				for (uint i = 0; i < WM_K; ++i)
					g_tensor[WAVE_TENSOR_SIZE * waveId + (WM_K * n + i)] = input.Data[k + i];
				GroupMemoryBarrierWithGroupSync();

#if defined(_LOAD_FP16_)
				//A.Load(matrixBuffer, r + c * k, s, transpose);
				LoadMatrix<T, transpose>(A, matrixBuffer, r + c * k, s, waveId);
#else
				LoadMatrix<T, false>(A, matrixBuffer, r + sizeof(T) * k, sizeof(T) * K, waveId);
#endif

				// Load x from group-shared memory
				X0.Load(g_tensor, WAVE_TENSOR_SIZE * waveId, WM_K, !transpose);
				C0.MultiplyAccumulate(A, X0);
				X1.Load(g_tensor, WAVE_TENSOR_SIZE * waveId + WM_M * WM_K, WM_K, !transpose);
				C1.MultiplyAccumulate(A, X1);
			}

			C0.Store(g_matrixC, WAVE_MATRIX_C_SIZE * waveId, WM_M, !transpose);
			C1.Store(g_matrixC, WAVE_MATRIX_C_SIZE * waveId + WM_M * WM_N, WM_M, !transpose);
			GetTensor(output, m, 0, laneMask, waveId);
		}
	}
#endif

	return output;
}

#undef N

#else

template<typename T, INT M, DataType inputDataType, DataType matrixDataType, DataType biasDataType,
	MatrixLayout matrixLayout, bool transpose, uint matrixStride, typename U, INT K>
Vector<T, M> matVecMulAdd(
	Vector<U, K> input,
	ByteAddressBuffer matrixBuffer,
	int matrixOffset,
	ByteAddressBuffer biasBuffer,
	int biasOffset,
	uint gi : SV_GroupIndex)
{
#ifdef _SM_69_
	const MatrixRef<matrixDataType, M, K, matrixLayout, transpose> mRef = { matrixBuffer, matrixOffset, matrixStride };
	const VectorRef<biasDataType> bRef = { biasBuffer, biasOffset };

	return MulAdd<T>(mRef, MakeInterpretedVector<inputDataType>(input), bRef);
#else
	Vector<T, M> output = biasBuffer.Load< Vector<T, M> >(biasOffset);

	for (uint m = 0; m < M; ++m)
	{
		const uint c = matrixOffset + sizeof(T) * K * m;

#if defined(_LOAD_FP16_)
		for (uint k = 0; k < K; k += 2)
		{
			const vector<U, 2> p = vector<U, 2>(input.Data[k], input.Data[k + 1]);
			const vector<U, 2> a = vector<U, 2>(matrixBuffer.Load< vector<T, 2> >(c + sizeof(T) * k));
			output.Data[m] = T(dot2add(a, p, output.Data[m]));
		}
#else
		for (uint k = 0; k < K; ++k)
			output.Data[m] += U(matrixBuffer.Load<T>(c + sizeof(T) * k)) * input.Data[k];
#endif
	}

	return output;
#endif
}

#endif

#ifndef _SM_69_
#define vector Vector
#endif

template<INT N, typename T>
vector<T, N> vectorLoad(ByteAddressBuffer buffer, int byteOffset16ByteAligned)
{
	return buffer.Load< vector<T, N> >(byteOffset16ByteAligned);
}

template<typename T, typename U, INT N>
vector<T, N> vectorCopyFrom(vector<U, N> x)
{
	return vector<T, N>(x);
}

template<typename T, INT N>
vector<T, N> vectorFill(T x)
{
#ifdef _SM_69_
	return x;
#else
	vector<T, N> y;
	y.fill(x);

	return y;
#endif
}

#ifndef _SM_69_
template<typename T, INT N>
vector<T, N> max(vector<T, N> y, T x)
{
	FOR_EACH(n, N, y.Data[n] = max(x, y[n]))

	return y;
}

template<typename T, INT N>
vector<T, N> max(T x, vector<T, N> y)
{
	FOR_EACH(n, N, y.Data[n] = max(x, y[n]))

	return y;
}

template<typename T, INT N>
vector<T, N> min(vector<T, N> y, T x)
{
	FOR_EACH(n, N, y.Data[n] = min(x, y[n]))

	return y;
}

template<typename T, INT N>
vector<T, N> min(T x, vector<T, N> y)
{
	FOR_EACH(n, N, y.Data[n] = min(x, y[n]))

	return y;
}

template<typename T, INT N>
vector<T, N> abs(vector<T, N> x)
{
	FOR_EACH(n, N, x.Data[n] = abs(x[n]))

	return x;
}

template<typename T, INT N>
vector<T, N> rcp(vector<T, N> x)
{
	FOR_EACH(n, N, x.Data[n] = 1.0 / x[n])

	return x;
}

template<typename T, INT N>
vector<T, N> exp(vector<T, N> x)
{
	FOR_EACH(n, N, x.Data[n] = exp(x[n]))

	return x;
}

template<typename T, INT N>
vector<T, N> log(vector<T, N> x)
{
	FOR_EACH(n, N, x.Data[n] = log(x[n]))

	return x;
}

template<typename T, INT N>
vector<T, N> tanh(vector<T, N> x)
{
	FOR_EACH(n, N, x.Data[n] = tanh(x[n]))

	return x;
}

template<typename T, INT N>
vector<T, N> clamp(vector<T, N> x, vector<T, N> a, vector<T, N> b)
{
	FOR_EACH(n, N, x.Data[n] = clamp(x[n], a[n], b[n]))

	return x;
}

template<typename T, INT N>
vector<T, N> clamp(vector<T, N> x, vector<T, N> a, T b)
{
	FOR_EACH(n, N, x.Data[n] = clamp(x[n], a[n], b))

	return x;
}

template<typename T, INT N>
vector<T, N> clamp(vector<T, N> x, T a, vector<T, N> b)
{
	FOR_EACH(n, N, x.Data[n] = clamp(x[n], a, b[n]))

	return x;
}

template<typename T, INT N>
vector<T, N> clamp(vector<T, N> x, T a, T b)
{
	FOR_EACH(n, N, x.Data[n] = clamp(x[n], a, b))

	return x;
}

template<typename T, INT N>
vector<T, N> select(vector<bool, N> c, vector<T, N> x, vector<T, N> y)
{
	FOR_EACH(n, N, y.Data[n] = c[n] ? x[n] : y[n])

	return y;
}

template<INT N, typename T>
vector<T, N> select(vector<bool, N> c, vector<T, N> y, T x)
{
	FOR_EACH(n, N, y.Data[n] = c[n] ? y[n] : x)

	return y;
}

template<typename T, INT N>
vector<T, N> select(vector<bool, N> c, T x, vector<T, N> y)
{
	FOR_EACH(n, N, y.Data[n] = c[n] ? x : y[n])

	return y;
}

template<typename T, INT N>
vector<bool, N> and(vector<bool, N> x, vector<bool, N> y)
{
	FOR_EACH(n, N, y.Data[n] = x[n] && y[n])

	return y;
}

template<INT N>
vector<bool, N> or(vector<bool, N> x, vector<bool, N> y)
{
	FOR_EACH(n, N, y.Data[n] = x[n] || y[n])

	return y;
}
#endif

template<typename T, INT N>
vector<T, N> negate(vector<T, N> x)
{
#ifdef _SM_69_
	x = -x;
#else
	FOR_EACH(n, N, x.Data[n] = -x[n])
#endif

	return x;
}

#endif

#if defined(_SM_69_) || defined(_SLANG_)
#define vectorWriteToIndex(vec, index, value) vec[index] = value
#else
#define vectorWriteToIndex(vec, index, value) vec.writeToIndex(index, value)
#endif
