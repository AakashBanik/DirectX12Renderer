#include "stdafx.h"
#include "Shader.h"
#include <fstream>

dmp::Shader::Shader(const std::string & path) :
   mFilename(path)
{
   std::ifstream fin(mFilename, std::ios::binary);
   fin.seekg(0, std::ios_base::end);
   auto endPos = fin.tellg();
   fin.seekg(0, std::ios_base::beg);

   expectRes("Create shader blob",
             D3DCreateBlob(endPos, mShaderBlob.ReleaseAndGetAddressOf()));

   fin.read((char*) mShaderBlob->GetBufferPointer(), endPos);
   fin.close();

   mIsNullShader = false;
}

dmp::Shader::Shader(const Shader & rhs)
{
   mIsNullShader = rhs.mIsNullShader;
   mFilename = rhs.mFilename;
   mShaderBlob = rhs.mShaderBlob;
}

dmp::Shader & dmp::Shader::operator=(const Shader & rhs)
{
   auto tmp = rhs;
   std::swap(*this, tmp);
   return *this;
}

dmp::Shader::operator D3D12_SHADER_BYTECODE() const
{
   D3D12_SHADER_BYTECODE sbc;
   sbc.BytecodeLength = mShaderBlob->GetBufferSize();
   sbc.pShaderBytecode = reinterpret_cast<BYTE *>(mShaderBlob->GetBufferPointer());
   return sbc;
}
