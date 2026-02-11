#pragma once
#include <d3d11.h>
#include <string>

namespace TextureLoader {
    ID3D11ShaderResourceView* LoadTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& filepath);
    void ReleaseTexture(ID3D11ShaderResourceView* texture);
}

