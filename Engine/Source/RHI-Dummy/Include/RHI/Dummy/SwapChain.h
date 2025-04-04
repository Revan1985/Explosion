//
// Created by johnk on 2023/3/21.
//

#pragma once

#include <vector>

#include <RHI/SwapChain.h>

namespace RHI::Dummy {
    class DummyTexture;

    class DummySwapChain final : public SwapChain {
    public:
        NonCopyable(DummySwapChain)
        explicit DummySwapChain(const SwapChainCreateInfo& createInfo);
        ~DummySwapChain() override;

        uint8_t GetTextureNum() override;
        Texture* GetTexture(uint8_t index) override;
        uint8_t AcquireBackTexture(Semaphore* signalSemaphore) override;
        void Present(Semaphore* waitSemaphore) override;

    private:
        bool pingPong;
        std::vector<Common::UniquePtr<DummyTexture>> dummyTextures;
    };
}
