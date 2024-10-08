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
#pragma once
namespace FallbackLayer
{
    class LoadInstancesPass
    {
    public:
        LoadInstancesPass();

        void CreateTopLevelPipeline(ID3D12Device* pDevice, UINT nodeMask, UINT numUAVs);

        void LoadInstances(ID3D12GraphicsCommandList *pCommandList, 
            D3D12_GPU_VIRTUAL_ADDRESS outputBVH, 
            D3D12_GPU_VIRTUAL_ADDRESS instancesDesc, 
            D3D12_ELEMENTS_LAYOUT instanceDescLayout, 
            UINT numElements, 
            D3D12_GPU_DESCRIPTOR_HANDLE descriptorHeapBase,
            D3D12_GPU_VIRTUAL_ADDRESS cachedSortBuffer);
    private:
        enum RootParameterSlot
        {
            OutputBVHRootUAV = 0,
            InstanceDescsSRV,
            Constants,
            GlobalDescriptorHeap,
            GlobalDescriptorHeapSRV,
            CachedSortBuffer,
            NumRootParameters,
        };

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pLoadAABBsFromArrayOfPointersPSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pLoadAABBsFromArrayOfInstancesPSO;
    };
}
