#include "d3d11_device.h"
#include "d3d11_texture.h"

namespace dxvk {
  
  /**
   * \brief Retrieves format mode from bind flags
   * 
   * Uses the bind flags to determine whether a resource
   * needs to be created with a color format or a depth
   * format, even if the DXGI format is typeless.
   * \param [in] BindFlags Image bind flags
   * \returns Format mode
   */
  static DxgiFormatMode GetFormatModeFromBindFlags(UINT BindFlags) {
    if (BindFlags & D3D11_BIND_RENDER_TARGET)
      return DxgiFormatMode::Color;
    
    if (BindFlags & D3D11_BIND_DEPTH_STENCIL)
      return DxgiFormatMode::Depth;
    
    return DxgiFormatMode::Any;
  }
  
  
  /**
   * \brief Fills in image info stage and access flags
   * 
   * \param [in] pDevice Target device
   * \param [in] BindFLags Resource bind flags
   * \param [in] CPUAccessFlags CPU access flags
   * \param [in] MiscFlags Additional usage info
   * \param [out] pImageInfo DXVK image create info
   */
  static void GetImageStagesAndAccessFlags(
    const D3D11Device*          pDevice,
          UINT                  BindFlags,
          UINT                  CPUAccessFlags,
          UINT                  MiscFlags,
          DxvkImageCreateInfo*  pImageInfo) {
    
    if (BindFlags & D3D11_BIND_SHADER_RESOURCE) {
      pImageInfo->usage  |= VK_IMAGE_USAGE_SAMPLED_BIT;
      pImageInfo->stages |= pDevice->GetEnabledShaderStages();
      pImageInfo->access |= VK_ACCESS_SHADER_READ_BIT;
    }
    
    if (BindFlags & D3D11_BIND_RENDER_TARGET) {
      pImageInfo->usage  |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      pImageInfo->stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      pImageInfo->access |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                         |  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    
    if (BindFlags & D3D11_BIND_DEPTH_STENCIL) {
      pImageInfo->usage  |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      pImageInfo->stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                         |  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      pImageInfo->access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                         |  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    
    if (BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
      pImageInfo->usage  |= VK_IMAGE_USAGE_STORAGE_BIT;
      pImageInfo->stages |= pDevice->GetEnabledShaderStages();
      pImageInfo->access |= VK_ACCESS_SHADER_READ_BIT
                         |  VK_ACCESS_SHADER_WRITE_BIT;
    }
    
    if (CPUAccessFlags != 0) {
      pImageInfo->tiling  = VK_IMAGE_TILING_LINEAR;
      pImageInfo->stages |= VK_PIPELINE_STAGE_HOST_BIT;
      
      if (CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
        pImageInfo->access |= VK_ACCESS_HOST_WRITE_BIT;
      
      if (CPUAccessFlags & D3D11_CPU_ACCESS_READ)
        pImageInfo->access |= VK_ACCESS_HOST_READ_BIT;
    }
    
    if (MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
      pImageInfo->flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    if (pImageInfo->mipLevels == 0)
      pImageInfo->mipLevels = util::computeMipLevelCount(pImageInfo->extent);
  }
  
  
  D3D11Texture1D::D3D11Texture1D(
          D3D11Device*                pDevice,
    const D3D11_TEXTURE1D_DESC*       pDesc)
  : m_device    (pDevice),
    m_formatMode(GetFormatModeFromBindFlags(pDesc->BindFlags)),
    m_desc      (*pDesc) {
    
    DxvkImageCreateInfo info;
    info.type           = VK_IMAGE_TYPE_1D;
    info.format         = pDevice->LookupFormat(pDesc->Format, m_formatMode).format;
    info.flags          = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    info.sampleCount    = VK_SAMPLE_COUNT_1_BIT;
    info.extent.width   = pDesc->Width;
    info.extent.height  = 1;
    info.extent.depth   = 1;
    info.numLayers      = pDesc->ArraySize;
    info.mipLevels      = pDesc->MipLevels;
    info.usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.stages         = VK_PIPELINE_STAGE_TRANSFER_BIT;
    info.access         = VK_ACCESS_TRANSFER_READ_BIT
                        | VK_ACCESS_TRANSFER_WRITE_BIT;
    info.tiling         = VK_IMAGE_TILING_OPTIMAL;
    info.layout         = VK_IMAGE_LAYOUT_GENERAL;
    
    GetImageStagesAndAccessFlags(
      pDevice,
      pDesc->BindFlags,
      pDesc->CPUAccessFlags,
      pDesc->MiscFlags,
      &info);
    
    m_image = pDevice->GetDXVKDevice()->createImage(
      info, GetMemoryFlagsForUsage(pDesc->Usage));
  }
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 1 D
  D3D11Texture1D::~D3D11Texture1D() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D11Texture1D::QueryInterface(REFIID riid, void** ppvObject) {
    COM_QUERY_IFACE(riid, ppvObject, IUnknown);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11DeviceChild);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11Resource);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11Texture1D);
    
    Logger::warn("D3D11Texture1D::QueryInterface: Unknown interface query");
    return E_NOINTERFACE;
  }
  
    
  void STDMETHODCALLTYPE D3D11Texture1D::GetDevice(ID3D11Device** ppDevice) {
    *ppDevice = m_device.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture1D::GetType(D3D11_RESOURCE_DIMENSION *pResourceDimension) {
    *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
  }
  
  
  UINT STDMETHODCALLTYPE D3D11Texture1D::GetEvictionPriority() {
    Logger::warn("D3D11Texture1D::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture1D::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D11Texture1D::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture1D::GetDesc(D3D11_TEXTURE1D_DESC *pDesc) {
    *pDesc = m_desc;
  }
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 2 D
  D3D11Texture2D::D3D11Texture2D(
          D3D11Device*                pDevice,
    const D3D11_TEXTURE2D_DESC*       pDesc)
  : m_device    (pDevice),
    m_formatMode(GetFormatModeFromBindFlags(pDesc->BindFlags)),
    m_desc      (*pDesc) {
    
    DxvkImageCreateInfo info;
    info.type           = VK_IMAGE_TYPE_2D;
    info.format         = pDevice->LookupFormat(pDesc->Format, m_formatMode).format;
    info.flags          = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    info.sampleCount    = VK_SAMPLE_COUNT_1_BIT;
    info.extent.width   = pDesc->Width;
    info.extent.height  = pDesc->Height;
    info.extent.depth   = 1;
    info.numLayers      = pDesc->ArraySize;
    info.mipLevels      = pDesc->MipLevels;
    info.usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.stages         = VK_PIPELINE_STAGE_TRANSFER_BIT;
    info.access         = VK_ACCESS_TRANSFER_READ_BIT
                        | VK_ACCESS_TRANSFER_WRITE_BIT;
    info.tiling         = VK_IMAGE_TILING_OPTIMAL;
    info.layout         = VK_IMAGE_LAYOUT_GENERAL;
    
    if (FAILED(GetSampleCount(pDesc->SampleDesc.Count, &info.sampleCount)))
      throw DxvkError(str::format("D3D11: Invalid sample count: ", pDesc->SampleDesc.Count));
    
    GetImageStagesAndAccessFlags(
      pDevice,
      pDesc->BindFlags,
      pDesc->CPUAccessFlags,
      pDesc->MiscFlags,
      &info);
    
    m_image = pDevice->GetDXVKDevice()->createImage(
      info, GetMemoryFlagsForUsage(pDesc->Usage));
  }
  
  
  D3D11Texture2D::~D3D11Texture2D() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D11Texture2D::QueryInterface(REFIID riid, void** ppvObject) {
    COM_QUERY_IFACE(riid, ppvObject, IUnknown);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11DeviceChild);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11Resource);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11Texture2D);
    
    Logger::warn("D3D11Texture2D::QueryInterface: Unknown interface query");
    return E_NOINTERFACE;
  }
  
    
  void STDMETHODCALLTYPE D3D11Texture2D::GetDevice(ID3D11Device** ppDevice) {
    *ppDevice = m_device.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture2D::GetType(D3D11_RESOURCE_DIMENSION *pResourceDimension) {
    *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
  }
  
  
  UINT STDMETHODCALLTYPE D3D11Texture2D::GetEvictionPriority() {
    Logger::warn("D3D11Texture2D::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture2D::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D11Texture2D::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture2D::GetDesc(D3D11_TEXTURE2D_DESC *pDesc) {
    *pDesc = m_desc;
  }
  
  
  ///////////////////////////////////////////
  //      D 3 D 1 1 T E X T U R E 2 D
  D3D11Texture3D::D3D11Texture3D(
          D3D11Device*                pDevice,
    const D3D11_TEXTURE3D_DESC*       pDesc)
  : m_device    (pDevice),
    m_formatMode(GetFormatModeFromBindFlags(pDesc->BindFlags)),
    m_desc      (*pDesc) {
    
    DxvkImageCreateInfo info;
    info.type           = VK_IMAGE_TYPE_3D;
    info.format         = pDevice->LookupFormat(pDesc->Format, m_formatMode).format;
    info.flags          = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
                        | VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
    info.sampleCount    = VK_SAMPLE_COUNT_1_BIT;
    info.extent.width   = pDesc->Width;
    info.extent.height  = pDesc->Height;
    info.extent.depth   = pDesc->Depth;
    info.numLayers      = 1;
    info.mipLevels      = pDesc->MipLevels;
    info.usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.stages         = VK_PIPELINE_STAGE_TRANSFER_BIT;
    info.access         = VK_ACCESS_TRANSFER_READ_BIT
                        | VK_ACCESS_TRANSFER_WRITE_BIT;
    info.tiling         = VK_IMAGE_TILING_OPTIMAL;
    info.layout         = VK_IMAGE_LAYOUT_GENERAL;
    
    GetImageStagesAndAccessFlags(
      pDevice,
      pDesc->BindFlags,
      pDesc->CPUAccessFlags,
      pDesc->MiscFlags,
      &info);
    
    m_image = pDevice->GetDXVKDevice()->createImage(
      info, GetMemoryFlagsForUsage(pDesc->Usage));
  }
  
  
  D3D11Texture3D::~D3D11Texture3D() {
    
  }
  
  
  HRESULT STDMETHODCALLTYPE D3D11Texture3D::QueryInterface(REFIID riid, void** ppvObject) {
    COM_QUERY_IFACE(riid, ppvObject, IUnknown);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11DeviceChild);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11Resource);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11Texture3D);
    
    Logger::warn("D3D11Texture3D::QueryInterface: Unknown interface query");
    return E_NOINTERFACE;
  }
  
    
  void STDMETHODCALLTYPE D3D11Texture3D::GetDevice(ID3D11Device** ppDevice) {
    *ppDevice = m_device.ref();
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture3D::GetType(D3D11_RESOURCE_DIMENSION *pResourceDimension) {
    *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
  }
  
  
  UINT STDMETHODCALLTYPE D3D11Texture3D::GetEvictionPriority() {
    Logger::warn("D3D11Texture3D::GetEvictionPriority: Stub");
    return DXGI_RESOURCE_PRIORITY_NORMAL;
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture3D::SetEvictionPriority(UINT EvictionPriority) {
    Logger::warn("D3D11Texture3D::SetEvictionPriority: Stub");
  }
  
  
  void STDMETHODCALLTYPE D3D11Texture3D::GetDesc(D3D11_TEXTURE3D_DESC *pDesc) {
    *pDesc = m_desc;
  }
  
  
  
  HRESULT GetCommonTextureInfo(
          ID3D11Resource*   pResource,
          D3D11TextureInfo* pTextureInfo) {
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dimension);
    
    switch (dimension) {
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D: {
        auto tex = static_cast<D3D11Texture1D*>(pResource);
        pTextureInfo->formatMode = tex->GetFormatMode();
        pTextureInfo->image      = tex->GetDXVKImage();
      } return S_OK;
      
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D: {
        auto tex = static_cast<D3D11Texture2D*>(pResource);
        pTextureInfo->formatMode = tex->GetFormatMode();
        pTextureInfo->image      = tex->GetDXVKImage();
      } return S_OK;
      
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D: {
        auto tex = static_cast<D3D11Texture3D*>(pResource);
        pTextureInfo->formatMode = tex->GetFormatMode();
        pTextureInfo->image      = tex->GetDXVKImage();
      } return S_OK;
      
      default:
        return E_INVALIDARG;
    }
  }
  
}
