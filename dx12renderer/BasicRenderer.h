#pragma once

#include "BaseRenderer.h"
#include "BasicTypes.h"
#include "Mesh.h"
#include "FrameResource.h"
#include "Timer.h"
#include "Shader.h"

namespace dmp
{
   class BasicRenderer : public BaseRenderer<Timer>
   {
   public:
      BasicRenderer(HWND windowHandle, int width, int height) :
         BaseRenderer(windowHandle, width, height)
      {
         init();
      }

      BasicRenderer();
      ~BasicRenderer();
   private:
      HRESULT drawImpl();
      HRESULT drawRItems();
      bool initImpl();

      bool buildRootSignature();
      Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;

      bool buildShadersAndInputLayouts();
      std::unordered_map<std::string, Shader> mShaders;
      const std::string vertexShaderFile = "BasicVertexShader.cso";
      const std::string pixelShaderFile = "BasicPixelShader.cso";
      std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
      
      bool buildMeshBuffer();
      std::unique_ptr<MeshBuffer<BasicVertex, uint16_t>> mMeshBuffer = nullptr;

      bool buildRenderItems();
      std::vector<std::unique_ptr<BasicRenderItem>> mRItems;
      std::vector<BasicMaterial> mMats;

      bool buildFrameResources();
      std::vector<std::unique_ptr<FrameResource<BasicPassConstants, BasicMaterial, BasicObjectConstants>>> mFrameResources;

      bool buildCBVDescriptorHeaps();
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCBVHeap = nullptr;
      UINT mMatsCBVOffset = 0;
      UINT mPassCBVOffset = 0;

      bool buildPSOs();
      std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
      const std::string opaqueKey = "opaque";
      const std::string wireframeKey = "wireframe";

      FrameResource<BasicPassConstants, BasicMaterial, BasicObjectConstants> * mCurrFrameResource = nullptr;
      int mCurrFrameResourceIndex = 0;

      bool updateImpl(const Timer & t) override;
      DirectX::XMFLOAT4 mClearColor;

      DirectX::SimpleMath::Matrix mV;
   };

}