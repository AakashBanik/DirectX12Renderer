#include "stdafx.h"
#include "BasicRenderer.h"
#include "BaseRenderer.h"
#include "UploadBuffer.h"
#include "Mesh.h"
#include "BasicModel.h"
#include "CommittedVertexBuffer.h"

dmp::BasicRenderer::BasicRenderer()
{}


dmp::BasicRenderer::~BasicRenderer()
{}

HRESULT dmp::BasicRenderer::drawImpl()
{
   using namespace DirectX;
   auto allocator = mCurrFrameResource->allocator;

   expectRes("Reset the frame resource's allocator",
             allocator->Reset());

   // hardcoding PSO for now
   expectRes("Reset the command list",
             mCommandList->Reset(allocator.Get(), mPSOs[opaqueKey].Get()));

   mCommandList->RSSetViewports(1, &mViewport);
   mCommandList->RSSetScissorRects(1, &mScissorRect);

   mCommandList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(),
                                                                       D3D12_RESOURCE_STATE_PRESENT,
                                                                       D3D12_RESOURCE_STATE_RENDER_TARGET));

   XMVECTORF32 clearColor = 
   {
      mClearColor.x * 0.5f, 
      mClearColor.y * 0.5f, 
      mClearColor.z * 0.5f, 
      mClearColor.w
   };

   mCommandList->ClearRenderTargetView(currentBackBufferView(),
                                       clearColor,
                                       0,
                                       nullptr);

   mCommandList->ClearDepthStencilView(depthStencilView(),
                                       D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                       1.0f, 0, 0, nullptr);

   mCommandList->OMSetRenderTargets(1, &currentBackBufferView(),
                                    true,
                                    &depthStencilView());

   ID3D12DescriptorHeap * dh[] = {mCBVHeap.Get()};
   mCommandList->SetDescriptorHeaps(_countof(dh), dh);

   mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

   mCurrFrameResource->passCB->bind(mCommandList.Get(),
                                    mCBVHeap.Get(),
                                    2,
                                    0);

   drawRItems();

   mCommandList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(),
                                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                       D3D12_RESOURCE_STATE_PRESENT));

   expectRes("Close the command list, done with this pass",
             mCommandList->Close());

   ID3D12CommandList * clist[] = {mCommandList.Get()};
   mCommandQueue->ExecuteCommandLists(_countof(clist), clist);

   expectRes("Swap buffers",
             mSwapChain->Present(0, 0));
   mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

   mCurrFrameResource->fenceVal = ++mFenceVal;
   mCommandQueue->Signal(mFence.Get(), mFenceVal);

   return S_OK;

}

HRESULT dmp::BasicRenderer::drawRItems()
{
   UINT objSize = (UINT)calcConstantBufferByteSize(sizeof(BasicObjectConstants));
   int objectCount = (int)mRItems.size();
   int matCount = (int) mMats.size();
   auto clist = mCommandList.Get();

   int i = 0;
   for (auto & curr : mRItems)
   {
      //clist->IASetVertexBuffers(0, 1, &curr->meshBuffer->vertexBufferView());
      VertexBindInfo vbi;
      vbi.commandList = clist;
      vbi.startSlot = 0;
      curr->meshBuffer->vertexBuffer()->bind(vbi);

      //clist->IASetIndexBuffer(&curr->meshBuffer->indexBufferView());
      curr->meshBuffer->indexBuffer()->bind({clist});
      clist->IASetPrimitiveTopology(curr->primitiveType);

      mCurrFrameResource->objectCB->bind(clist,
                                         mCBVHeap.Get(),
                                         0,
                                         i);

      mCurrFrameResource->materialCB->bind(clist,
                                           mCBVHeap.Get(),
                                           1,
                                           (UINT)curr->matIndex);

      clist->DrawIndexedInstanced(curr->indexCount,
                                  1,
                                  curr->startIndexLocation,
                                  curr->baseVertexLocation,
                                  0);
      ++i;
   }

   return S_OK;
}

bool dmp::BasicRenderer::initImpl()
{
   using namespace DirectX::SimpleMath;
   expectRes("Reset Command List",
             mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

   expectTrue("Build root signature", buildRootSignature());
   expectTrue("Load Shaders and build Input Layouts", buildShadersAndInputLayouts());
   expectTrue("Build mesh buffer and upload", buildMeshBuffer());
   expectTrue("Build render item", buildRenderItems());
   expectTrue("Build CBV descriptor heap", buildCBVDescriptorHeaps());
   expectTrue("Build frame resources", buildFrameResources());
   expectTrue("Build PSOs", buildPSOs());
   
   Vector3 eye(0.0f, 0.0f, 5.0f);
   Vector3 target(0.0f, 0.0f, 0.0f);
   Vector3 up(0.0f, 1.0f, 0.0f);

   mV = Matrix::CreateLookAt(eye, target, up);

   // actuall do all of that stuff ^
   expectRes("Close the command list", mCommandList->Close());
   ID3D12CommandList * lists[] = {mCommandList.Get()};
   mCommandQueue->ExecuteCommandLists(_countof(lists), lists);

   flushCommandQueue();

   return true;
}

bool dmp::BasicRenderer::buildRootSignature()
{
   using Microsoft::WRL::ComPtr;

   CD3DX12_DESCRIPTOR_RANGE objectCB;
   objectCB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

   CD3DX12_DESCRIPTOR_RANGE materialCB;
   materialCB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

   CD3DX12_DESCRIPTOR_RANGE passCB;
   passCB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

   CD3DX12_ROOT_PARAMETER params[3];

   params[0].InitAsDescriptorTable(1, &objectCB);
   params[1].InitAsDescriptorTable(1, &materialCB);
   params[2].InitAsDescriptorTable(1, &passCB);

   // TODO: investigate CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC that is not defined in your copy of
   // cd3dx12.h
   CD3DX12_ROOT_SIGNATURE_DESC rsd(3,
                                   params,
                                   0,
                                   nullptr,
                                   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

   ComPtr<ID3DBlob> srs = nullptr;
   ComPtr<ID3DBlob> error = nullptr;

   //TODO: "This function has been superceded by D3D12SerializeVersionedRootSignature."

   auto res = D3D12SerializeRootSignature(&rsd,
                                          D3D_ROOT_SIGNATURE_VERSION_1,
                                          srs.GetAddressOf(),
                                          error.GetAddressOf());

   if (error != nullptr)
   {
      ::OutputDebugStringA((char*) error->GetBufferPointer());
   }

   expectRes("Serialize root signature", res);

   expectRes("Create Root signature",
             mDevice->CreateRootSignature(0,
                                          srs->GetBufferPointer(),
                                          srs->GetBufferSize(),
                                          IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf())));

   return true;
}

bool dmp::BasicRenderer::buildShadersAndInputLayouts()
{
   mShaders.insert(std::make_pair(vertexShaderFile, Shader(vertexShaderFile)));
   mShaders.insert(std::make_pair(pixelShaderFile, Shader(pixelShaderFile)));

   mInputLayout =
   {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
      offsetof(BasicVertex, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
      offsetof(BasicVertex, normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
   };

   return true;
}

bool dmp::BasicRenderer::buildMeshBuffer()
{   
   // TODO: barf; magic constants
   std::vector<MeshData<BasicVertex>> models;

   loadModel("model//Voyager.lwo", models, mMats);

   mMeshBuffer = std::make_unique<MeshBuffer<BasicVertex, uint16_t>>(models, mDevice.Get(), mCommandList.Get());

   return true;
}

bool dmp::BasicRenderer::buildRenderItems()
{
   UINT index = 0;
   for (auto & curr : mMeshBuffer->mSubMeshes)
   {
      auto vger = std::make_unique<BasicRenderItem>();
      vger->meshBuffer = mMeshBuffer.get();
      vger->cbIndex = index;
      vger->indexCount = (UINT) curr.indexCount;
      vger->startIndexLocation = (UINT) curr.startIndexOffset;
      vger->baseVertexLocation = (UINT) curr.startVertexOffset;
      vger->matIndex = curr.matIndex;

      mRItems.push_back(std::move(vger));
      ++index;
   }

   std::sort(mRItems.begin(), mRItems.end(),
             [](std::unique_ptr<BasicRenderItem> & lhs, std::unique_ptr<BasicRenderItem> & rhs)
   {
      return lhs->matIndex < rhs->matIndex;
   });

   return true;
}

bool dmp::BasicRenderer::buildFrameResources()
{
   expectTrue("|mMats| not 0", mMats.size() != 0);
   expectTrue("|mRItems| not 0", mRItems.size() != 0);

   for (UINT i = 0; i < FRAME_RESOURCES_COUNT; ++i)
   {
      mFrameResources.push_back(std::make_unique<FrameResource<BasicPassConstants, 
                                BasicMaterial, 
                                BasicObjectConstants>>(mDevice.Get(),
                                                       mCBVHeap.Get(),
                                                       mCbvSrvUavDescriptorSize,
                                                       (UINT)1,
                                                       (UINT)mPassCBVOffset + i,
                                                       (UINT)mMats.size(),
                                                       mMatsCBVOffset + (i * (UINT)mMats.size()),
                                                       (UINT)mRItems.size(),
                                                       (i * (UINT)mRItems.size())));

   }

   return true;
}

bool dmp::BasicRenderer::buildCBVDescriptorHeaps()
{
   UINT objCount = (UINT)mRItems.size();
   UINT matCount = (UINT)mMats.size();

   // one descriptor per object for object constants
   // one descriptor per material for material constants
   // 1 extra descriptor for pass constants
   // times 3 since we need this per frame resource
   UINT descCount = (matCount + objCount + 1) * FRAME_RESOURCES_COUNT;

   mMatsCBVOffset = objCount * FRAME_RESOURCES_COUNT;
   mPassCBVOffset = (objCount + matCount) * FRAME_RESOURCES_COUNT;

   D3D12_DESCRIPTOR_HEAP_DESC dhd;
   dhd.NumDescriptors = descCount;
   dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   dhd.NodeMask = 0;

   expectRes("Create CBV descriptor heaps",
             mDevice->CreateDescriptorHeap(&dhd,
                                           IID_PPV_ARGS(mCBVHeap.ReleaseAndGetAddressOf())));

   return true;
}

bool dmp::BasicRenderer::buildPSOs()
{
   D3D12_GRAPHICS_PIPELINE_STATE_DESC psd = {};
   psd.InputLayout = {mInputLayout.data(), (UINT) mInputLayout.size()};
   psd.pRootSignature = mRootSignature.Get();
   psd.VS = mShaders[vertexShaderFile];
   psd.PS = mShaders[pixelShaderFile];

   psd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   psd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
   psd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   psd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
   psd.SampleMask = UINT_MAX; // don't feel like figuring out exactly what this is in std::limits
   psd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   psd.NumRenderTargets = 1;
   psd.RTVFormats[0] = mBackBufferFormat;
   psd.SampleDesc.Count = mMsaaEnabled ? 4 : 1;
   psd.SampleDesc.Quality = mMsaaEnabled ? (mMsaaQualityLevel - 1) : 0;
   psd.DSVFormat = mDepthStencilFormat;

   expectRes("Create opaque PSO",
             mDevice->CreateGraphicsPipelineState(&psd,
                                                  IID_PPV_ARGS(mPSOs[opaqueKey].ReleaseAndGetAddressOf())));

   // might as well make a wireframe PSO too

   auto wfPsd = psd;
   wfPsd.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
   expectRes("Create wireframe PSO",
             mDevice->CreateGraphicsPipelineState(&wfPsd,
                                                  IID_PPV_ARGS(mPSOs[wireframeKey].ReleaseAndGetAddressOf())));

   return true;
}

bool dmp::BasicRenderer::updateImpl(const Timer & t)
{
   using namespace DirectX;
   using namespace DirectX::SimpleMath;

   mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % FRAME_RESOURCES_COUNT;
   mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

   if (mCurrFrameResource->fenceVal != 0 && mFence->GetCompletedValue() < mCurrFrameResource->fenceVal)
   {
      auto handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
      expectRes("Set event to allow GPU to catch up to CPU",
                mFence->SetEventOnCompletion(mCurrFrameResource->fenceVal, handle));
      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
   }


   // Clear color

   auto time = t.time();

   float lerpColor = std::fmod(time * 0.08f, 1.0f);
   float theta = std::fmod(time /** 0.08f*/, XM_2PI);
   float rotTheta = std::fmod(time, XM_2PI);

   rotTheta = rotTheta - (XM_PI * 0.5f);
   rotTheta = rotTheta > (XM_PI * 0.5f) ? XM_PI - rotTheta : rotTheta;

   XMVECTOR c1;
   XMVECTOR c2;

   if (lerpColor < 0.33f)
   {
      c1 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
      c2 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
      XMStoreFloat4(&mClearColor, XMVectorLerp(c1, c2, lerpColor * 3.0f));
   }
   else if (lerpColor < 0.66f)
   {
      c1 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
      c2 = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
      XMStoreFloat4(&mClearColor, XMVectorLerp(c1, c2, (lerpColor - 0.33f) * 3.0f));
   }
   else
   {
      c1 = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
      c2 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
      XMStoreFloat4(&mClearColor, XMVectorLerp(c1, c2, (lerpColor - 0.66f) * 3.0f));
   }
   
   // Pass Constants

   BasicPassConstants pc;

   pc.V = mV.Transpose();
   pc.inverseV = mV.Invert().Transpose();
   pc.P = mP.Transpose();
   pc.inverseP = mP.Invert().Transpose();
   pc.PV = (mV * mP).Transpose();
   pc.inversePV = (mV * mP).Invert().Transpose();
   pc.eye = Vector4(0.0f, 0.0f, 5.0f, 1.0f);
   pc.nearZ = 1.0f;
   pc.farZ = 1000.0f;
   pc.totalTime = t.time();
   pc.deltaTime = t.deltaTime();
   pc.light.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
   pc.light.dir = Vector4(1.0f, 0.0f, 0.0f, 0.0f);

   mCurrFrameResource->passCB->update(0, pc);

   for (auto & curr : mRItems)
   {
      curr->M = Matrix::CreateRotationY(theta)
         * Matrix::CreateRotationZ(-theta)
         * Matrix::CreateRotationX(theta)
         * Matrix::CreateTranslation(Vector3(sin(theta), cos(theta), 0.0f));

      curr->framesDirty = FRAME_RESOURCES_COUNT;

      auto & objCB = mCurrFrameResource->objectCB;
      auto & matCB = mCurrFrameResource->materialCB;

      if (curr->framesDirty > 0) // which it will be
      {
         // TODO: XMStoreFloat4x4 didn't invalidate M did it? That would suck...

         BasicObjectConstants oc;
         oc.M = curr->M.Transpose();

         objCB->update(curr->cbIndex, oc);

         curr->framesDirty--;

         BasicMaterial mc;
         mc.ambient = mMats[curr->matIndex].ambient;
         mc.diffuse = mMats[curr->matIndex].diffuse;
         mc.specular = mMats[curr->matIndex].specular;
         mc.shininess = mMats[curr->matIndex].shininess;

         matCB->update((UINT)curr->matIndex, mc);
      }
   }

   return true;
}