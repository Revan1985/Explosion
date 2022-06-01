//
// Created by johnk on 2022/5/25.
//

#pragma once

#include <fstream>

#include <Common/Utility.h>

namespace MetaTool {
    struct MetaInfo;

    class HeaderGenerator {
    public:
        NON_COPYABLE(HeaderGenerator)
        explicit HeaderGenerator(const char* outputFilePath);
        ~HeaderGenerator();

        void Generate(const MetaTool::MetaInfo& metaInfo);

    private:
    };
}