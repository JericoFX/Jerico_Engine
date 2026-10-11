// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT

#include <Windows.h>
#include <d3dx12.h>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>

static_assert(sizeof(void*) == 8);
static_assert(std::is_base_of_v<D3D12_RESOURCE_DESC, CD3DX12_RESOURCE_DESC>);
static_assert(sizeof(D3D12_RESOURCE_DESC) == sizeof(CD3DX12_RESOURCE_DESC));
static_assert(sizeof(D3D12_TEXTURE_COPY_LOCATION) == sizeof(CD3DX12_TEXTURE_COPY_LOCATION));

namespace JE::LocalPackages::D3dx12
{
    bool Resources()
    {
        // Same descriptor APIs used by AsyncUploadQueue, Device and HeapAllocator.
        const CD3DX12_HEAP_PROPERTIES upload(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_HEAP_PROPERTIES defaults(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_HEAP_DESC heap(65536, D3D12_HEAP_TYPE_DEFAULT, 0, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
        const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(4096);
        const CD3DX12_RANGE mapped(0, 4096);
        const FLOAT color[] = { 0.25f, 0.5f, 0.75f, 1.0f };
        const CD3DX12_CLEAR_VALUE rgba(DXGI_FORMAT_R16G16B16A16_FLOAT, color);
        const CD3DX12_CLEAR_VALUE depth(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, 0.5f, 7);
        return upload.IsCPUAccessible() && !defaults.IsCPUAccessible()
            && upload.Type == D3D12_HEAP_TYPE_UPLOAD && upload.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_UNKNOWN
            && upload.MemoryPoolPreference == D3D12_MEMORY_POOL_UNKNOWN
            && upload.CreationNodeMask == 1 && upload.VisibleNodeMask == 1
            && heap.SizeInBytes == 65536 && heap.Properties == defaults && heap.Alignment == 0
            && heap.Flags == D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
            && buffer.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && buffer.Width == 4096
            && buffer.Height == 1 && buffer.DepthOrArraySize == 1 && buffer.MipLevels == 1
            && buffer.Format == DXGI_FORMAT_UNKNOWN && buffer.SampleDesc.Count == 1
            && buffer.Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR && buffer.Flags == D3D12_RESOURCE_FLAG_NONE
            && mapped.Begin == 0 && mapped.End == 4096
            && rgba.Format == DXGI_FORMAT_R16G16B16A16_FLOAT && std::memcmp(rgba.Color, color, sizeof(color)) == 0
            && depth.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT && depth.DepthStencil.Depth == 0.5f
            && depth.DepthStencil.Stencil == 7;
    }

    bool Copies()
    {
        // Build the copy descriptors used by CommandList/ImagePool without a GPU resource.
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{ 512, { DXGI_FORMAT_R8G8B8A8_UNORM, 8, 4, 1, 256 } };
        const CD3DX12_TEXTURE_COPY_LOCATION placed(nullptr, footprint);
        const CD3DX12_TEXTURE_COPY_LOCATION indexed(nullptr, 9);
        const CD3DX12_BOX box(1, 2, 3, 9, 6, 4);
        return placed.pResource == nullptr && placed.Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT
            && placed.PlacedFootprint.Offset == 512 && placed.PlacedFootprint.Footprint.Format == DXGI_FORMAT_R8G8B8A8_UNORM
            && placed.PlacedFootprint.Footprint.Width == 8 && placed.PlacedFootprint.Footprint.Height == 4
            && placed.PlacedFootprint.Footprint.Depth == 1 && placed.PlacedFootprint.Footprint.RowPitch == 256
            && indexed.pResource == nullptr && indexed.Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
            && indexed.SubresourceIndex == 9 && box.left == 1 && box.top == 2 && box.front == 3
            && box.right == 9 && box.bottom == 6 && box.back == 4;
    }

    bool Tiles()
    {
        // Image's tiled-resource coordinate/region constructors and subresource indexing.
        const CD3DX12_TILED_RESOURCE_COORDINATE coordinate(2, 3, 4, 5);
        const CD3DX12_TILE_REGION_SIZE boxed(24, TRUE, 2, 3, 4);
        const CD3DX12_TILE_REGION_SIZE packed(7, FALSE, 0, 0, 0);
        auto texture = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, 256, 128, 3, 4,
            1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        const UINT index = texture.CalcSubresource(2, 1, 1);
        UINT mip = 0;
        UINT array = 0;
        UINT plane = 0;
        D3D12DecomposeSubresource(index, 4, 3, mip, array, plane);
        return coordinate.X == 2 && coordinate.Y == 3 && coordinate.Z == 4 && coordinate.Subresource == 5
            && boxed.NumTiles == 24 && boxed.UseBox == TRUE && boxed.Width == 2 && boxed.Height == 3 && boxed.Depth == 4
            && packed.NumTiles == 7 && packed.UseBox == FALSE && packed.Width == 0 && packed.Height == 0 && packed.Depth == 0
            && texture.Depth() == 1 && texture.ArraySize() == 3 && texture.MipLevels == 4
            && index == 18 && mip == 2 && array == 1 && plane == 1;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 1;
    }
    const std::string test = argv[1];
    const bool passed = test == "resources" ? JE::LocalPackages::D3dx12::Resources()
        : test == "copies" ? JE::LocalPackages::D3dx12::Copies()
        : test == "tiles" && JE::LocalPackages::D3dx12::Tiles();
    std::cout << "CASE " << test << ": " << (passed ? "PASS" : "FAIL") << '\n';
    return passed ? 0 : 1;
}
