//
// Created by johnk on 2022/2/25.
//

#include <RHI/Common.h>
#include <RHI/DirectX12/Common.h>
#include <RHI/DirectX12/Device.h>
#include <RHI/DirectX12/Texture.h>
#include <RHI/DirectX12/TextureView.h>

namespace RHI::DirectX12 {
    static bool IsShaderResource(const TextureViewType type)
    {
        return (type == TextureViewType::textureBinding);
    }

    static bool IsUnorderedAccess(const TextureViewType type)
    {
        return (type == TextureViewType::storageBinding);
    }

    static bool IsRenderTarget(const TextureViewType type)
    {
        return (type == TextureViewType::colorAttachment);
    }

    static bool IsDepthStencil(const TextureViewType type)
    {
        return (type == TextureViewType::depthStencil);
    }

    static void FillTexture1DSRV(D3D12_TEX1D_SRV& srv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv1D) {
            return;
        }
        srv.MostDetailedMip = createInfo.baseMipLevel;
        srv.MipLevels = createInfo.mipLevelNum;
        srv.ResourceMinLODClamp = static_cast<float>(createInfo.baseMipLevel);
    }

    static void FillTexture2DSRV(D3D12_TEX2D_SRV& srv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2D) {
            return;
        }
        srv.MostDetailedMip = createInfo.baseMipLevel;
        srv.MipLevels = createInfo.mipLevelNum;
        srv.PlaneSlice = 0;
        srv.ResourceMinLODClamp = static_cast<float>(createInfo.baseMipLevel);
    }

    static void FillTexture2DArraySRV(D3D12_TEX2D_ARRAY_SRV& srv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2DArray) {
            return;
        }
        srv.MostDetailedMip = createInfo.baseMipLevel;
        srv.MipLevels = createInfo.mipLevelNum;
        srv.FirstArraySlice = createInfo.baseArrayLayer;
        srv.ArraySize = createInfo.arrayLayerNum;
        srv.PlaneSlice = 0;
        srv.ResourceMinLODClamp = static_cast<float>(createInfo.baseMipLevel);
    }

    static void FillTextureCubeSRV(D3D12_TEXCUBE_SRV& srv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tvCube) {
            return;
        }
        srv.MostDetailedMip = createInfo.baseMipLevel;
        srv.MipLevels = createInfo.mipLevelNum;
        srv.ResourceMinLODClamp = static_cast<float>(createInfo.baseMipLevel);
    }

    static void FillTextureCubeArraySRV(D3D12_TEXCUBE_ARRAY_SRV& srv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tvCubeArray) {
            return;
        }
        srv.MostDetailedMip = createInfo.baseMipLevel;
        srv.MipLevels = createInfo.mipLevelNum;
        srv.First2DArrayFace = createInfo.baseArrayLayer;
        srv.NumCubes = createInfo.arrayLayerNum;
        srv.ResourceMinLODClamp = static_cast<float>(createInfo.baseMipLevel);
    }

    static void FillTexture3DSRV(D3D12_TEX3D_SRV& srv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv3D) {
            return;
        }
        srv.MostDetailedMip = createInfo.baseMipLevel;
        srv.MipLevels = createInfo.mipLevelNum;
        srv.ResourceMinLODClamp = static_cast<float>(createInfo.baseMipLevel);
    }

    static void FillTexture1DUAV(D3D12_TEX1D_UAV& uav, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv1D) {
            return;
        }
        uav.MipSlice = createInfo.baseMipLevel;
    }

    static void FillTexture2DUAV(D3D12_TEX2D_UAV& uav, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2D) {
            return;
        }
        uav.MipSlice = createInfo.baseMipLevel;
        uav.PlaneSlice = 0;
    }

    static void FillTexture2DArrayUAV(D3D12_TEX2D_ARRAY_UAV& uav, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2DArray) {
            return;
        }
        uav.MipSlice = createInfo.baseMipLevel;
        uav.FirstArraySlice = createInfo.baseArrayLayer;
        uav.ArraySize = createInfo.arrayLayerNum;
        uav.PlaneSlice = 0;
    }

    static void FillTexture3DUAV(D3D12_TEX3D_UAV& uav, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv3D) {
            return;
        }
        uav.MipSlice = createInfo.baseMipLevel;
        uav.FirstWSlice = createInfo.baseArrayLayer;
        uav.WSize = createInfo.arrayLayerNum;
    }

    static void FillTexture1DRTV(D3D12_TEX1D_RTV& rtv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv1D) {
            return;
        }
        rtv.MipSlice = createInfo.baseMipLevel;
    }

    static void FillTexture2DRTV(D3D12_TEX2D_RTV& rtv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2D) {
            return;
        }
        rtv.MipSlice = createInfo.baseMipLevel;
        rtv.PlaneSlice = 0;
    }

    static void FillTexture2DArrayRTV(D3D12_TEX2D_ARRAY_RTV& rtv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2DArray) {
            return;
        }
        rtv.MipSlice = createInfo.baseMipLevel;
        rtv.FirstArraySlice = createInfo.baseArrayLayer;
        rtv.ArraySize = createInfo.arrayLayerNum;
        rtv.PlaneSlice = 0;
    }

    static void FillTexture3DRTV(D3D12_TEX3D_RTV& rtv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv3D) {
            return;
        }
        rtv.MipSlice = createInfo.baseMipLevel;
        rtv.FirstWSlice = createInfo.baseArrayLayer;
        rtv.WSize = createInfo.arrayLayerNum;
    }

    static void FillTexture1DDSV(D3D12_TEX1D_DSV& dsv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv1D) {
            return;
        }
        dsv.MipSlice = createInfo.baseMipLevel;
    }

    static void FillTexture2DDSV(D3D12_TEX2D_DSV& dsv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2D) {
            return;
        }
        dsv.MipSlice = createInfo.baseMipLevel;
    }

    static void FillTexture2DArrayDSV(D3D12_TEX2D_ARRAY_DSV& dsv, const TextureViewCreateInfo& createInfo)
    {
        if (createInfo.dimension != TextureViewDimension::tv2DArray) {
            return;
        }
        dsv.MipSlice = createInfo.baseMipLevel;
        dsv.FirstArraySlice = createInfo.baseArrayLayer;
        dsv.ArraySize = createInfo.arrayLayerNum;
    }
}

namespace RHI::DirectX12 {
    DX12TextureView::DX12TextureView(DX12Device& inDevice, DX12Texture& inTexture, const TextureViewCreateInfo& inCreateInfo)
        : TextureView(inCreateInfo)
        , texture(inTexture)
        , descriptorAllocation()
    {
        CreateNativeDescriptor(inDevice, inCreateInfo);
    }

    DX12TextureView::~DX12TextureView() = default;

    CD3DX12_CPU_DESCRIPTOR_HANDLE DX12TextureView::GetNativeCpuDescriptorHandle() const
    {
        return descriptorAllocation->GetCpuHandle();
    }

    void DX12TextureView::CreateNativeDescriptor(DX12Device& inDevice, const TextureViewCreateInfo& inCreateInfo)
    {
        if (IsShaderResource(inCreateInfo.type)) {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc {};
            desc.Format = EnumCast<PixelFormat, DXGI_FORMAT>(texture.GetCreateInfo().format);
            desc.ViewDimension = EnumCast<TextureViewDimension, D3D12_SRV_DIMENSION>(inCreateInfo.dimension);
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            FillTexture1DSRV(desc.Texture1D, inCreateInfo);
            FillTexture2DSRV(desc.Texture2D, inCreateInfo);
            FillTexture2DArraySRV(desc.Texture2DArray, inCreateInfo);
            FillTextureCubeSRV(desc.TextureCube, inCreateInfo);
            FillTextureCubeArraySRV(desc.TextureCubeArray, inCreateInfo);
            FillTexture3DSRV(desc.Texture3D, inCreateInfo);

            descriptorAllocation = inDevice.AllocateCbvSrvUavDescriptor();
            inDevice.GetNative()->CreateShaderResourceView(texture.GetNative(), &desc, descriptorAllocation->GetCpuHandle());
        } else if (IsUnorderedAccess(inCreateInfo.type)) {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc {};
            desc.Format = EnumCast<PixelFormat, DXGI_FORMAT>(texture.GetCreateInfo().format);
            desc.ViewDimension = EnumCast<TextureViewDimension, D3D12_UAV_DIMENSION>(inCreateInfo.dimension);
            FillTexture1DUAV(desc.Texture1D, inCreateInfo);
            FillTexture2DUAV(desc.Texture2D, inCreateInfo);
            FillTexture2DArrayUAV(desc.Texture2DArray, inCreateInfo);
            FillTexture3DUAV(desc.Texture3D, inCreateInfo);

            descriptorAllocation = inDevice.AllocateCbvSrvUavDescriptor();
            inDevice.GetNative()->CreateUnorderedAccessView(texture.GetNative(), nullptr, &desc, descriptorAllocation->GetCpuHandle());
        } else if (IsRenderTarget(inCreateInfo.type)) {
            D3D12_RENDER_TARGET_VIEW_DESC desc {};
            desc.Format = EnumCast<PixelFormat, DXGI_FORMAT>(texture.GetCreateInfo().format);
            desc.ViewDimension = EnumCast<TextureViewDimension, D3D12_RTV_DIMENSION>(inCreateInfo.dimension);
            FillTexture1DRTV(desc.Texture1D, inCreateInfo);
            FillTexture2DRTV(desc.Texture2D, inCreateInfo);
            FillTexture2DArrayRTV(desc.Texture2DArray, inCreateInfo);
            FillTexture3DRTV(desc.Texture3D, inCreateInfo);

            descriptorAllocation = inDevice.AllocateRtvDescriptor();
            inDevice.GetNative()->CreateRenderTargetView(texture.GetNative(), &desc, descriptorAllocation->GetCpuHandle());
        } else if (IsDepthStencil(inCreateInfo.type)) {
            D3D12_DEPTH_STENCIL_VIEW_DESC desc {};
            desc.Format = EnumCast<PixelFormat, DXGI_FORMAT>(texture.GetCreateInfo().format);
            desc.ViewDimension = EnumCast<TextureViewDimension, D3D12_DSV_DIMENSION>(inCreateInfo.dimension);
            FillTexture1DDSV(desc.Texture1D, inCreateInfo);
            FillTexture2DDSV(desc.Texture2D, inCreateInfo);
            FillTexture2DArrayDSV(desc.Texture2DArray, inCreateInfo);

            descriptorAllocation = inDevice.AllocateDsvDescriptor();
            inDevice.GetNative()->CreateDepthStencilView(texture.GetNative(), &desc, descriptorAllocation->GetCpuHandle());
        }
    }
}
