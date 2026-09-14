#pragma once

#include "GraphSampler.h"
#include "common/Diagnostics/Result.h"

#include <cstddef>
#include <string>

namespace Graph = src::core::ExpressionEngine::Graph;

namespace src::core::GraphEngine::AsciiPlot
{

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
};

using PlotResult = src::common::Diagnostics::Result<std::string>;

PlotResult renderAscii(const Graph::SamplePoints& points, const PlotConfig& config = {});

}