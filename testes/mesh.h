#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"

struct Vertex{
    glm::vec3 vertices;
    glm::vec3 normals;
    glm::vec2 texCoords;
};

class Mesh{
private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indexes;
    std::vector<unsigned int> textures;
    unsigned int VAO;
    unsigned int EBO;
    unsigned int VBO;

public:
    void Mesh(std::vector<Vertex> vertices, 
              std::vector<unsigned int> indexes, std::vector<unsigned int> textures){
        this->vertices = vertices;
        this->textures = textures;
        this->indexes = indexes;

        setupMesh();
    }

    void setupMesh(){
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(VBO, vertices.size()*sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(EBO, indexes.size()*sizeof(unsigned int), indexes.data(), GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, 0, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, 0, sizeof(Vertex), (void*)3*float);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, 0, sizeof(Vertex), (void*)6*float);
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    void Draw(unsigned int shaderProgram){
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, vertices.size(), GL_UNSIGNED_INT, indexes.data());
    }

};

class Model{
private:
    std::vector<Mesh> meshes;
    std::string path;


public:
    void Model(std::string path){
        this->path = path;
    }

    void Draw(unsigned int shaderProgram){
        for(int i = 0: i < meshes.size(); i++){
            meshes[i].draw(shaderProgram);
        }
    }

    void importMeshes(){
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs); 

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
            cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));

        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        // process all the node's meshes (if any)
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
            meshes.push_back(processMesh(mesh, scene));			
        }
        // then do the same for each of its children
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }  

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indexes;
        std::vector<unsigned int> textures;

        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vec;
            vec.x = mesh->mVertices[i].x;
            vec.y = mesh->mVertices[i].y;
            vec.z = mesh->mVertices[i].z;
            vertex.vertices = vec;

            vec.x = mesh->mNormals[i].x;
            vec.y = mesh->mNormals[i].y;
            vec.z = mesh->mNormals[i].z;
            vertex.normals = vec;

            if(mesh->mTextureCoords[0]) {
                glm::vec2 tex;
                tex.x = mesh->mTextureCoords[0][i].x; 
                tex.y = mesh->mTextureCoords[0][i].y;
                vertex.texCoords = vec;
            }
            else
                vertex.texCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }        
        
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indexes.push_back(face.mIndices[j]);
        }  
       

        if(mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            vector<Texture> diffuseMaps = loadMaterialTextures(material, 
                                                aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            vector<Texture> specularMaps = loadMaterialTextures(material, 
                                                aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        return Mesh(vertices, indices, textures);
    } 
};