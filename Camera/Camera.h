/**********************************************************************************
// Camera (Arquivo de Cabeçalho)
//
// Criação:		27 Abr 2016
// Atualização:	26 Set 2021
// Compilador:	Visual C++ 2019
//
// Descrição:	Controla a câmera em uma cena 3D
//
**********************************************************************************/

#ifndef CAMERA_H
#define CAMERA_H

#include "DXUT.h"
#include <D3DCompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <sstream>
#include <fstream>
#include "iostream"
#include <string>
#include <vector>
using namespace DirectX;
using namespace std;


// ------------------------------------------------------------------------------

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

// ------------------------------------------------------------------------------

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj =
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
};

// ------------------------------------------------------------------------------

class Camera : public App
{
private:
    ID3D12RootSignature* rootSignature = nullptr;
    ID3D12PipelineState* pipelineState = nullptr;
    Mesh* geometry = nullptr;
    ID3D12DescriptorHeap* constantBufferHeap = nullptr;
    ID3D12Resource* constantBufferUpload = nullptr;
    BYTE* constantBufferData = nullptr;

    Timer medidorTempo;
    bool spin = true;
    int contadorVertex = false;
    int contadorTexture = false;
    int contadorNormal = false;
    

    XMFLOAT4X4 World = {};
    XMFLOAT4X4 View = {};
    XMFLOAT4X4 Proj = {};

    float theta = 0;
    float phi = 0;
    float radius = 0;

    float lastMousePosX = 0;
    float lastMousePosY = 0;


    vector<ushort> listIndex;
    vector<Vertex> listVertex;
    bool azul = false;

public:
    void Init();
    void Update();
    void Draw();
    void Finalize();

    void BuildConstantBuffers();
    void BuildGeometry();
    void BuildRootSignature();
    void BuildPipelineState();
    void readObject();


};

// ------------------------------------------------------------------------------

#endif