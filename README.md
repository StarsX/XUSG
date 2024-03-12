# XUSG
XUSG, XU's supreme graphics lib, is a handy wrapper currently for DirectX 12. XUSG includes the following modules:

* XUSG core (XUSGCore), the wrapper of the core functions in graphics API (currently DirectX 12).
* XUSG ray-tracing (XUSGRayTracing), the wrapper of the ray-tracing functions in graphics API, including the ray-tracing pipleine functions through [DXR fallback layer](https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3D12RaytracingFallback), which supports both native DXR and compute emulation. The hardware requirement of DXR fallback layer is backward-compatible to Tier 2 (official Microsoft DXR fallback layer needs Tier 3).
* XUSG core with ultimate extensions, the wrapper of the extension functions in graphics API (currently DirectX 12 Ultimate).
* XUSG-EZ, DirectX 11-style wrapper of XUSG core. It exposes pipeline state machine, auto implicit barrier settings, and auto implicit descriptor table compositions.
* XUSGRayTracing-EZ, DirectX 11-style wrapper of XUSGRayTracing. It exposes pipeline state machine, auto implicit barrier settings, auto implicit descriptor table compositions, simplified acceleration-structure building with implicit scratch-buffer allocations, and implicit shader-table creations. What's more, it supports both native and DXR-fallback path of ray tracing.
* XUSG machine learning, the wrapper of the machine learning functions, currently based on DirectML (DML).

# Samples

Using XUSGCore only:

https://github.com/StarsX/VolumeRender

https://github.com/StarsX/MIPGen

https://github.com/StarsX/IrradianceMap

https://github.com/StarsX/VoxelizerX12

https://github.com/StarsX/ComputeRaster

https://github.com/StarsX/ParticleEmitter

Using XUSGCore and partially open-source XUSGAdvanced:

https://github.com/StarsX/Character12

Using XUSGCore with ultimate extensions (mesh shader related):

https://github.com/StarsX/MeshShader

https://github.com/StarsX/MSFallback

Using XUSGRayTracing (supporting Tier 2 hardware with DXR fallback layer):

https://github.com/StarsX/RayTracedGGX

https://github.com/StarsX/MultiVolumes

Using XUSGRayTracing (native DXR path only):

https://github.com/StarsX/SparseVolumeDXR (DXR fallback layer has a bug on IgnoreHit() to implement the algorithm, but the partial demo can run without DXR.)

Using XUSG-EZ:

https://github.com/StarsX/FluidX12

https://github.com/StarsX/NonuniformBlur

https://github.com/StarsX/SHIrradianceEZ

Using XUSGRayTracing-EZ:

https://github.com/StarsX/DXRVoxelizer (supporting Tier 2 hardware with DXR fallback layer)

https://github.com/StarsX/RayTracedSPH

Using XUSGMachineLearning:

https://github.com/StarsX/SuperResolution (reproducing DML SuperResolution sample)

Interops:

https://github.com/StarsX/OclDX12Interop

https://github.com/StarsX/Amp12Interop

