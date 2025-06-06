//
// Created by johnk on 2024/6/20.
//

#include <Application.h>
#include <RHI/RHI.h>
#include <Render/ShaderCompiler.h>
#include <Render/RenderGraph.h>
#include <Render/RenderThread.h>

using namespace Common;
using namespace Render;
using namespace RHI;

struct Vertex {
    FVec3 position;
};

class TriangleVS : public GlobalShader {
public:
    ShaderInfo(
        "TriangleVS",
        "Engine/Test/Sample/Rendering-Triangle/Triangle.esl",
        "VSMain",
        RHI::ShaderStageBits::sVertex);

    NonVariant;
    DefaultVariantFilter;
};

class TrianglePS : public GlobalShader {
public:
    ShaderInfo(
        "TrianglePS",
        "Engine/Test/Sample/Rendering-Triangle/Triangle.esl",
        "PSMain",
        RHI::ShaderStageBits::sPixel);

    NonVariant;
    DefaultVariantFilter;
};

RegisterGlobalShader(TriangleVS);
RegisterGlobalShader(TrianglePS);

struct PsUniform {
    FVec3 pixelColor;
};

class TriangleApplication final : public Application {
public:
    explicit TriangleApplication(const std::string& inName);
    ~TriangleApplication() override;

    void OnCreate() override;
    void OnDrawFrame() override;
    void OnDestroy() override;

private:
    static constexpr size_t backBufferCount = 2;

    void CreateDevice();
    void CompileAllShaders();
    void CreateSwapChain();
    void CreateTriangleVertexBuffer();
    void CreateSyncObjects();

    PixelFormat swapChainFormat;
    ShaderInstance triangleVS;
    ShaderInstance trianglePS;
    UniquePtr<Device> device;
    UniquePtr<Surface> surface;
    UniquePtr<SwapChain> swapChain;
    std::array<Texture*, backBufferCount> swapChainTextures;
    UniquePtr<Buffer> triangleVertexBuffer;
    UniquePtr<Semaphore> imageReadySemaphore;
    UniquePtr<Semaphore> renderFinishedSemaphore;
    UniquePtr<Fence> frameFence;
};

TriangleApplication::TriangleApplication(const std::string& inName)
    : Application(inName)
    , swapChainFormat(PixelFormat::max)
    , swapChainTextures()
{
}

TriangleApplication::~TriangleApplication() = default;

void TriangleApplication::OnCreate()
{
    RenderWorkerThreads::Get().Start();
    CreateDevice();
    CompileAllShaders();
    CreateSwapChain();
    CreateTriangleVertexBuffer();
    CreateSyncObjects();
}

void TriangleApplication::OnDrawFrame()
{
    frameFence->Reset();
    const auto backTextureIndex = swapChain->AcquireBackTexture(imageReadySemaphore.Get());

    auto* pso = PipelineCache::Get(*device).GetOrCreate(
        RasterPipelineStateDesc()
            .SetVertexShader(triangleVS)
            .SetPixelShader(trianglePS)
            .SetVertexState(
                RVertexState()
                    .AddVertexBufferLayout(
                        RVertexBufferLayout(VertexStepMode::perVertex, sizeof(Vertex))
                            .AddAttribute(RVertexAttribute(RVertexBinding("POSITION", 0), VertexFormat::float32X3, offsetof(Vertex, position)))))
            .SetFragmentState(
                RFragmentState()
                    .AddColorTarget(ColorTargetState(swapChainFormat, ColorWriteBits::all, false))));

    RGBuilder builder(*device);
    auto* backTexture = builder.ImportTexture(swapChainTextures[backTextureIndex], TextureState::present);
    auto* backTextureView = builder.CreateTextureView(backTexture, RGTextureViewDesc(TextureViewType::colorAttachment, TextureViewDimension::tv2D));
    auto* vertexBuffer = builder.ImportBuffer(triangleVertexBuffer.Get(), BufferState::shaderReadOnly);
    auto* vertexBufferView = builder.CreateBufferView(vertexBuffer, RGBufferViewDesc(BufferViewType::vertex, vertexBuffer->GetDesc().size, 0, VertexBufferViewInfo(sizeof(Vertex))));
    auto* psUniformBuffer = builder.CreateBuffer(RGBufferDesc(sizeof(PsUniform), BufferUsageBits::uniform | BufferUsageBits::mapWrite, BufferState::staging, "psUniform"));
    auto* psUniformBufferView = builder.CreateBufferView(psUniformBuffer, RGBufferViewDesc(BufferViewType::uniformBinding, sizeof(PsUniform)));

    auto* bindGroup = builder.AllocateBindGroup(
        RGBindGroupDesc::Create(pso->GetPipelineLayout()->GetBindGroupLayout(0))
            .UniformBuffer("psUniform", psUniformBufferView));

    PsUniform psUniform {};
    psUniform.pixelColor = FVec3(
        (std::sin(GetCurrentTimeSeconds()) + 1) / 2,
        (std::cos(GetCurrentTimeSeconds()) + 1) / 2,
        std::abs(std::sin(GetCurrentTimeSeconds())));

    builder.QueueBufferUpload(
        psUniformBuffer,
        RGBufferUploadInfo(&psUniform, sizeof(PsUniform)));

    builder.AddRasterPass(
        "BasePass",
        RGRasterPassDesc()
            .AddColorAttachment(RGColorAttachment(backTextureView, LoadOp::clear, StoreOp::store)),
        { bindGroup },
        [pso, vertexBufferView, bindGroup, viewportWidth = GetWindowWidth(), viewportHeight = GetWindowHeight()](const RGBuilder& rg, RasterPassCommandRecorder& recorder) -> void {
            recorder.SetPipeline(pso->GetRHI());
            recorder.SetScissor(0, 0, viewportWidth, viewportHeight);
            recorder.SetViewport(0, 0, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0, 1);
            recorder.SetVertexBuffer(0, rg.GetRHI(vertexBufferView));
            recorder.SetPrimitiveTopology(PrimitiveTopology::triangleList);
            recorder.SetBindGroup(0, rg.GetRHI(bindGroup));
            recorder.Draw(3, 1, 0, 0);
        },
        {},
        [backTexture](const RGBuilder& rg, CommandRecorder& recorder) -> void {
            recorder.ResourceBarrier(Barrier::Transition(rg.GetRHI(backTexture), TextureState::renderTarget, TextureState::present));
        });

    RGExecuteInfo executeInfo;
    executeInfo.semaphoresToWait = { imageReadySemaphore.Get() };
    executeInfo.semaphoresToSignal = { renderFinishedSemaphore.Get() };
    executeInfo.inFenceToSignal = frameFence.Get();
    builder.Execute(executeInfo);
    swapChain->Present(renderFinishedSemaphore.Get());
    frameFence->Wait();

    Core::ThreadContext::IncFrameNumber();
    BufferPool::Get(*device).Forfeit();
    TexturePool::Get(*device).Forfeit();
    ResourceViewCache::Get(*device).Forfeit();
    BindGroupCache::Get(*device).Forfeit();
}

void TriangleApplication::OnDestroy()
{
    const UniquePtr<Fence> fence = device->CreateFence(false);
    device->GetQueue(QueueType::graphics, 0)->Flush(fence.Get());
    fence->Wait();

    BindGroupCache::Get(*device).Invalidate();
    PipelineCache::Get(*device).Invalidate();
    BufferPool::Get(*device).Invalidate();
    TexturePool::Get(*device).Invalidate();
    GlobalShaderRegistry::Get().Invalidate();
    RenderWorkerThreads::Get().Stop();
}

void TriangleApplication::CreateDevice()
{
    device = GetRHIInstance()
        ->GetGpu(0)
        ->RequestDevice(
            DeviceCreateInfo()
                .AddQueueRequest(QueueRequestInfo(QueueType::graphics, 1)));
}

void TriangleApplication::CompileAllShaders()
{
    ShaderCompileOptions options;
    options.includePaths = { "../Test/Sample/ShaderInclude", "../Test/Sample/Rendering-Triangle" };
    options.byteCodeType = GetRHIType() == RHI::RHIType::directX12 ? ShaderByteCodeType::dxil : ShaderByteCodeType::spirv;
    options.withDebugInfo = false;
    auto result = ShaderTypeCompiler::Get().CompileGlobalShaderTypes(options);
    const auto& [success, errorInfo] = result.get();
    Assert(success);

    triangleVS = GlobalShaderMap<TriangleVS>::Get().GetShaderInstance(*device, {});
    trianglePS = GlobalShaderMap<TrianglePS>::Get().GetShaderInstance(*device, {});
}

void TriangleApplication::CreateSwapChain()
{
    static std::vector swapChainFormatQualifiers = {
        PixelFormat::rgba8Unorm,
        PixelFormat::bgra8Unorm
    };

    surface = device->CreateSurface(SurfaceCreateInfo(GetPlatformWindow()));

    for (const auto format : swapChainFormatQualifiers) {
        if (device->CheckSwapChainFormatSupport(surface.Get(), format)) {
            swapChainFormat = format;
            break;
        }
    }
    Assert(swapChainFormat != PixelFormat::max);

    swapChain = device->CreateSwapChain(
        SwapChainCreateInfo()
            .SetFormat(swapChainFormat)
            .SetPresentMode(PresentMode::immediately)
            .SetTextureNum(backBufferCount)
            .SetWidth(GetWindowWidth())
            .SetHeight(GetWindowHeight())
            .SetSurface(surface.Get())
            .SetPresentQueue(device->GetQueue(QueueType::graphics, 0)));

    for (auto i = 0; i < backBufferCount; i++) {
        swapChainTextures[i] = swapChain->GetTexture(i);
    }
}

void TriangleApplication::CreateTriangleVertexBuffer()
{
    const std::vector<Vertex> vertices = {
        { { -.5f, -.5f, 0.f } },
        { { .5f, -.5f, 0.f } },
        { { 0.f, .5f, 0.f } },
    };

    const BufferCreateInfo bufferCreateInfo = BufferCreateInfo()
        .SetSize(vertices.size() * sizeof(Vertex))
        .SetUsages(BufferUsageBits::vertex | BufferUsageBits::mapWrite | BufferUsageBits::copySrc)
        .SetInitialState(BufferState::staging)
        .SetDebugName("vertexBuffer");

    triangleVertexBuffer = device->CreateBuffer(bufferCreateInfo);
    if (triangleVertexBuffer != nullptr) {
        auto* data = triangleVertexBuffer->Map(MapMode::write, 0, bufferCreateInfo.size);
        memcpy(data, vertices.data(), bufferCreateInfo.size);
        triangleVertexBuffer->UnMap();
    }
}

void TriangleApplication::CreateSyncObjects()
{
    imageReadySemaphore = device->CreateSemaphore();
    renderFinishedSemaphore = device->CreateSemaphore();
    frameFence = device->CreateFence(true);
}

int main(int argc, char* argv[])
{
    TriangleApplication application("Rendering-Triangle");
    if (!application.Initialize(argc, argv)) {
        return -1;
    }
    return application.RunLoop();
}
