#include "stdafx.h"
#include "Mesh.h"
#include "BasicTypes.h"
#include "BasicModel.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace dmp;
using namespace Assimp;

static DirectX::SimpleMath::Vector4 colorToVec4(const aiColor3D & color)
{
   return DirectX::SimpleMath::Vector4(color.r, color.g, color.b, 1.0f);
}

static void processMesh(std::vector<MeshData<BasicVertex>> & models, 
                        std::vector<BasicMaterial> & mats,
                        aiMesh * mesh,
                        const aiScene * scene)
{
   using namespace DirectX::SimpleMath;

   // material

   auto material = scene->mMaterials[mesh->mMaterialIndex];

   
   aiColor3D diffuse;
   material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);

   aiColor3D ambient = aiColor3D(diffuse.r * 0.2f, diffuse.g * 0.2f, diffuse.b * 0.2f);
   //material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);

   aiColor3D specular = aiColor3D(std::min(diffuse.r * 2.0f, 1.0f), 
                                  std::min(diffuse.g * 2.0f, 1.0f), 
                                  std::min(diffuse.b * 2.0f, 1.0f));
   //material->Get(AI_MATKEY_COLOR_SPECULAR, specular);

   float shininess = 0.5f;
   //material->Get(AI_MATKEY_SHININESS, shininess);

   int shadingType;
   material->Get(AI_MATKEY_SHADING_MODEL, shadingType);
   // TODO: handle the fact that models don't all use phong shading

   auto color = mesh->mColors;

   BasicMaterial mat;
   mat.ambient = colorToVec4(ambient);
   mat.diffuse = colorToVec4(diffuse);
   mat.specular = colorToVec4(specular);
   mat.shininess = shininess;

   if (std::find(mats.begin(), mats.end(), mat) == mats.end())
   {
      mats.push_back(mat);
   }

   size_t matIndex = 0;
   bool matFound = false;

   for (matIndex = 0; matIndex < mats.size(); ++matIndex)
   {
      if (mats[matIndex] == mat)
      {
         matFound = true;
         break;
      }
   }

   expectTrue("found the material in the memo", matFound);

   // model

   std::vector<BasicVertex> verts(0);
   std::vector<uint16_t> idxs(0);

   for (size_t i = 0; i < mesh->mNumVertices; ++i)
   {
      BasicVertex v;

      v.pos = Vector4(mesh->mVertices[i].x,
                      mesh->mVertices[i].y,
                      mesh->mVertices[i].z,
                      1.0f);
      v.normal = Vector4(mesh->mNormals[i].x,
                        mesh->mNormals[i].y,
                        mesh->mNormals[i].z,
                        1.0f);
      verts.push_back(v);
   }

   for (size_t i = 0; i < mesh->mNumFaces; ++i)
   {
      auto face = mesh->mFaces[i];
      if (face.mNumIndices != 3) continue;
      for (size_t j = 0; j < face.mNumIndices; ++j)
      {
         idxs.push_back(face.mIndices[j]);
      }
   }

   MeshData<BasicVertex> md(verts, idxs, matIndex);

   models.push_back(md);
}

static void processNode(std::vector<MeshData<BasicVertex>> & models, 
                        std::vector<BasicMaterial> & mats,
                        aiNode * node, 
                        const aiScene * scene)
{
   for (size_t i = 0; i < node->mNumMeshes; ++i)
   {
      aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
      processMesh(models, mats, mesh, scene);
   }

   for (size_t i = 0; i < node->mNumChildren; ++i)
   {
      processNode(models, mats, node->mChildren[i], scene);
   }
}

void dmp::loadModel(const std::string & filename,
                    std::vector<MeshData<BasicVertex>> & models,
                    std::vector<BasicMaterial> & mats)
{
   Importer imp;
   imp.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);

   // TODO: This will probably be stupidly slow or something
   auto scene = imp.ReadFile(filename,
                             aiProcess_JoinIdenticalVertices
                             | aiProcess_Triangulate
                             | aiProcess_GenSmoothNormals
                             | aiProcess_PreTransformVertices
                             | aiProcess_ValidateDataStructure
                             | aiProcess_ImproveCacheLocality
                             | aiProcess_RemoveRedundantMaterials
                             | aiProcess_FindInvalidData
                             | aiProcess_GenUVCoords
                             | aiProcess_TransformUVCoords
                             | aiProcess_FindInstances
                             | aiProcess_OptimizeMeshes
                             | aiProcess_OptimizeGraph
                             | aiProcess_Debone);

   expectTrue("model read succeeded",
              !(!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode));
   
   std::vector<MeshData<BasicVertex>> retval;

   processNode(models, mats, scene->mRootNode, scene);
}
