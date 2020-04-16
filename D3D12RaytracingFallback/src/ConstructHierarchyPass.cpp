//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
#include "pch.h"
#include "ConstructHierarchyBindings.h"
#include "CompiledShaders/TopLevelBuildBVHSplits.h"
#include "CompiledShaders/BottomLevelBuildBVHSplits.h"

namespace FallbackLayer
{
    ConstructHierarchyPass::ConstructHierarchyPass(ID3D12Device *pDevice, UINT nodeMask, UINT numUAVs)
    {
        D3D12_DESCRIPTOR_RANGE1 globalDescriptorHeapRange = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAVs, GlobalDescriptorHeapRegister, GlobalDescriptorHeapRegisterSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
        CD3DX12_ROOT_PARAMETER1 rootParameters[NumRootParameters];
        rootParameters[HierarchyUAVParam].InitAsUnorderedAccessView(HierarchyBufferRegister);
        rootParameters[MortonCodesBufferParam].InitAsUnorderedAccessView(MortonCodesBufferRegister);
        rootParameters[InputRootConstants].InitAsConstants(SizeOfInUint32(InputConstants), InputConstantsRegister);
        rootParameters[GlobalDescriptorHeap].InitAsDescriptorTable(1, &globalDescriptorHeapRange);

        auto rootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(ARRAYSIZE(rootParameters), rootParameters);
        CreateRootSignatureHelper(pDevice, rootSignatureDesc, &m_pRootSignatures[Level::Top]);

        rootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(ARRAYSIZE(rootParameters) - 1, rootParameters);
        CreateRootSignatureHelper(pDevice, rootSignatureDesc, &m_pRootSignatures[Level::Bottom]);

        CreatePSOHelper(pDevice, nodeMask, m_pRootSignatures[Level::Top], COMPILED_SHADER(g_pTopLevelBuildBVHSplits), &m_pBuildSplits[Level::Top]);

        CreatePSOHelper(pDevice, nodeMask, m_pRootSignatures[Level::Bottom], COMPILED_SHADER(g_pBottomLevelBuildBVHSplits), &m_pBuildSplits[Level::Bottom]);
    }

    void ConstructHierarchyPass::ConstructHierarchy(ID3D12GraphicsCommandList *pCommandList,
        SceneType sceneType,
        D3D12_GPU_VIRTUAL_ADDRESS mortonCodeBuffer,
        D3D12_GPU_VIRTUAL_ADDRESS hierarchyBuffer,
        D3D12_GPU_DESCRIPTOR_HANDLE globalDescriptorHeap,
        UINT numElements)
    {
        if (numElements == 0) return;

        Level level = (sceneType == SceneType::Triangles) ? Level::Bottom : Level::Top;

        InputConstants constants = { numElements };

        pCommandList->SetComputeRootSignature(m_pRootSignatures[level]);
        pCommandList->SetComputeRoot32BitConstants(InputRootConstants, SizeOfInUint32(InputConstants), &constants, 0);
        pCommandList->SetComputeRootUnorderedAccessView(MortonCodesBufferParam, mortonCodeBuffer);
        pCommandList->SetComputeRootUnorderedAccessView(HierarchyUAVParam, hierarchyBuffer);
        if (level == Top)
        {
            pCommandList->SetComputeRootDescriptorTable(GlobalDescriptorHeap, globalDescriptorHeap);
        }

        // Only given the GPU VA not the resource itself so need to resort to doing an overarching UAV barrier
        const UINT dispatchWidth = DivideAndRoundUp<UINT>(numElements, THREAD_GROUP_1D_WIDTH);
        auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);

        pCommandList->SetPipelineState(m_pBuildSplits[level]);
        pCommandList->Dispatch(dispatchWidth, 1, 1);
        pCommandList->ResourceBarrier(1, &uavBarrier);
    }
}
