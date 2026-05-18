//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen and Yang, Jiale. All rights reserved.
//--------------------------------------------------------------------------------------

#define WAVE_SIZE 32

#ifndef GROUP_SIZE
#define GROUP_SIZE WAVE_SIZE
#endif

#ifndef ACC_T
#define ACC_T float16_t
#endif

#define dot2add(a, b, acc) (dot(a, b) + float(acc))

#define FOR_EACH(n, N, statement) for (INT n = 0; n < N; ++n) { statement; }

#ifdef _SLANG_

#define __SHADER_TARGET_MINOR 9
#define INT int
#define FLOAT_TYPE_NAME(T) T : __BuiltinFloatingPointType
#define TEMPLATE_FUNC(retType, func, ...) retType func<__VA_ARGS__>

#define vectorLoad	coopVecLoad
#define vector		CoopVec

enum class ComponentType
{
	I16 = CoopVecComponentType::SignedInt16,
	U16 = CoopVecComponentType::UnsignedInt16,
	I32 = CoopVecComponentType::SignedInt32,
	U32 = CoopVecComponentType::UnsignedInt32,
	I64 = CoopVecComponentType::SignedInt64,
	U64 = CoopVecComponentType::UnsignedInt64,
	F16 = CoopVecComponentType::Float16,
	F32 = CoopVecComponentType::Float32,
	F64 = CoopVecComponentType::Float64,
	PackedS8x32 = CoopVecComponentType::SignedInt8Packed,
	PackedU8x32 = CoopVecComponentType::UnsignedInt8Packed,
	I8 = CoopVecComponentType::SignedInt8,
	U8 = CoopVecComponentType::UnsignedInt8,
	F8_E4M3FN = CoopVecComponentType::FloatE4M3,
	F8_E5M2 = CoopVecComponentType::FloatE5M2
};
typedef ComponentType ComponentEnum;

enum class MatrixLayout
{
	RowMajor = CoopVecMatrixLayout::RowMajor,
	ColMajor = CoopVecMatrixLayout::ColumnMajor,
	MulOptimal = CoopVecMatrixLayout::InferencingOptimal,
	MulOptimalTranspose = CoopVecMatrixLayout::InferencingOptimal,
	OuterProductOptimal = CoopVecMatrixLayout::TrainingOptimal,
	OuterProductOptimalTranspose = CoopVecMatrixLayout::TrainingOptimal
};
typedef MatrixLayout MatrixLayoutEnum;

vector<T, M> matVecMulAdd<T : __BuiltinArithmeticType, INT M, ComponentEnum inputDataType,
	ComponentEnum matrixDataType, ComponentEnum biasDataType, MatrixLayoutEnum matrixLayout,
	bool transpose, INT K, U : __BuiltinArithmeticType>(
	vector<U, K> input,
	ByteAddressBuffer matrixBuffer,
	int matrixOffset,
	ByteAddressBuffer biasBuffer,
	int biasOffset,
	constexpr uint matrixStride,
	uint gi : SV_GroupIndex)
{
	if (inputDataType == ComponentEnum::PackedS8x32 || inputDataType == ComponentEnum::PackedU8x32)
		return coopVecMatMulAddPacked<T, M>(input, (CoopVecComponentType)inputDataType, sizeof(U) * K, matrixBuffer, matrixOffset, (CoopVecComponentType)matrixDataType,
			biasBuffer, biasOffset, (CoopVecComponentType)biasDataType, (CoopVecMatrixLayout)matrixLayout, transpose, matrixStride);
	else return coopVecMatMulAdd<T, M>(input, (CoopVecComponentType)inputDataType, matrixBuffer, matrixOffset, (CoopVecComponentType)matrixDataType,
			biasBuffer, biasOffset, (CoopVecComponentType)biasDataType, (CoopVecMatrixLayout)matrixLayout, transpose, matrixStride);
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

#include <dx/linalg.h>

#define FLOAT_TYPE_NAME(T) typename T
#define TEMPLATE_FUNC(retType, func, ...) template<__VA_ARGS__> retType func

#if ((__SHADER_TARGET_MAJOR > 6) || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 9)) && (__HLSL_VERSION >= 2021)

using namespace dx;
using namespace dx::linalg;

#define INT int
#define _SM_6_9_

#if __SHADER_TARGET_MINOR >= 10
#define _SM_6_10_
#else
struct MatrixLayout
{
	enum MatrixLayoutEnum
	{
		RowMajor = MATRIX_LAYOUT_ROW_MAJOR,
		ColMajor = MATRIX_LAYOUT_COLUMN_MAJOR,
		MulOptimal = MATRIX_LAYOUT_MUL_OPTIMAL,
		MulOptimalTranspose = MATRIX_LAYOUT_MUL_OPTIMAL,
		OuterProductOptimal = MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL,
		OuterProductOptimalTranspose = MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL
	};
};
using MatrixLayoutEnum = ::MatrixLayout::MatrixLayoutEnum;
#endif

#else

#define INT uint

struct MatrixLayout
{
	enum MatrixLayoutEnum
	{
		RowMajor = 0,
		ColMajor = 1,
		MulOptimal = 2,
		MulOptimalTranspose = 3,
		OuterProductOptimal = 4,
		OuterProductOptimalTranspose = 5
	};
};
using MatrixLayoutEnum = MatrixLayout::MatrixLayoutEnum;

#endif

#ifndef _SM_6_10_
struct ComponentType
{
	enum ComponentEnum
	{
		//Invalid = 0,
		//I1 = 1,
		I16 = 2,
		U16 = 3,
		I32 = 4,
		U32 = 5,
		I64 = 6,
		U64 = 7,
		F16 = 8,
		F32 = 9,
		F64 = 10,
		//SNormF16 = 11,
		//UNormF16 = 12,
		//SNormF32 = 13,
		//UNormF32 = 14,
		//SNormF64 = 15,
		//UNormF64 = 16,
		PackedS8x32 = 17,
		PackedU8x32 = 18,
		I8 = 19,
		U8 = 20,
		F8_E4M3FN = 21,
		F8_E5M2 = 22
	};
};
using ComponentEnum = ComponentType::ComponentEnum;
#endif

#ifdef _SM_6_9_
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

groupshared float16_t g_tensorf[WAVE_TENSOR_SIZE * NUM_GROUP_WAVES];
groupshared ACC_T g_matrixCf[WAVE_MATRIX_C_SIZE * NUM_GROUP_WAVES];
groupshared float16_t g_matrixAf[WAVE_MATRIX_A_SIZE * NUM_GROUP_WAVES];

groupshared int8_t4_packed g_tensori[WAVE_TENSOR_SIZE * NUM_GROUP_WAVES];
groupshared int g_matrixCi[WAVE_MATRIX_C_SIZE * NUM_GROUP_WAVES];
//groupshared int8_t4_packed g_matrixAi[WAVE_MATRIX_A_SIZE * NUM_GROUP_WAVES];

groupshared uint8_t4_packed g_tensoru[WAVE_TENSOR_SIZE * NUM_GROUP_WAVES];
//groupshared uint8_t4_packed g_matrixAu[WAVE_MATRIX_A_SIZE * NUM_GROUP_WAVES];

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
		for (uint k = 0; k < WM_K / 2; ++k) g_matrixAf[WAVE_MATRIX_A_SIZE * waveId + (WM_K * (m % WM_M) + b + k)] = U(v[k]);
	}
	else if (WM_K == 32)
	{
		const Vector<T, 16> v = matrixBuffer.Load< Vector<T, 16> >(p + sizeof(T) * b);
		for (uint k = 0; k < WM_K / 2; ++k) g_matrixAf[WAVE_MATRIX_A_SIZE * waveId + (WM_K * (m % WM_M) + b + k)] = U(v[k]);
	}
	else for (uint k = 0; k < WM_K / 2; ++k)
	{
		const T a = matrixBuffer.Load<T>(p + sizeof(T) * (b + k));
		g_matrixAf[WAVE_MATRIX_A_SIZE * waveId + (WM_K * (m % WM_M) + b + k)] = U(a);
	}

	GroupMemoryBarrierWithGroupSync();

	A.Load(g_matrixAf, WAVE_MATRIX_A_SIZE * waveId, WM_K, transpose);
}

template<ComponentEnum dataType, typename T, INT M>
void GetTensor(out Vector<T, M> y, uint offsetOut, uint offsetIn, uint laneMask, uint waveId)
{
	const uint n = WaveGetLaneIndex();
	for (uint m = 0; m < WM_M; ++m)
	{
		if ((1u << n) & laneMask)
		{
			if (dataType == ComponentEnum::I32) y.Data[offsetOut + m] = T(g_matrixCi[WAVE_MATRIX_C_SIZE * waveId + (offsetIn + WM_M * n + m)]);
			else y.Data[offsetOut + m] = T(g_matrixCf[WAVE_MATRIX_C_SIZE * waveId + (offsetIn + WM_M * n + m)]);
		}
	}
		
}

template<typename T, INT M, ComponentEnum inputDataType, ComponentEnum matrixDataType, ComponentEnum biasDataType,
	MatrixLayoutEnum matrixLayout, bool transpose, typename U, INT K>
Vector<T, M> matVecMulAdd(
	Vector<U, K> input,
	ByteAddressBuffer matrixBuffer,
	int matrixOffset,
	ByteAddressBuffer biasBuffer,
	int biasOffset,
	uint matrixStride,
	uint gi : SV_GroupIndex)
{
	Vector<T, M> output;

	WaveMatrixLeft<U, WM_M, WM_N> A;
	WaveMatrixLeftColAcc<T, WM_M, WM_N> B;

	const bool isInputInteger8 = inputDataType == ComponentEnum::I8 || inputDataType == ComponentEnum::U8;
	const uint vecElementSize = isInputInteger8 ? 1 : sizeof(U);
	const uint kStep = isInputInteger8 ? 4 : 1;

	const uint waveId = WaveReadLaneAt(gi / WaveGetLaneCount(), 0);
	const uint n = WaveGetLaneIndex();
	const uint WM_K = A.MatrixDepth();

	const uint c = vecElementSize * min(WM_M, M); // Aligned column offset
	const uint s = vecElementSize * min(WM_K, K); // Aligned stride

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

			// Row offset
			const uint r = mOffset + vecElementSize * m * K;

			if (biasDataType == ComponentEnum::I32)
			{
				WaveMatrixAccumulator<int, WM_M, WM_N> C0, C1;
				C0.Fill(0);
				C0.Add(B);
				C1.Fill(0);
				C1.Add(C0);

				//[unroll]
				for (uint k = 0; k < K; k += kStep * WM_K)
				{
					// Store x into group-shared memory
					if (matrixDataType == ComponentEnum::U8)
					{
						for (uint i = 0; i < WM_K; ++i)
						{
							g_tensoru[WAVE_TENSOR_SIZE * waveId + (WM_K * n + i)] = isInputInteger8 ?
								pack_clamp_u8(int4(input.Data[k + i], input.Data[k + kStep * i + 1],
								input.Data[k + kStep * i + 2], input.Data[k + kStep * i + 3])) :
								uint8_t4_packed(input.Data[k + i]);
						}
					}
					else
					{
						for (uint i = 0; i < WM_K; ++i)
						{
							g_tensori[WAVE_TENSOR_SIZE * waveId + (WM_K * n + i)] = isInputInteger8 ?
								pack_clamp_s8(int4(input.Data[k + i], input.Data[k + kStep * i + 1],
								input.Data[k + kStep * i + 2], input.Data[k + kStep * i + 3])) :
								int8_t4_packed(input.Data[k + i]);
						}
					}

					GroupMemoryBarrierWithGroupSync();
					A.Load(matrixBuffer, r + c * k, s, transpose);

					// Load x from group-shared memory and perform WMMA
					if (matrixDataType == ComponentEnum::U8)
					{
						WaveMatrixRight<uint8_t4_packed, WM_M, WM_N> X;
						X.Load(g_tensoru, WAVE_TENSOR_SIZE * waveId, WM_K, !transpose);
						C0.MultiplyAccumulate(A, X);
						X.Load(g_tensoru, WAVE_TENSOR_SIZE * waveId + WM_M * WM_K, WM_K, !transpose);
						C1.MultiplyAccumulate(A, X);
					}
					else
					{
						WaveMatrixRight<int8_t4_packed, WM_M, WM_N> X;
						X.Load(g_tensori, WAVE_TENSOR_SIZE * waveId, WM_K, !transpose);
						C0.MultiplyAccumulate(A, X);
						X.Load(g_tensori, WAVE_TENSOR_SIZE * waveId + WM_M * WM_K, WM_K, !transpose);
						C1.MultiplyAccumulate(A, X);
					}
				}

				C0.Store(g_matrixCi, WAVE_MATRIX_C_SIZE * waveId, WM_M, !transpose);
				C1.Store(g_matrixCi, WAVE_MATRIX_C_SIZE * waveId + WM_M * WM_N, WM_M, !transpose);
				GetTensor<biasDataType>(output, m, 0, laneMask, waveId);
			}
			else
			{
				WaveMatrixAccumulator<ACC_T, WM_M, WM_N> C0, C1;
				C0.Fill(0.0);
				C0.Add(B);
				C1.Fill(0.0);
				C1.Add(C0);

				//[unroll]
				for (uint k = 0; k < K; k += WM_K)
				{
					// Store x into group-shared memory
					for (uint i = 0; i < WM_K; ++i)
						g_tensorf[WAVE_TENSOR_SIZE * waveId + (WM_K * n + i)] = input.Data[k + i];

#if 1
					LoadMatrix<U, transpose>(A, matrixBuffer, r + c * k, s, waveId);
#else
					GroupMemoryBarrierWithGroupSync();
					A.Load(matrixBuffer, r + c * k, s, transpose);
#endif

					// Load x from group-shared memory
					WaveMatrixRight<U, WM_M, WM_N> X;
					X.Load(g_tensorf, WAVE_TENSOR_SIZE * waveId, WM_K, !transpose);
					C0.MultiplyAccumulate(A, X);
					X.Load(g_tensorf, WAVE_TENSOR_SIZE * waveId + WM_M * WM_K, WM_K, !transpose);
					C1.MultiplyAccumulate(A, X);
				}

				C0.Store(g_matrixCf, WAVE_MATRIX_C_SIZE * waveId, WM_M, !transpose);
				C1.Store(g_matrixCf, WAVE_MATRIX_C_SIZE * waveId + WM_M * WM_N, WM_M, !transpose);
				GetTensor<biasDataType>(output, m, 0, laneMask, waveId);
			}
		}
	}

	return output;
}

#undef N

#else

template<typename T, INT M, ComponentEnum inputDataType, ComponentEnum matrixDataType, ComponentEnum biasDataType,
	MatrixLayoutEnum matrixLayout, bool transpose, INT K, typename U>
Vector<T, M> matVecMulAdd(
	Vector<U, K> input,
	ByteAddressBuffer matrixBuffer,
	uint matrixOffset,
	ByteAddressBuffer biasBuffer,
	uint biasOffset,
	uint matrixStride,
	uint gi : SV_GroupIndex)
{
#ifdef _SM_6_10_
	using MatrixA = Matrix<matrixDataType, M, K, transpose ? MatrixUse::B : MatrixUse::A, MatrixScope::Thread>;
	const MatrixA mA = MatrixA::template Load<matrixLayout>(matrixBuffer, matrixOffset, matrixStride);
	const VectorRef<biasDataType, M> b = { biasBuffer, biasOffset };

	return MultiplyAdd<T>(mA, MakeInterpretedVector<inputDataType>(input), b);
#elif defined(_SM_6_9_)
	const MatrixRef<(DataType)matrixDataType, M, K, (linalg::MatrixLayout)matrixLayout, transpose> mA = { matrixBuffer, matrixOffset, matrixStride };
	const VectorRef<(DataType)biasDataType> b = { biasBuffer, biasOffset };

	return MulAdd<T>(mA, MakeInterpretedVector<(DataType)inputDataType>(input), b);
#else
	Vector<T, M> output = biasBuffer.Load< Vector<T, M> >(biasOffset);

	const bool isInputInteger8 = inputDataType == ComponentEnum::I8 || inputDataType == ComponentEnum::U8;
	const uint vecElementSize = isInputInteger8 ? 1 : sizeof(U);

	uint kStep;
	if (matrixDataType == ComponentEnum::F16) kStep = 2; // FP16
	else if (isInputInteger8) kStep = 4;                 // I8/U8
	else kStep = 1;                                      // PackedS8x32/U32/Others

	for (uint m = 0; m < M; ++m)
	{
		const uint c = matrixOffset + vecElementSize * K * m;

		for (uint k = 0; k < K; k += kStep)
		{
			switch (matrixDataType)
			{
			case ComponentEnum::F16:
			{
				const vector<U, 2> a = matrixBuffer.Load< vector<U, 2> >(c + vecElementSize * k);
				const vector<U, 2> x = vector<U, 2>(input.Data[k], input.Data[k + 1]);
				output.Data[m] = T(dot2add(a, x, output.Data[m]));
				break;
			}
			case ComponentEnum::I8:
			{
				const int8_t4_packed a = matrixBuffer.Load<int8_t4_packed>(c + vecElementSize * k);
				const int8_t4_packed x = isInputInteger8 ?
					pack_clamp_s8(int4(input.Data[k], input.Data[k + 1], input.Data[k + 2], input.Data[k + 3])) :
					int8_t4_packed(input.Data[k]);
				output.Data[m] = T(dot4add_i8packed(a, x, output.Data[m]));
				break;
			}
			case ComponentEnum::U8:
			{
				const uint8_t4_packed a = matrixBuffer.Load<uint8_t4_packed>(c + vecElementSize * k);
				const uint8_t4_packed x = isInputInteger8 ?
					pack_clamp_u8(int4(input.Data[k], input.Data[k + 1], input.Data[k + 2], input.Data[k + 3])) :
					uint8_t4_packed(input.Data[k]);
				output.Data[m] = T(dot4add_u8packed(a, x, output.Data[m]));
				break;
			}
			default:
				output.Data[m] += matrixBuffer.Load<U>(c + vecElementSize * k) * input.Data[k];
			}
		}
	}

	return output;
#endif
}

#endif

#ifndef _SM_6_9_
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
#ifdef _SM_6_9_
	return x;
#else
	vector<T, N> y;
	y.fill(x);

	return y;
#endif
}

#ifndef _SM_6_9_
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
#ifdef _SM_6_9_
	x = -x;
#else
	FOR_EACH(n, N, x.Data[n] = -x[n])
#endif

	return x;
}

#endif

#if defined(_SM_6_9_) || defined(_SLANG_)
#define vectorWriteToIndex(vec, index, value) vec[index] = value
#else
#define vectorWriteToIndex(vec, index, value) vec.writeToIndex(index, value)
#endif
