The kloofendal skybox is based on
https://polyhaven.com/a/kloofendal_48d_partly_cloudy_puresky

The .iblenv format is a custom format.
It stores the mip map levels of an hdr cubemap image.
The 32-bit floating point images are concatenated in a way that makes it easy to upload to the GPU.
The format supports LZ4 compression.

The files were generated using
https://github.com/Qendolin/advanced-gl/tree/master/Project03/cmd/iblconv

- `iblconv.exe convert -compress 2 -size 25% .\kloofendal.hdr`
- `iblconv.exe diffuse -compress 0 -samples 1024 kloofendal.iblenv`
- `iblconv.exe specular -compress 0 .\kloofendal.iblenv`

The ibl_brdf_lut.f32 file was generated using 
https://github.com/Qendolin/advanced-gl/tree/master/Project03/cmd/brdflut

`brdflut.exe ibl_bdrf_lut.f32`

The .f32 format is a custom format.
It stores 2D 32-bit floating point images and supports LZ4 compression.