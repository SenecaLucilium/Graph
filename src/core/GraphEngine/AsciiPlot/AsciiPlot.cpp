#include "AsciiPlot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace diagnostics = src::common::Diagnostics;
namespace expressionGraph = src::core::ExpressionEngine::Graph;

namespace src::core::GraphEngine::AsciiPlot
{
namespace
{

struct Tick
{
    double value;
    std::size_t position;
    std::string label;
};

struct Layout
{
    std::size_t labelWidth = 0;
    std::size_t plotWidth = 0;
    std::size_t plotHeight = 0;
    std::optional<std::size_t> xAxisRow;
    std::optional<std::size_t> yAxisColumn;
    std::vector<Tick> xTicks;
    std::vector<Tick> yTicks;
};

PlotResult makeInvalidArgumentError(
    std::string message,
    diagnostics::Context context = {}
)
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
    return config.width >= 20 &&
           config.height >= 5 &&
           std::isfinite(config.yMin) &&
           std::isfinite(config.yMax) &&
           config.yMin < config.yMax &&
           std::isfinite(config.discontinuityJumpFactor) &&
           config.discontinuityJumpFactor > 0.0;
}

std::size_t mapX(
    double x,
    double xMin,
    double xMax,
    std::size_t plotWidth
)
{
    const double normalized = (x - xMin) / (xMax - xMin);
    const double clamped = std::clamp(normalized, 0.0, 1.0);
    const double position = clamped * static_cast<double>(plotWidth - 1);

    return static_cast<std::size_t>(std::lround(position));
}

std::size_t mapY(
    double y,
    double yMin,
    double yMax,
    std::size_t plotHeight
)
{
    const double normalized = (y - yMin) / (yMax - yMin);
    const double clamped = std::clamp(normalized, 0.0, 1.0);
    const double position = (1.0 - clamped) * static_cast<double>(plotHeight - 1);

    return static_cast<std::size_t>(std::lround(position));
}

double chooseNiceStep(double range, std::size_t targetTickCount = 6)
{
    const double rawStep = range / static_cast<double>(targetTickCount);
    const double exponent = std::floor(std::log10(rawStep));
    const double magnitude = std::pow(10.0, exponent);
    const double normalized = rawStep / magnitude;

    double niceNormalized = 1.0;

    if (normalized <= 1.0) niceNormalized = 1.0;
    else if (normalized <= 2.0) niceNormalized = 2.0;
    else if (normalized <= 5.0) niceNormalized = 5.0;
    else niceNormalized = 10.0;

    return niceNormalized * magnitude;
}

std::size_t decimalPlaces(double step)
{
    if (step >= 1.0) return 0;

    const double exponent = std::floor(std::log10(step));
    return static_cast<std::size_t>(std::max(0.0, -exponent));
}

std::string formatNumber(double value, double step)
{
    if (std::abs(value) < step * 1e-8) value = 0.0;

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(static_cast<int>(decimalPlaces(step))) << value;

    std::string result = stream.str();

    if (result.find('.') != std::string::npos)
    {
        while (result.size() > 1 && result.back() == '0')
            result.pop_back();

        if (!result.empty() && result.back() == '.')
            result.pop_back();
    }

    if (result == "-0") result = "0";

    return result;
}

std::vector<Tick> makeTicks(double minimum, double maximum, std::size_t positionSize, std::size_t targetTickCount, bool reversePosition)
{
    const double step = chooseNiceStep(maximum - minimum, targetTickCount);
    const double first = std::ceil(minimum / step) * step;
    const double epsilon = step * 1e-9;
    std::vector<Tick> ticks;

    for (double value = first; value <= maximum + epsilon; value += step)
    {
        const double normalized = (value - minimum) / (maximum - minimum);
        const double oriented = reversePosition ? 1.0 - normalized : normalized;
        const double position = std::clamp(oriented, 0.0, 1.0) * static_cast<double>(positionSize - 1);

        ticks.push_back(Tick{
            .value = value,
            .position = static_cast<std::size_t>(std::lround(position)),
            .label = formatNumber(value, step)
        });

        if (ticks.size() > 1000) break;
    }

    return ticks;
}

std::optional<std::size_t> findXAxisRow(
    double yMin,
    double yMax,
    std::size_t plotHeight
)
{
    if (yMin > 0.0 || yMax < 0.0) return std::nullopt;
    return mapY(0.0, yMin, yMax, plotHeight);
}

std::optional<std::size_t> findYAxisColumn(
    double xMin,
    double xMax,
    std::size_t plotWidth
)
{
    if (xMin > 0.0 || xMax < 0.0) return std::nullopt;
    return mapX(0.0, xMin, xMax, plotWidth);
}

std::size_t calculateLabelWidth(
    const std::vector<Tick>& yTicks,
    bool drawLabels
)
{
    if (!drawLabels) return 0;

    std::size_t width = 0;

    for (const Tick& tick : yTicks)
        width = std::max(width, tick.label.size());

    return width;
}

Layout calculateLayout(
    const PlotConfig& config,
    double xMin,
    double xMax
)
{
    Layout layout;

    layout.plotHeight = config.height - (config.drawLabels ? 2 : 0);
    layout.yTicks = makeTicks(config.yMin, config.yMax, layout.plotHeight, 6, true);
    layout.labelWidth = calculateLabelWidth(layout.yTicks, config.drawLabels);

    const std::size_t leftMargin = config.drawLabels ? layout.labelWidth + 1 : 0;

    if (leftMargin >= config.width)
        return layout;

    layout.plotWidth = config.width - leftMargin;
    layout.xTicks = makeTicks(xMin, xMax, layout.plotWidth, 7, false);
    layout.xAxisRow = findXAxisRow(config.yMin, config.yMax, layout.plotHeight);
    layout.yAxisColumn = findYAxisColumn(xMin, xMax, layout.plotWidth);

    return layout;
}

void putCharacter(
    std::vector<std::string>& canvas,
    std::size_t row,
    std::size_t column,
    char character
)
{
    if (row < canvas.size() && column < canvas[row].size())
        canvas[row][column] = character;
}

void drawAxes(
    std::vector<std::string>& canvas,
    const PlotConfig& config,
    const Layout& layout
)
{
    if (!config.drawAxes) return;

    const std::size_t leftMargin = config.drawLabels ? layout.labelWidth + 1 : 0;

    if (layout.xAxisRow.has_value())
    {
        for (std::size_t column = 0; column < layout.plotWidth; column++)
            putCharacter(canvas, *layout.xAxisRow, leftMargin + column, config.xAxisSymbol);
    }

    if (layout.yAxisColumn.has_value())
    {
        for (std::size_t row = 0; row < layout.plotHeight; row++)
            putCharacter(canvas, row, leftMargin + *layout.yAxisColumn, config.yAxisSymbol);
    }

    if (layout.xAxisRow.has_value() && layout.yAxisColumn.has_value())
    {
        putCharacter(
            canvas,
            *layout.xAxisRow,
            leftMargin + *layout.yAxisColumn,
            config.originSymbol
        );
    }
}

void drawTicks(
    std::vector<std::string>& canvas,
    const PlotConfig& config,
    const Layout& layout
)
{
    if (!config.drawAxes) return;

    const std::size_t leftMargin = config.drawLabels ? layout.labelWidth + 1 : 0;

    for (const Tick& tick : layout.xTicks)
    {
        const std::size_t column = leftMargin + tick.position;

        if (layout.xAxisRow.has_value())
            putCharacter(canvas, *layout.xAxisRow, column, config.originSymbol);
        else
            putCharacter(canvas, layout.plotHeight - 1, column, config.originSymbol);
    }

    for (const Tick& tick : layout.yTicks)
    {
        const std::size_t row = tick.position;

        if (layout.yAxisColumn.has_value())
            putCharacter(canvas, row, leftMargin + *layout.yAxisColumn, config.originSymbol);
        else
            putCharacter(canvas, row, leftMargin, config.originSymbol);
    }

    if (layout.xAxisRow.has_value() && layout.yAxisColumn.has_value())
    {
        putCharacter(
            canvas,
            *layout.xAxisRow,
            leftMargin + *layout.yAxisColumn,
            config.originSymbol
        );
    }
}

void drawAxisArrows(std::vector<std::string>& canvas, const PlotConfig& config, const Layout& layout)
{
    if (!config.drawAxes || layout.plotWidth == 0 || layout.plotHeight == 0) return;

    const std::size_t leftMargin = config.drawLabels ? layout.labelWidth + 1 : 0;

    if (layout.xAxisRow.has_value())
        putCharacter(canvas, *layout.xAxisRow, leftMargin + layout.plotWidth - 1, config.xAxisArrow);

    if (layout.yAxisColumn.has_value())
        putCharacter(canvas, 0, leftMargin + *layout.yAxisColumn, config.yAxisArrow);
}

void writeRightAligned(
    std::string& row,
    std::size_t width,
    const std::string& text
)
{
    if (width == 0 || row.empty()) return;

    const std::size_t textWidth = std::min(width, text.size());
    const std::size_t start = width - textWidth;

    for (std::size_t index = 0; index < textWidth; index++)
        row[start + index] = text[text.size() - textWidth + index];
}

void writeCentered(
    std::string& row,
    std::size_t center,
    const std::string& text
)
{
    if (row.empty() || text.empty()) return;

    const std::size_t half = text.size() / 2;
    const std::size_t start = center > half ? center - half : 0;

    for (std::size_t index = 0; index < text.size(); index++)
    {
        const std::size_t column = start + index;
        if (column < row.size() && row[column] == ' ')
            row[column] = text[index];
    }
}

void drawLabels(
    std::vector<std::string>& canvas,
    const PlotConfig& config,
    const Layout& layout
)
{
    if (!config.drawLabels) return;

    const std::size_t leftMargin = layout.labelWidth + 1;

    for (const Tick& tick : layout.yTicks)
    {
        if (tick.position < layout.plotHeight)
            writeRightAligned(canvas[tick.position], layout.labelWidth, tick.label);
    }

    if (layout.plotHeight >= canvas.size()) return;

    std::string& labelRow = canvas[layout.plotHeight + 1];

    std::size_t lastLabelEnd = 0;
    bool hasPreviousLabel = false;

    for (const Tick& tick : layout.xTicks)
    {
        const std::size_t column = leftMargin + tick.position;

        const std::size_t half = tick.label.size() / 2;
        const std::size_t start = column > half ? column - half : 0;
        const std::size_t end = start + tick.label.size();

        if (hasPreviousLabel && start <= lastLabelEnd) continue;

        writeCentered(labelRow, column, tick.label);
        lastLabelEnd = end;
        hasPreviousLabel = true;
    }
}

void drawLine(
    std::vector<std::string>& canvas,
    std::size_t row1,
    std::size_t column1,
    std::size_t row2,
    std::size_t column2,
    std::size_t leftMargin,
    char symbol
)
{
    int x1 = static_cast<int>(column1);
    int y1 = static_cast<int>(row1);
    const int x2 = static_cast<int>(column2);
    const int y2 = static_cast<int>(row2);

    const int dx = std::abs(x2 - x1);
    const int sx = x1 < x2 ? 1 : -1;
    const int dy = -std::abs(y2 - y1);
    const int sy = y1 < y2 ? 1 : -1;
    int error = dx + dy;

    while (true)
    {
        putCharacter(
            canvas,
            static_cast<std::size_t>(y1),
            leftMargin + static_cast<std::size_t>(x1),
            symbol
        );

        if (x1 == x2 && y1 == y2) break;

        const int doubledError = 2 * error;

        if (doubledError >= dy)
        {
            error += dy;
            x1 += sx;
        }

        if (doubledError <= dx)
        {
            error += dx;
            y1 += sy;
        }
    }
}

void drawCurve(
    std::vector<std::string>& canvas,
    const PlotConfig& config,
    const Layout& layout,
    const SamplePoints& points,
    double xMin,
    double xMax
)
{
    const std::size_t leftMargin = config.drawLabels ? layout.labelWidth + 1 : 0;
    const double jumpLimit = (config.yMax - config.yMin) * config.discontinuityJumpFactor;

    std::optional<std::pair<std::size_t, std::size_t>> previousPixel;
    std::optional<double> previousY;

    for (const expressionGraph::SamplePoint& point : points)
    {
        if (!point.y.has_value() || !std::isfinite(*point.y))
        {
            previousPixel.reset();
            previousY.reset();
            continue;
        }

        const double y = *point.y;

        if (y < config.yMin || y > config.yMax)
        {
            previousPixel.reset();
            previousY.reset();
            continue;
        }

        const std::size_t column = mapX(point.x, xMin, xMax, layout.plotWidth);
        const std::size_t row = mapY(y, config.yMin, config.yMax, layout.plotHeight);

        if (config.connectPoints && previousPixel.has_value() && previousY.has_value() &&
            std::abs(y - *previousY) <= jumpLimit)
        {
            drawLine(
                canvas,
                previousPixel->first,
                previousPixel->second,
                row,
                column,
                leftMargin,
                config.curveSymbol
            );
        }

        putCharacter(canvas, row, leftMargin + column, config.curveSymbol);
        previousPixel = std::make_pair(row, column);
        previousY = y;
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

PlotResult renderAscii(const SamplePoints& points, const PlotConfig& config)
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

    for (const expressionGraph::SamplePoint& point : points)
    {
        if (!std::isfinite(point.x))
            return makeInvalidArgumentError("Sample point contains a non-finite x value");

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

    const Layout layout = calculateLayout(config, xMin, xMax);

    if (layout.plotWidth < 2 || layout.plotHeight < 2)
    {
        return makeInvalidArgumentError(
            "Plot configuration leaves no room for the graph"
        );
    }

    std::vector<std::string> canvas(
        config.height,
        std::string(config.width, ' ')
    );

    drawAxes(canvas, config, layout);
    drawTicks(canvas, config, layout);
    drawAxisArrows(canvas, config, layout);
    drawLabels(canvas, config, layout);
    drawCurve(canvas, config, layout, points, xMin, xMax);

    return PlotResult::success(canvasToString(canvas));
}

}
