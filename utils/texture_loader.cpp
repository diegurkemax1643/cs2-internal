#include "texture_loader.h"
#include <Windows.h>
#include <gdiplus.h>
#include <vector>
#include <fstream>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Helper function to get encoder CLSID
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    ImageCodecInfo* pImageCodecInfo = NULL;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;
    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

ID3D11ShaderResourceView* TextureLoader::LoadTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& filepath) {
    if (!device) return nullptr;
    
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Convert string to wstring
    std::wstring wfilepath(filepath.begin(), filepath.end());
    
    // Load image using GDI+
    Bitmap* bitmap = new Bitmap(wfilepath.c_str());
    if (bitmap->GetLastStatus() != Ok) {
        delete bitmap;
        GdiplusShutdown(gdiplusToken);
        return nullptr;
    }
    
    UINT width = bitmap->GetWidth();
    UINT height = bitmap->GetHeight();
    
    // Create D3D11 texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    // Lock bitmap and get pixel data
    BitmapData bitmapData;
    Rect rect(0, 0, width, height);
    if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Ok) {
        delete bitmap;
        GdiplusShutdown(gdiplusToken);
        return nullptr;
    }
    
    // Create texture with pixel data
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = bitmapData.Scan0;
    initData.SysMemPitch = bitmapData.Stride;
    initData.SysMemSlicePitch = 0;
    
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);
    
    bitmap->UnlockBits(&bitmapData);
    delete bitmap;
    GdiplusShutdown(gdiplusToken);
    
    if (FAILED(hr) || !texture) {
        return nullptr;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    
    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);
    texture->Release();
    
    if (FAILED(hr) || !srv) {
        return nullptr;
    }
    
    return srv;
}

void TextureLoader::ReleaseTexture(ID3D11ShaderResourceView* texture) {
    if (texture) {
        texture->Release();
    }
}

