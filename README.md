# XUSG
XUSG, XU's supreme graphics lib, is a handy wrapper currently for DirectX 12. XUSG includes the following modules:

* XUSG core (XUSGCore), the wrapper of the core functions in graphics API (currently DirectX 12).
* XUSG ray-tracing (XUSGRayTracing), the wrapper of the ray-tracing functions in graphics API (currently native DXR).
* XUSG DXR with fallback layer (XUSGDXRFallback), the wrapper of the ray-tracing functions through DXR [fallback layer] (https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3D12RaytracingFallback). The hardware requirement of DXR fallback layer is backward-compatible to Tier 2 (official Microsoft DXR fallback layer needs Tier 3).

# Samples

Using XUSGCore only:

https://github.com/StarsX/NonuniformBlur

https://github.com/StarsX/IrradianceMap

https://github.com/StarsX/VoxelizerX12

https://github.com/StarsX/ComputeRaster

https://github.com/StarsX/FluidX12

https://github.com/StarsX/ParticleEmitter
