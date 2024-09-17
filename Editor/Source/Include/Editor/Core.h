//
// Created by johnk on 2024/6/23.
//

#pragma once

#include <Runtime/Engine.h>
#include <Rendering/RenderingModule.h>

namespace Editor {
    class Core {
    public:
        static Core& Get();

        ~Core();

        void Initialize(int argc, char** argv);
        void Cleanup();
        bool ProjectHasSet() const;
        Rendering::RenderingModule* GetRenderingModule() const;
        Runtime::Engine* GetEngine() const;

    private:
        Core();

        void ParseCommandLineArgs(int argc, char** argv) const;
        void InitializeRuntime();
        void InitializeRendering();

        Rendering::RenderingModule* renderingModule;
        Runtime::Engine* engine;
    };
}