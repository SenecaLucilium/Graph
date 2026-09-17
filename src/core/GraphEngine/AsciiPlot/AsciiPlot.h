#pragma once

#include "GraphSampler.h"
#include "common/Diagnostics/Result.h"

#include <cstddef>
#include <string>

namespace src::core::GraphEngine::AsciiPlot
{

using SamplePoints = src::core::ExpressionEngine::Graph::SamplePoints;

struct PlotConfig
{
    std::size_t width = 80;
    std::size_t height = 24;

    double yMin = -10.0;
    double yMax = 10.0;

    char curveSymbol = '*';
    char xAxisSymbol = '-';
    char yAxisSymbol = '|';
    char originSymbol = '+';

    bool drawAxes = true;
    bool drawLabels = true;
    bool connectPoints = true;
    double discontinuityJumpFactor = 0.75;
};

using PlotResult = src::common::Diagnostics::Result<std::string>;

PlotResult renderAscii(const SamplePoints& points, const PlotConfig& config = {});

}
