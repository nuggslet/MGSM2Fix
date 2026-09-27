"""Prepare losslessly compressed native resources; runs only on build machines."""
import ctypes
import io
from pathlib import Path
import sys
import zipfile

source, target = map(Path, sys.argv[1:])
stream = io.BytesIO()
with zipfile.ZipFile(source) as archive, zipfile.ZipFile(stream, 'w', zipfile.ZIP_STORED) as stored:
    for name in archive.namelist():
        info = zipfile.ZipInfo(name, (2020, 1, 1, 0, 0, 0))
        stored.writestr(info, archive.read(name))
data = stream.getvalue()
api = ctypes.WinDLL('cabinet', use_last_error=True)
handle = ctypes.c_void_p()
api.CreateCompressor.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
api.CreateDecompressor.argtypes = api.CreateCompressor.argtypes
api.SetCompressorInformation.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_size_t]
api.Compress.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
api.Decompress.argtypes = api.Compress.argtypes
api.CloseCompressor.argtypes = [ctypes.c_void_p]
api.CloseDecompressor.argtypes = [ctypes.c_void_p]
def check(ok):
    if not ok:
        raise ctypes.WinError(ctypes.get_last_error())
check(api.CreateCompressor(5, None, ctypes.byref(handle)))
try:
    block = ctypes.c_uint(16 * 1024 * 1024)
    check(api.SetCompressorInformation(handle, 1, ctypes.byref(block), 4))
    length = ctypes.c_size_t()
    api.Compress(handle, data, len(data), None, 0, ctypes.byref(length))
    output = ctypes.create_string_buffer(length.value)
    check(api.Compress(handle, data, len(data), output, len(output), ctypes.byref(length)))
    compressed = output.raw[:length.value]
finally:
    api.CloseCompressor(handle)
check(api.CreateDecompressor(5, None, ctypes.byref(handle)))
try:
    restored = ctypes.create_string_buffer(len(data))
    length = ctypes.c_size_t()
    check(api.Decompress(handle, compressed, len(compressed), restored, len(restored), ctypes.byref(length)))
    if length.value != len(data) or restored.raw != data:
        raise ValueError('Recipe compression round-trip mismatch')
finally:
    api.CloseDecompressor(handle)
target.parent.mkdir(parents=True, exist_ok=True)
result = len(data).to_bytes(8, 'little') + compressed
if not target.exists() or target.read_bytes() != result:
    target.write_bytes(result)
print(f'Recipes: {source.stat().st_size:,} byte ZIP -> {len(result):,} byte LZMS; exact round trip verified')
