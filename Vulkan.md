# Vulkan 路径状态

范围是 XUSG core 和 Ultimate。不含 ray tracing、EZ、work graph。默认工厂仍是 `API::DIRECTX_12`。Debug x64 能编过 `XUSG.dll`。还没有跑过样例。Release 在后期改动之后没有重编，Win32 没有编过。

`assert` 在 Release（`NDEBUG`）里会被拿掉。下面标成未实现的接口，Release 里调用会静默失败或返回空。

## 未实现

调用会 `assert`，不要调。

| 接口 | 行为 |
| --- | --- |
| `CopyTiles`、`UpdateTileMappings`、`CopyTileMappings` | 空。`GetTiling` assert 后输出填 0 |
| `SOSetTargets`、`SetSamplePositions` | 空 |
| 间接参数 `DISPATCH_RAYS` 以及未列出的 token | 返回 false |
| `CreateRootSignature`、`GetRootSignature`、`CreateRootSignatureFromLibSubobject`、`GetRootSignatureFromLibSubobject` | 返回 nullptr |
| `Resolve` 的 `MIN`、`MAX`、sampler feedback | 返回。`AVERAGE` 和 `DECOMPRESS` 走 `vkCmdResolveImage` |
| `SetStateObject`、`SetProgram`、`DispatchGraph` | 空 |
| `ConvertLinearAlgebraMatrix`；Vulkan 上的 `GetLinearAlgebraMatrixInfo` | assert 后返回 |
| Sampler feedback 的 Create、CreateUAV、descriptor write | 返回 false 或 nullptr |
| `GetProgramIdentifier` | 返回空 |
| Video command list、allocator、queue | assert 类型不是 `VIDEO_*` |
| `WorkGraph` 工厂在 `API::VULKAN` | assert 后仍返回 DX12 类型 |

## 已实现，但有限制

| 接口 | 现状 |
| --- | --- |
| `Texture::ReadBack` | `vkCmdCopyImageToBuffer` 拷进 host-visible 的 `READBACK` buffer，并写入 `pRowPitches`。Row pitch 按 256 对齐；纹素大小不能整除 256 时，pitch 抬到同时是 256 和纹素大小倍数的值。后面的 subresource 从 512 对齐处开始，纹素大小不能整除 512 时同样抬高。起始 offset 必须是纹素大小的倍数。Multisample 会 assert。Depth/stencil 只拷 depth aspect。Stencil plane 不在跟踪的 subresource 里，越界会 assert。返回时拷贝还在 command list 上，要等 fence 再 map |
| `ClearUnorderedAccessView` | Buffer 用 `vkCmdFillBuffer`，只重复 4 字节，取 `color.uint32[0]`。Image 用 `vkCmdClearColorImage`。矩形会 assert。Texel buffer 只接受 `R32_UINT`、`R32_SINT`、`R32_SFLOAT`。stride 大于 4 且各分量不一致时 assert。不做 UNORM 格式转换 |
| `DiscardResource` | 只支持整张 image 的 subresource。下一道 image barrier 的 old layout 是 `UNDEFINED`，src access 是 `NONE`。RT/DS 整份 view 都被 discard 时 load op 为 `DONT_CARE`。矩形和 buffer 会 assert。`numSubresources == 0` 表示全部。同一 view 里只有部分 subresource 被 discard 时仍用 `LOAD`，但那些标记会被消费。这个 SDK 的 depth/stencil 共用一个 load op |
| Input layout | DX12 和 Vulkan 共用 `InputLayoutLib_Impl`（`XUSGInputLayout.h`）。建 pipeline 时拆成 vertex binding 和 attribute。`location` 是元素下标，`SemanticName` 不参与匹配。`InstanceDataStepRate` 不是 1 时写入 divisor state |
| 枚举映射 | 稠密枚举和单 bit flag 用数组下标。`XUSG.h` 里的值不能重排。DX12、Vulkan、Ultimate 的表都有顺序 assert。`GetVKLogicOp` 按名字映射，不再 `static_cast`。`GetDX12ClearFlag` 的表已去掉错误的前导 0 |
| 宽字符 | `WideToUtf8` 按 `wchar_t` 宽度编码 UTF-8。2 字节处理 UTF-16 代理对，4 字节每个值是一个码点 |
| Shader 文件 | Windows 用 `_wfopen_s`。其他平台先 `WideToUtf8` 再 `fopen`，然后 `fread` |
| Fence event | `XUSG_SIGNAL_EVENT`：Windows 调 `SetEvent`。其他平台传入 event 时 assert。等待仍是 `vkWaitSemaphores` |
| `SetPredication` | 设备有 `VK_EXT_conditional_rendering` 时启用。谓词取 buffer 里 offset 处的低 32 位。`opEqualZero` 使用 `VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT`。扩展不在时调用会 assert。Draw、Dispatch、ExecuteIndirect、DispatchMesh 包进 begin/end；copy 和 barrier 之前会先结束 |
| Conservative raster | 设备有 `VK_EXT_conservative_rasterization` 时启用。`ConservativeRaster` 为真时挂 `OVERESTIMATE`。扩展不在时创建这样的 pipeline 会 assert |
| Null descriptor | 资源 heap 的空槽用 `vkWriteResourceDescriptorsEXT` 写 null，需要 `nullDescriptor`。Image descriptor 不比 buffer 小就写 sampled image，否则写 storage buffer。Sampler 没有 null descriptor，空 sampler 仍是 0 |
| Depth/stencil view | SRV 按 plane 只带 depth 或 stencil。DSV 仍同时带两个 aspect，给 dynamic rendering 的 depth/stencil attachment 用。Copy 的 aspect 只取 depth |
| 初始 layout | 每个 image subresource 单独从 `UNDEFINED` 开始。第一道碰到它的 barrier 消费这个标记 |
| Image barrier | Image 不再把 `COMMON` 到 copy 状态当成可提升而跳过 |
| Placed heap | DEFAULT heap 的 memory type 取 buffer 与 color/depth image 的 `memoryTypeBits` 交集。绑定 placed 资源时，heap 的 type 不在资源的 bits 里会 assert |
| READBACK | 优先 `HOST_VISIBLE \| HOST_COHERENT \| HOST_CACHED`。没有 cached 时退回 visible + coherent |
| Castable format | `numCastableFormats` 非 0 时，image 带 `VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT` 和 `VkImageFormatListCreateInfo` |
| `InstanceDataStepRate` | 不是 1 时写入 `VkPipelineVertexInputDivisorStateCreateInfo`。需要 vertex attribute divisor。Step rate 为 0 会 assert |
| `patchControlPoints` | 从 hull shader SPIR-V 的 Input 数组长度读取，这是 input control point 数。`OutputVertices` 是 output 数量，不用。扫不到时 assert，然后按 1 创建。dxil-spirv 把 hull 输入数组写成 32，那种 SPIR-V 读出来是 32。`IASetPrimitiveTopology` 的 `CONTROL_POINTN` 都是 `PATCH_LIST`，不会改 pipeline 里已经定下的这个数 |
| Queue | `DIRECT`、`COMPUTE`、`COPY` 按 vkd3d-proton 选 family。Compute 优先没有 graphics 的 family，copy 优先纯 transfer，没有就回退。每个 family 最多 4 条 VkQueue，更多的 XUSG queue 复用负载最小的那条。`XUSG_SINGLE_QUEUE` 非 0，或 `VKD3D_CONFIG` 里有 `single_queue` 时，三种都用 graphics family 的 queue 0。priority 和 `CommandQueueFlag` 仍忽略。Video 类型会 assert。多个 family 时 buffer 和 image 用 `VK_SHARING_MODE_CONCURRENT` |
| 交换链 | `GetCurrentBackBufferIndex` 和 `Present` 调用 `vkAcquireNextImageKHR`。Acquire semaphore 由创建交换链时传入的那条 queue 的下一次 submit 等待。同一条 VkQueue 上只有一个等待槽，两条交换链会互相覆盖。`Present` 再空提交一次，发出 per-image render-finished，然后 `vkQueuePresentKHR` 等它。`syncInterval` 不改变 present mode，模式在创建时按 `ALLOW_TEARING` 选 `IMMEDIATE`，否则 `FIFO`。Present 之后跟踪的 state 不会变成 `PRESENT_SRC_KHR`。画到 back buffer 的 command list 必须提交到 `SwapChain::Create` 传入的那条 queue |

## 明确延后

全面支持 Linux 以后再说。DirectXMath 是 MIT 头文件库，Linux 上 GCC 10+ 和 Clang 12+ 能编，非 Windows 需要自备 `sal.h`。这次不把它从 `stdafx.h` 里拆出来。

| 项 | 原因 |
| --- | --- |
| 交换链 surface | `SwapChain_VK::Create` 把 `hWnd` 当 `HWND`，调用 `vkCreateWin32SurfaceKHR` 和 `GetModuleHandle` |
| Instance 扩展 | `createInstance` 无条件启用 `VK_KHR_win32_surface`。`XUSG_VK.h` 在包含 `vulkan.h` 前定义了 `VK_USE_PLATFORM_WIN32_KHR` |
| `stdafx.h` | 由 vcxproj 的 `ForcedIncludeFiles` 引进，不是源码里的 include。里面有 `windows.h`、d3d12、dxgi、D3DCompiler、DirectXMath、winrt，以及 `__declspec(dllexport)` |

## 遗留行为

这些能编过，运行时和 DX12 不完全一样。已经按约定实现、不再列在这里的有：graphics/compute 共用 push data、`DiscardResource`、多条 queue、`vkAcquireNextImageKHR`、`patchControlPoints`、空的 `VkPipelineLayout`、mapping 按 shader stage 过滤、设备创建时启用用到的 EXT/KHR、`IASetVertexBuffers` 的 `pViews == nullptr` 解绑。Push data 的约定写在 `XUSG.h` 的 `SetComputePipelineLayout` 上面。`pViews == nullptr` 时 DX12 把空指针交给 `IASetVertexBuffers`，Vulkan 对每个 slot 绑定 `VK_NULL_HANDLE`。

- Bundle 不继承 dynamic rendering。执行 bundle 前会先结束 rendering。
- Viewport 和 scissor 的 command list 上限是 16，和 DX12 一样。Graphics pipeline 里这两项是动态状态，`viewportCount` 和 `scissorCount` 仍是 1，多出来的 viewport 不会被这条 pipeline 使用。
- View instancing 写进 PSO key，没有变成 multiview。Vulkan 的 `SetNumViewInstances` 会保存 `ViewInstanceFlags`。
- Shading-rate image 挂在下一次 `beginRendering` 上。`SUM` combiner 映射成 Vulkan 的 `MUL`。
- 普通 pipeline 的入口点是 `"main"`。Generic pipeline 用 shader 名的 UTF-8。
- SPIR-V reflector 只解析 `OpName` 和 `Binding`，不解析 descriptor set。
- UAV counter 存在对象里，不会自动写进 descriptor。
- 没有启用 `samplerFilterMinmax`，min/max reduction 可能在创建 sampler 时失败。
- Line rasterization 没有写进 pipeline。
- Split barrier 只在 `END_ONLY` 或 `NONE` 时发出。
- `vkCmdBindVertexBuffers2EXT` 不存在时改用 `vkCmdBindVertexBuffers`，vertex buffer view 的 size 和 stride 被忽略。Stride 用 pipeline 里从 input layout 推出来的值。
- 没有对应 VkFormat 的格式会 assert 并返回 `VK_FORMAT_UNDEFINED`。`GetVKFilter` 是近似映射。
- `ReadFromSubresource` / `WriteToSubresource` 只在 host-visible 时 `memcpy`，忽略 box。
- 3D UAV 忽略 `wSize` / `firstWSlice`。Texture3D 的 `GetAllocationInfo` 可能忽略 depth。
- 不同资源视图种类共用同一个 space 时，Vulkan 不能共存。调用方自己避开。
- 间接 index buffer 使用 `VK_INDIRECT_COMMANDS_INPUT_MODE_DXGI_INDEX_BUFFER_EXT`。这是扩展枚举，参数的二进制布局按 DXGI index buffer。
