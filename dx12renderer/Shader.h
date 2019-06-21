#pragma once

namespace dmp
{
   class Shader
   {
   public:
      Shader() {}
      Shader(const std::string & path);
      Shader(const Shader & rhs);
      Shader & operator=(const Shader & rhs);
      ~Shader() {}

      operator bool() const { return !mIsNullShader; }

      operator D3D12_SHADER_BYTECODE() const;

      operator std::string() const { return mFilename; }

   private:
      bool mIsNullShader = true;
      std::string mFilename = "";
      Microsoft::WRL::ComPtr<ID3DBlob> mShaderBlob = nullptr;
   };
}