#include "AsciiPlot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace diagnostics = src::common::Diagnostics;

namespace src::core::GraphEngine::AsciiPlot
{
namespace
{

PlotResult makeInvalidArgumentError(std::string message, diagnostics::Context context = {})
{
    diagnostics::Errors::Error error;

    error.domain = diagnostics::ErrorDomain::UI;
    error.code = diagnostics::ErrorCode::InvalidArgument;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = std::move(message);
    error.context = std::move(context);

    return PlotResult::failure(std::move(error));
}

bool isValidConfig(const PlotConfig& config)
{
    return config.width >= 3 && config.height >= 3 &&
           std::isfinite(config.yMin) && std::isfinite(config.yMax) &&
           config.yMin < config.yMax;
}

std::size_t mapX(double x, double xMin, double xMax, std::size_t width)
{
    const double normalized = (x - xMin) / (xMax - xMin);

    const double clamped = std::clamp(normalized, 0.0, 1.0);

    const double position = clamped * static_cast<double>(width - 1);

    return static_cast<std::size_t>(std::lround(position));
}

std::size_t mapY(double y, double yMin, double yMax, std::size_t height)
{
    const double normalized = (y - yMin) / (yMax - yMin);

    const double clamped = std::clamp(normalized, 0.0, 1.0);

    const double position = (1.0 - clamped) * static_cast<double>(height - 1);

    return static_cast<std::size_t>(std::lround(position));
}

std::optional<std::size_t> findXAxisRow(double yMin, double yMax, std::size_t height)
{
    if (yMin > 0.0 || yMax < 0.0)
        return std::nullopt;

    return mapY(0.0, yMin, yMax, height);
}

std::optional<std::size_t> findYAxisColumn(double xMin, double xMax, std::size_t width
)
{
    if (xMin > 0.0 || xMax < 0.0) return std::nullopt;

    return mapX(0.0, xMin, xMax, width);
}

void drawAxes(std::vector<std::string>& canvas, const PlotConfig& config, double xMin, double xMax
)
{
    if (!config.drawAxes) return;

    const std::size_t height = canvas.size();
    const std::size_t width = canvas.front().size();

    const std::optional<std::size_t> xAxisRow = findXAxisRow(config.yMin, config.yMax, height);
    const std::optional<std::size_t> yAxisColumn = findYAxisColumn(xMin, xMax, width);

    if (xAxisRow.has_value())
    {
        const std::size_t row = *xAxisRow;

        for (std::size_t column = 0; column < width; column++)
        {
            canvas[row][column] = config.xAxisSymbol;
        }
    }

    if (yAxisColumn.has_value())
    {
        const std::size_t column = *yAxisColumn;

        for (std::size_t row = 0; row < height; row++)
        {
            canvas[row][column] = config.yAxisSymbol;
        }
    }

    if (xAxisRow.has_value() && yAxisColumn.has_value())
    {
        canvas[*xAxisRow][*yAxisColumn] = config.originSymbol;
    }
}

std::string canvasToString(const std::vector<std::string>& canvas)
{
    std::string output;

    for (const std::string& row : canvas)
    {
        output += row;
        output += '\n';
    }

    return output;
}

}

PlotResult renderAscii(const Graph::SamplePoints& points, const PlotConfig& config)
{
    if (points.empty())
        return makeInvalidArgumentError("Cannot render an empty set of sample points");

    if (!isValidConfig(config))
    {
        return makeInvalidArgumentError(
            "Invalid ASCII plot configuration",
            {
                {"width", static_cast<std::int64_t>(config.width)},
                {"height", static_cast<std::int64_t>(config.height)},
                {"yMin", config.yMin},
                {"yMax", config.yMax}
            }
        );
    }

    double xMin = std::numeric_limits<double>::infinity();
    double xMax = -std::numeric_limits<double>::infinity();

    for (const Graph::SamplePoint& point : points)
    {
        if (!std::isfinite(point.x))
        {
            return makeInvalidArgumentError("Sample point contains a non-finite x value");
        }

        xMin = std::min(xMin, point.x);
        xMax = std::max(xMax, point.x);
    }

    if (!std::isfinite(xMin) || !std::isfinite(xMax) || xMin >= xMax)
    {
        return makeInvalidArgumentError(
            "Sample points must contain a non-zero x range",
            {
                {"xMin", xMin},
                {"xMax", xMax}
            }
        );
    }

    std::vector<std::string> canvas(config.height, std::string(config.width, ' '));

    drawAxes(canvas, config, xMin, xMax);

    for (const Graph::SamplePoint& point : points)
    {
        if (!point.y.has_value()) continue;

        const double y = *point.y;

        if (!std::isfinite(y)) continue;

        if (y < config.yMin || y > config.yMax) continue;

        const std::size_t column = mapX(point.x, xMin, xMax, config.width);

        const std::size_t row = mapY(y, config.yMin, config.yMax, config.height);

        canvas[row][column] = config.curveSymbol;
    }

    return PlotResult::success(canvasToString(canvas));
}

}