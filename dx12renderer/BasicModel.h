#pragma once



namespace dmp
{
   template <typename VertexT> class MeshData;
   struct BasicVertex;

   void loadModel(const std::string & filename, 
                  std::vector<MeshData<BasicVertex>> & models,
                  std::vector<BasicMaterial> & mats);
}