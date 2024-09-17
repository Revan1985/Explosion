//
// Created by johnk on 2024/6/23.
//

#include <Editor/Core.h>
#include <Core/Cmdline.h>

Core::CmdlineArgValue<std::string> caRhiType(
    "rhiType", "-rhi", RHI::GetPlatformDefaultRHIAbbrString(),
    "rhi abbr string, can be 'dx12' or 'vulkan'");

Core::CmdlineArgValue<std::string> caProjectFile(
    "projectFile", "-project", "",
    "project file path");

namespace Editor {
    Core& Core::Get()
    {
        static Core instance;
        return instance;
    }

    Core::Core()
        : renderingModule(nullptr)
        , engine(nullptr)
    {
    }

    Core::~Core() = default;

    void Core::Initialize(int argc, char** argv)
    {
        ParseCommandLineArgs(argc, argv);
        InitializeRuntime();
        InitializeRendering();
    }

    void Core::Cleanup() // NOLINT
    {
        ::Core::ModuleManager::Get().Unload("Runtime");
        ::Core::ModuleManager::Get().Unload("Rendering");
    }

    Runtime::Engine* Core::GetEngine() const
    {
        return engine;
    }

    bool Core::ProjectHasSet() const // NOLINT
    {
        return !caProjectFile.GetValue().empty();
    }

    Rendering::RenderingModule* Core::GetRenderingModule() const
    {
        return renderingModule;
    }

    void Core::ParseCommandLineArgs(int argc, char** argv) const // NOLINT
    {
        ::Core::Cli::Get().Parse(argc, argv);
    }

    void Core::InitializeRuntime()
    {
        Runtime::EngineInitParams params {};
        params.projectFile = caProjectFile.GetValue();
        Runtime::EngineHolder::Load("Editor", params);
        engine = &Runtime::EngineHolder::Get();
    }

    void Core::InitializeRendering()
    {
        renderingModule = ::Core::ModuleManager::Get().FindOrLoadTyped<Rendering::RenderingModule>("Rendering");
        Assert(renderingModule != nullptr);

        Rendering::RenderingModuleInitParams initParams;
        initParams.rhiType = RHI::RHIAbbrStringToRHIType(caRhiType.GetValue());
        renderingModule->Initialize(initParams);
    }
}