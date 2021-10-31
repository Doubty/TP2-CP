/**********************************************************************************
// Camera (C�digo Fonte)
//
// Cria��o:		27 Abr 2016
// Atualiza��o:	26 Set 2021
// Compilador:	Visual C++ 2019
//
// Descri��o:	Controla a c�mera em uma cena 3D
//
**********************************************************************************/

#include "Camera.h"

// ------------------------------------------------------------------------------




void Camera::Init()
{
	medidorTempo.Start();
	spin = true;
	listIndex = {};
	listVertex = {};
	contadorNormal = 0;
	contadorVertex = 0;
	contadorTexture = 0;
	readObject();

	theta = XM_PIDIV4;
	phi = XM_PIDIV4;
	radius = 5.0f;

	// pega �ltima posi��o do mouse
	lastMousePosX = (float)input->MouseX();
	lastMousePosY = (float)input->MouseY();

	// inicializa as matrizes World e View para a identidade
	World = View = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f };

	// inicializa a matriz de proje��o
	XMStoreFloat4x4(&Proj, XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f),
		window->AspectRatio(),
		1.0f, 100.0f));

	// contr�i geometria e inicializa pipeline
	graphics->ResetCommands();
	// ---------------------------------------
	BuildConstantBuffers();
	BuildGeometry();
	BuildRootSignature();
	BuildPipelineState();
	// ---------------------------------------
	graphics->SubmitCommands();





}

// ------------------------------------------------------------------------------

void Camera::Update()
{

	// sai com o pressionamento da tecla ESC
	if (input->KeyPress(VK_ESCAPE))
		window->Close();

	// ativa ou desativa o giro do objeto
	if (input->KeyPress('S'))
	{
		spin = spin ? false : true;

		if (spin)
			medidorTempo.Start();
		else
			medidorTempo.Stop();
	}


	float mousePosX = (float)input->MouseX();
	float mousePosY = (float)input->MouseY();

	if (input->KeyDown(VK_LBUTTON))
	{
		// cada pixel corresponde a 1/4 de grau
		float dx = XMConvertToRadians(0.25f * (mousePosX - lastMousePosX));
		float dy = XMConvertToRadians(0.25f * (mousePosY - lastMousePosY));

		// atualiza �ngulos com base no deslocamento do mouse 
		// para orbitar a c�mera ao redor da caixa
		theta += dx;
		phi += dy;

		// restringe o �ngulo de phi ]0-180[ graus
		phi = phi < 0.1f ? 0.1f : (phi > (XM_PI - 0.1f) ? XM_PI - 0.1f : phi);
	}
	else if (input->KeyDown(VK_RBUTTON))
	{
		// cada pixel corresponde a 0.05 unidades
		float dx = 0.05f * (mousePosX - lastMousePosX);
		float dy = 0.05f * (mousePosY - lastMousePosY);

		// atualiza o raio da c�mera com base no deslocamento do mouse 
		radius += dx - dy;

		// restringe o raio (3 a 15 unidades)
		radius = radius < 3.0f ? 3.0f : (radius > 15.0f ? 15.0f : radius);
	}

	lastMousePosX = mousePosX;
	lastMousePosY = mousePosY;

	// converte coordenadas esf�ricas para cartesianas
	float x = radius * sinf(phi) * cosf(theta);
	float z = radius * sinf(phi) * sinf(theta);
	float y = radius * cosf(phi);

	// constr�i a matriz da c�mera (view matrix)
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&View, view);

	// constr�i matriz combinada (world x view x proj)
	XMMATRIX world = XMMatrixRotationY(float(medidorTempo.Elapsed())/2);
	XMMATRIX proj = XMLoadFloat4x4(&Proj);
	XMMATRIX WorldViewProj = world * view * proj;

	// atualiza o buffer constante com a matriz combinada
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(WorldViewProj));
	memcpy(constantBufferData, &objConstants, sizeof(ObjectConstants));

}

// ------------------------------------------------------------------------------

void Camera::Draw()
{

	// limpa o backbuffer
	graphics->Clear(pipelineState);

	// comandos de configura��o do pipeline
	ID3D12DescriptorHeap* descriptorHeaps[] = { constantBufferHeap };
	graphics->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
	graphics->CommandList()->IASetVertexBuffers(0, 1, geometry->VertexBufferView());
	graphics->CommandList()->IASetIndexBuffer(geometry->IndexBufferView());
	graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	graphics->CommandList()->SetGraphicsRootDescriptorTable(0, constantBufferHeap->GetGPUDescriptorHandleForHeapStart());

	// comando de desenho
	graphics->CommandList()->DrawIndexedInstanced((uint)listIndex.size(), 1, 0, 0, 0);

	// apresenta o backbuffer na tela
	graphics->Present();

}

// ------------------------------------------------------------------------------

void Camera::Finalize()
{

	constantBufferUpload->Unmap(0, nullptr);
	constantBufferUpload->Release();
	constantBufferHeap->Release();

	rootSignature->Release();
	pipelineState->Release();
	delete geometry;

}



void Camera::readObject()
{
	ifstream fin;

	fin.open("Resources/esfera_icosaedrica.obj");


	if (!fin.is_open())
	{


		OutputDebugString("Imposs�vel abrir o arquivo .obj\n");
		exit(1);
	}
	else
	{

		string line;

		while (fin.good())
		{
			getline(fin, line);
			stringstream flush(line, std::stringstream::in);

			std::string s;
			flush >> s;



			// linha de normal
			if (flush.str()[0] == 'v' && flush.str()[1] == 'n')
			{
				contadorNormal++;
				//OutputDebugString("linha vn\n");
			}
			else
			{
				// linha de textura
				if (flush.str()[0] == 'v' && flush.str()[1] == 't')
				{
					contadorTexture++;
					//OutputDebugString("linha vt\n");
				}
				else
				{
					// linha de vertice
					if (flush.str()[0] == 'v')
					{
						//OutputDebugString("linha v\n");
						contadorVertex++;

						Vertex v;
						if (azul)
						{
							v.Color = XMFLOAT4(Colors::Blue);
							azul = !azul;
						}
						else
						{
							v.Color = XMFLOAT4(Colors::Pink);
							azul = !azul;
						}
						
						

						flush >> v.Pos.x >> v.Pos.y >> v.Pos.z;
						listVertex.push_back(v);


					}
					else
					{
						int tipo = 0;
						if (contadorNormal) tipo++;
						if (contadorVertex) tipo++;
						if (contadorTexture) tipo++;


						// face de f
						if (flush.str()[0] == 'f')
						{
							uint tamanho_anterior = (uint)listIndex.size();

							//OutputDebugString("linha f\n");
							ushort index;
							do {
								stringstream ss2;
								switch (tipo)
								{

									//f v1/t1/n1 v2/t2/n2 ...
								case 3:
									flush >> index;

									flush.ignore();
									// adicionando o indice no vetor de indices
									listIndex.push_back(index - 1);

									//ss2 << "Inseriu " << index - 1 << "\n";
									//OutputDebugString(ss2.str().c_str());
									flush >> index;
									//ignorando a /
									flush.ignore();
									flush >> index;
									//ignorando o espa�o
									flush.ignore();
									break;

									//f v1//t1 v2//t2 ...
								case 2:

									flush >> index;
									//ignorando as duas /
									flush.ignore();
									flush.ignore();

									listIndex.push_back(index - 1);

									flush >> index;
									//ignorando o espa�o
									flush.ignore();

									break;


								case 1:
									// f v1 v2 ...
									flush >> index;
									listIndex.push_back(index - 1);
									break;


								}

							} while (!flush.eof());

							
							uint vertices_lidos_linha = (uint)listIndex.size() - (uint)tamanho_anterior;


							//OutputDebugString("\nEntrou aqui\n");
							vector<ushort> manipulado;

							// copiando os novos valores adicionados para o vetor de manipula�ao
							for (size_t cont = tamanho_anterior; cont < listIndex.size(); cont++)
							{
								manipulado.push_back(listIndex[cont]);
							}

							//removendo os valores adicionados no vetor de manipulacao do vetor original
							for (size_t cont = 0; cont < vertices_lidos_linha; cont++)
							{
								listIndex.pop_back();
							}

							//manipulando o vetor de manipula��o para gerar os triangulos
							// thanks stackoverflow.com/questions/23723993/converting-quadriladerals-in-an-obj-file-into-triangles/23724231#23724231

							for (size_t cont = 1; cont < manipulado.size() - 1; cont++)
							{
								listIndex.push_back(manipulado[0]);
								listIndex.push_back(manipulado[cont]);
								listIndex.push_back(manipulado[cont + 1]);
							}
							//manipulado.clear();


						}

					}

				}
			}
		}
		/*
		for (auto& v : listVertex)
		{
			stringstream s;
			s <<"Vertice: " <<  v.Pos.x << "," << v.Pos.y << "," << v.Pos.z << "\n";
			OutputDebugString(s.str().c_str());
			//s.str("");
			//s << "Tamanho da listVertex = " << listVertex.size() << "\n";
			//OutputDebugString(s.str().c_str());

		}
		*/

		

	}
}

// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Camera::BuildConstantBuffers()
{

	// descritor do buffer constante
	// descritor do buffer constante
	// descritor do buffer constante
	D3D12_DESCRIPTOR_HEAP_DESC constantBufferHeapDesc = {};
	constantBufferHeapDesc.NumDescriptors = 1;
	constantBufferHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	constantBufferHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// cria descritor para buffer constante
	graphics->Device()->CreateDescriptorHeap(&constantBufferHeapDesc, IID_PPV_ARGS(&constantBufferHeap));

	// propriedades da heap do buffer de upload
	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 1;
	uploadHeapProperties.VisibleNodeMask = 1;

	// o tamanho dos "Constant Buffers" precisam ser m�ltiplos 
	// do tamanho de aloca��o m�nima do hardware (256 bytes)
	uint constantBufferSize = (sizeof(ObjectConstants) + 255) & ~255;

	// descri��o do buffer de upload
	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Alignment = 0;
	uploadBufferDesc.Width = constantBufferSize;
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// cria um buffer de upload para o buffer constante
	graphics->Device()->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantBufferUpload));

	// endere�o do buffer de upload na GPU
	D3D12_GPU_VIRTUAL_ADDRESS uploadAddress = constantBufferUpload->GetGPUVirtualAddress();

	// descreve view do buffer constante
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = uploadAddress;
	cbvDesc.SizeInBytes = constantBufferSize;

	// cria uma view para o buffer constante
	graphics->Device()->CreateConstantBufferView(
		&cbvDesc,
		constantBufferHeap->GetCPUDescriptorHandleForHeapStart());

	// mapeia mem�ria do upload buffer para um endere�o acess�vel pela CPU
	constantBufferUpload->Map(0, nullptr, reinterpret_cast<void**>(&constantBufferData));

}




void Camera::BuildGeometry()
{





	// -----------------------------------------------------------
	// >> Aloca��o e C�pia de Vertex e Index Buffers para a GPU <<
	// -----------------------------------------------------------

	// tamanho em bytes dos v�rtices e �ndices
	uint vbSize = (uint)listVertex.size() * sizeof(Vertex);
	uint ibSize = (uint)listIndex.size() * sizeof(ushort);
	
	// cria malha 3D
	geometry = new Mesh("Box");

	// ajusta atributos da malha 3D
	geometry->vertexByteStride = sizeof(Vertex);
	geometry->vertexBufferSize = vbSize;
	geometry->indexFormat = DXGI_FORMAT_R16_UINT;
	geometry->indexBufferSize = ibSize;

	// aloca recursos para o vertex buffer
	graphics->Allocate(vbSize, &geometry->vertexBufferCPU);
	graphics->Allocate(UPLOAD, vbSize, &geometry->vertexBufferUpload);
	graphics->Allocate(GPU, vbSize, &geometry->vertexBufferGPU);

	// aloca recursos para o index buffer
	graphics->Allocate(ibSize, &geometry->indexBufferCPU);
	graphics->Allocate(UPLOAD, ibSize, &geometry->indexBufferUpload);
	graphics->Allocate(GPU, ibSize, &geometry->indexBufferGPU);

	// guarda uma c�pia dos v�rtices e �ndices na malha
	graphics->Copy(&listVertex[0], vbSize, geometry->vertexBufferCPU);
	graphics->Copy(&listIndex[0], ibSize, geometry->indexBufferCPU);

	// copia v�rtices e �ndices para a GPU usando o buffer de Upload
	graphics->Copy(&listVertex[0], vbSize, geometry->vertexBufferUpload, geometry->vertexBufferGPU);
	graphics->Copy(&listIndex[0], ibSize, geometry->indexBufferUpload, geometry->indexBufferGPU);
}

// ------------------------------------------------------------------------------

void Camera::BuildRootSignature()
{

	// cria uma �nica tabela de descritores de CBVs
	D3D12_DESCRIPTOR_RANGE cbvTable = {};
	cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	cbvTable.NumDescriptors = 1;
	cbvTable.BaseShaderRegister = 0;
	cbvTable.RegisterSpace = 0;
	cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// par�metro raiz pode ser uma tabela, descritor raiz ou constantes raiz
	D3D12_ROOT_PARAMETER rootParameters[1];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvTable;

	// uma assinatura raiz � um vetor de par�metros raiz
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = rootParameters;
	rootSigDesc.NumStaticSamplers = 0;
	rootSigDesc.pStaticSamplers = nullptr;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// serializa assinatura raiz
	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* error = nullptr;

	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig,
		&error));

	if (error != nullptr)
	{
		OutputDebugString((char*)error->GetBufferPointer());
	}

	// cria uma assinatura raiz com um �nico slot que aponta para  
	// uma faixa de descritores consistindo de um �nico buffer constante
	ThrowIfFailed(graphics->Device()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)));

}

// ------------------------------------------------------------------------------

void Camera::BuildPipelineState()
{

	// --------------------
	// --- Input Layout ---
	// --------------------

	D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// --------------------
	// ----- Shaders ------
	// --------------------

	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;

	D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
	D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

	// --------------------
	// ---- Rasterizer ----
	// --------------------

	D3D12_RASTERIZER_DESC rasterizer = {};
	rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
	//rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;
	rasterizer.FrontCounterClockwise = FALSE;
	rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizer.DepthClipEnable = TRUE;
	rasterizer.MultisampleEnable = FALSE;
	rasterizer.AntialiasedLineEnable = FALSE;
	rasterizer.ForcedSampleCount = 0;
	rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// ---------------------
	// --- Color Blender ---
	// ---------------------

	D3D12_BLEND_DESC blender = {};
	blender.AlphaToCoverageEnable = FALSE;
	blender.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

	// ---------------------
	// --- Depth Stencil ---
	// ---------------------

	D3D12_DEPTH_STENCIL_DESC depthStencil = {};
	depthStencil.DepthEnable = TRUE;
	depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencil.StencilEnable = FALSE;
	depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	depthStencil.FrontFace = defaultStencilOp;
	depthStencil.BackFace = defaultStencilOp;

	// -----------------------------------
	// --- Pipeline State Object (PSO) ---
	// -----------------------------------

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
	pso.pRootSignature = rootSignature;
	pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
	pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
	pso.BlendState = blender;
	pso.SampleMask = UINT_MAX;
	pso.RasterizerState = rasterizer;
	pso.DepthStencilState = depthStencil;
	pso.InputLayout = { inputLayout, 2 };
	pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso.NumRenderTargets = 1;
	pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pso.SampleDesc.Count = graphics->Antialiasing();
	pso.SampleDesc.Quality = graphics->Quality();
	graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

	vertexShader->Release();
	pixelShader->Release();

}



// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	try
	{
		// cria motor e configura a janela
		Engine* engine = new Engine();
		engine->window->Mode(WINDOWED);
		engine->window->Size(800, 600);
		engine->window->Color(30, 30, 30);
		engine->window->Title("C�mera");
		engine->window->Icon(IDI_ICON);
		engine->window->Cursor(IDC_CURSOR);
		engine->window->LostFocus(Engine::Pause);
		engine->window->InFocus(Engine::Resume);

		// cria e executa a aplica��o
		int exit = engine->Start(new Camera());

		// finaliza execu��o
		delete engine;
		return exit;
	}
	catch (Error& e)
	{
		// exibe mensagem em caso de erro
		MessageBox(nullptr, e.ToString().data(), "C�mera", MB_OK);
		return 0;
	}
}

// ----------------------------------------------------------------------------

