#include "Cli.h"

#include "AsciiPlot.h"
#include "GraphSampler.h"
#include "Lexer.h"
#include "Parser.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace lexer = src::core::ExpressionEngine::Lexer;
namespace parser = src::core::ExpressionEngine::Parser;
namespace sampler = src::core::ExpressionEngine::Graph;
namespace plot = src::core::GraphEngine::AsciiPlot;
namespace diagnostics = src::common::Diagnostics;

namespace src::ui::cli
{
namespace
{

struct Options
{
    std::string expression;
    bool helpRequested = false;
    bool autoY = false;
    double xMin = -10.0;
    double xMax = 10.0;
    std::size_t points = 80;

    plot::PlotConfig plotConfig;
};

void printUsage(std::ostream& output, std::string_view programName)
{
    output
        << "Usage: " << programName << " [\"expression\"] [options]\n\n"
        << "The expression can also be read from stdin.\n\n"
        << "Options:\n"
        << "  --x-min VALUE      Minimum x value (default: -10)\n"
        << "  --x-max VALUE      Maximum x value (default: 10)\n"
        << "  --from VALUE       Alias for --x-min\n"
        << "  --to VALUE         Alias for --x-max\n"
        << "  --points COUNT     Number of samples (default: 80)\n"
        << "  --width COUNT      Plot width (default: 80)\n"
        << "  --height COUNT     Plot height (default: 24)\n"
        << "  --y-min VALUE      Minimum y value (default: -10)\n"
        << "  --y-max VALUE      Maximum y value (default: 10)\n"
        << "  --auto-y           Choose y range from finite sample values\n"
        << "  --no-axes          Do not draw coordinate axes\n"
        << "  --no-labels        Do not draw numeric axis labels\n"
        << "  --help             Show this help\n";
}

bool parseDouble(std::string_view text, double& value)
{
    std::string buffer(text);
    std::size_t parsedCharacters = 0;

    try
    {
        value = std::stod(buffer, &parsedCharacters);
    }
    catch (const std::exception&)
    {
        return false;
    }

    return parsedCharacters == buffer.size() && std::isfinite(value);
}

bool parseSize(std::string_view text, std::size_t& value)
{
    if (text.empty()) return false;

    std::size_t parsedValue = 0;
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();

    const auto result = std::from_chars(begin, end, parsedValue);

    if (result.ec != std::errc{} || result.ptr != end)
        return false;

    value = parsedValue;
    return true;
}

bool readOptionValue(
    int argc,
    char** argv,
    int& index,
    std::string_view option,
    std::string_view& value
)
{
    if (index + 1 >= argc)
    {
        std::cerr << "Missing value for option " << option << '\n';
        return false;
    }

    ++index;
    value = argv[index];
    return true;
}

bool readExpressionFromStdin(std::string& expression)
{
    if (!std::getline(std::cin, expression)) return false;

    const auto first = expression.find_first_not_of(" \t\r\n");
    const auto last = expression.find_last_not_of(" \t\r\n");

    if (first == std::string::npos) return false;

    expression = expression.substr(first, last - first + 1);
    return true;
}

bool parseOptions(int argc, char** argv, Options& options)
{
    int firstOptionIndex = 1;

    if (argc > 1 && !std::string_view(argv[1]).starts_with("--"))
    {
        options.expression = argv[1];
        firstOptionIndex = 2;
    }

    for (int index = firstOptionIndex; index < argc; ++index)
    {
        const std::string_view option = argv[index];

        if (option == "--help")
        {
            options.helpRequested = true;
            continue;
        }

        if (option == "--no-axes")
        {
            options.plotConfig.drawAxes = false;
            continue;
        }

        if (option == "--no-labels")
        {
            options.plotConfig.drawLabels = false;
            continue;
        }

        if (option == "--auto-y")
        {
            options.autoY = true;
            continue;
        }

        std::string_view value;

        if (option == "--x-min" || option == "--from")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.xMin))
            {
                std::cerr << "Invalid value for --from\n";
                return false;
            }
            continue;
        }

        if (option == "--x-max" || option == "--to")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.xMax))
            {
                std::cerr << "Invalid value for --to\n";
                return false;
            }
            continue;
        }

        if (option == "--points")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseSize(value, options.points))
            {
                std::cerr << "Invalid value for --points\n";
                return false;
            }
            continue;
        }

        if (option == "--width")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseSize(value, options.plotConfig.width))
            {
                std::cerr << "Invalid value for --width\n";
                return false;
            }
            continue;
        }

        if (option == "--height")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseSize(value, options.plotConfig.height))
            {
                std::cerr << "Invalid value for --height\n";
                return false;
            }
            continue;
        }

        if (option == "--y-min")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.plotConfig.yMin))
            {
                std::cerr << "Invalid value for --y-min\n";
                return false;
            }
            continue;
        }

        if (option == "--y-max")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.plotConfig.yMax))
            {
                std::cerr << "Invalid value for --y-max\n";
                return false;
            }
            continue;
        }

        std::cerr << "Unknown option: " << option << '\n';
        return false;
    }

    if (!options.helpRequested && options.expression.empty() && !readExpressionFromStdin(options.expression))
    {
        printUsage(std::cerr, argc > 0 ? argv[0] : "Graph");
        return false;
    }

    return true;
}

const char* errorDomainToString(diagnostics::ErrorDomain domain)
{
    switch (domain)
    {
    case diagnostics::ErrorDomain::Lexer: return "Lexer";
    case diagnostics::ErrorDomain::Parser: return "Parser";
    case diagnostics::ErrorDomain::Evaluation: return "Evaluation";
    case diagnostics::ErrorDomain::IO: return "IO";
    case diagnostics::ErrorDomain::UI: return "UI";
    case diagnostics::ErrorDomain::Internal: return "Internal";
    }

    return "Unknown";
}

const char* errorCodeToString(diagnostics::ErrorCode code)
{
    switch (code)
    {
    case diagnostics::ErrorCode::Unknown: return "Unknown";
    case diagnostics::ErrorCode::UnexpectedCharacter: return "UnexpectedCharacter";
    case diagnostics::ErrorCode::InvalidNumber: return "InvalidNumber";
    case diagnostics::ErrorCode::InvalidIdentifier: return "InvalidIdentifier";
    case diagnostics::ErrorCode::UnexpectedToken: return "UnexpectedToken";
    case diagnostics::ErrorCode::ExpectedExpression: return "ExpectedExpression";
    case diagnostics::ErrorCode::MissingRightParenthesis: return "MissingRightParenthesis";
    case diagnostics::ErrorCode::MissingFunctionParenthesis: return "MissingFunctionParenthesis";
    case diagnostics::ErrorCode::UnexpectedEndOfExpression: return "UnexpectedEndOfExpression";
    case diagnostics::ErrorCode::TrailingTokens: return "TrailingTokens";
    case diagnostics::ErrorCode::DivisionByZero: return "DivisionByZero";
    case diagnostics::ErrorCode::InvalidArgument: return "InvalidArgument";
    case diagnostics::ErrorCode::NonFiniteResult: return "NonFiniteResult";
    }

    return "Unknown";
}

bool chooseAutomaticYRange(const sampler::SamplePoints& points, plot::PlotConfig& config)
{
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();

    for (const sampler::SamplePoint& point : points)
    {
        if (!point.y.has_value() || !std::isfinite(*point.y)) continue;

        minimum = std::min(minimum, *point.y);
        maximum = std::max(maximum, *point.y);
    }

    if (!std::isfinite(minimum) || !std::isfinite(maximum)) return false;

    const double range = maximum - minimum;
    const double padding = range > 0.0 ? range * 0.1 : std::max(1.0, std::abs(minimum) * 0.1);

    if (!std::isfinite(padding)) return false;

    config.yMin = minimum - padding;
    config.yMax = maximum + padding;

    return std::isfinite(config.yMin) && std::isfinite(config.yMax) && config.yMin < config.yMax;
}

void printError(const diagnostics::Errors::Error& error, std::string_view expression)
{
    std::cerr
        << errorDomainToString(error.domain)
        << " error ("
        << errorCodeToString(error.code)
        << "): "
        << error.message;

    if (error.location.has_value())
        std::cerr << " at position " << error.location->position;

    if (!expression.empty())
        std::cerr << "\nExpression: " << expression;

    std::cerr << '\n';
}

}

int run(int argc, char** argv)
{
    Options options;

    if (!parseOptions(argc, argv, options))
    {
        return 2;
    }

    if (options.helpRequested)
    {
        printUsage(std::cout, argv[0]);
        return 0;
    }

    lexer::Lexer lexer(options.expression);
    auto tokenizeResult = lexer.tokenize();

    if (!tokenizeResult)
    {
        printError(tokenizeResult.error(), options.expression);
        return 1;
    }

    auto tokens = std::move(tokenizeResult).value();
    auto parseResult = parser::parse(tokens);

    if (!parseResult)
    {
        printError(parseResult.error(), options.expression);
        return 1;
    }

    auto sampleResult = sampler::sample(
        *parseResult.value(),
        options.xMin,
        options.xMax,
        options.points
    );

    if (!sampleResult)
    {
        printError(sampleResult.error(), options.expression);
        return 1;
    }

    if (options.autoY && !chooseAutomaticYRange(sampleResult.value(), options.plotConfig))
    {
        diagnostics::Errors::Error error;
        error.domain = diagnostics::ErrorDomain::UI;
        error.code = diagnostics::ErrorCode::InvalidArgument;
        error.errorLevel = diagnostics::ErrorLevel::Error;
        error.message = "Cannot determine automatic y range: no finite sample values";
        printError(error, options.expression);
        return 1;
    }

    auto plotResult = plot::renderAscii(
        sampleResult.value(),
        options.plotConfig
    );

    if (!plotResult)
    {
        printError(plotResult.error(), options.expression);
        return 1;
    }

    std::cout << plotResult.value();
    return 0;
}

}
