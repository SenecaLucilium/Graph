#include "Cli.h"

#include "AsciiPlot.h"
#include "core/ExpressionEngine/ExpressionEngine.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace expressionEngine = src::core::ExpressionEngine;
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
    bool interactive = false;
    bool autoY = false;
    double xMin = -10.0;
    double xMax = 10.0;
    std::size_t points = 80;
    std::string outputPath;

    plot::PlotConfig plotConfig;
};

void printUsage(std::ostream& output, std::string_view programName)
{
    output
        << "Usage: " << programName << " [\"expression\"] [options]\n\n"
        << "The expression can also be read from stdin.\n\n"
        << "Options:\n"
        << "  --x-min VALUE      Minimum x value (default: -10), alias: -f\n"
        << "  --x-max VALUE      Maximum x value (default: 10), alias: -t\n"
        << "  --from VALUE       Alias for --x-min\n"
        << "  --to VALUE         Alias for --x-max\n"
        << "  --points COUNT     Number of samples (default: 80), alias: -p\n"
        << "  --width COUNT      Plot width (default: 80), alias: -W\n"
        << "  --height COUNT     Plot height (default: 24), alias: -H\n"
        << "  --y-min VALUE      Minimum y value (default: -10)\n"
        << "  --y-max VALUE      Maximum y value (default: 10)\n"
        << "  --auto-y           Choose y range from finite sample values, alias: -a\n"
        << "  --no-axes          Do not draw coordinate axes, alias: -n\n"
        << "  --no-labels        Do not draw numeric axis labels, alias: -l\n"
        << "  --no-connect       Draw points without connecting segments\n"
        << "  --jump-factor VALUE Discontinuity jump sensitivity (default: 0.75)\n"
        << "  --curve-symbol C   Curve character (default: *)\n"
        << "  --x-axis-symbol C  X-axis character (default: -)\n"
        << "  --y-axis-symbol C  Y-axis character (default: |)\n"
        << "  --origin-symbol C  Axis intersection character (default: +)\n"
        << "  --output FILE      Write the graph to a file, alias: -o\n"
        << "  --interactive      Read and render expressions until EOF, alias: -i\n"
        << "  --help             Show this help, alias: -h\n";
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

bool parseCharacter(std::string_view text, char& value)
{
    if (text.size() != 1) return false;

    value = text.front();
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

bool trimExpression(std::string& expression)
{
    const auto first = expression.find_first_not_of(" \t\r\n");
    const auto last = expression.find_last_not_of(" \t\r\n");

    if (first == std::string::npos) return false;

    expression = expression.substr(first, last - first + 1);
    return true;
}

bool readExpressionFromStdin(std::string& expression)
{
    if (!std::getline(std::cin, expression)) return false;

    return trimExpression(expression);
}

bool isOptionToken(std::string_view token)
{
    return token.starts_with("--") || token == "-h" || token == "-f" || token == "-t" || token == "-p" || token == "-W" || token == "-H" || token == "-a" || token == "-n" || token == "-l" || token == "-o" || token == "-i";
}

bool parseOptions(int argc, char** argv, Options& options)
{
    int firstOptionIndex = 1;

    if (argc > 1 && !isOptionToken(argv[1]))
    {
        options.expression = argv[1];
        firstOptionIndex = 2;
    }

    for (int index = firstOptionIndex; index < argc; ++index)
    {
        const std::string_view option = argv[index];

        if (option == "--help" || option == "-h")
        {
            options.helpRequested = true;
            continue;
        }

        if (option == "--interactive" || option == "-i")
        {
            options.interactive = true;
            continue;
        }

        if (option == "--no-axes" || option == "-n")
        {
            options.plotConfig.drawAxes = false;
            continue;
        }

        if (option == "--no-labels" || option == "-l")
        {
            options.plotConfig.drawLabels = false;
            continue;
        }

        if (option == "--auto-y" || option == "-a")
        {
            options.autoY = true;
            continue;
        }

        std::string_view value;

        if (option == "--x-min" || option == "--from" || option == "-f")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.xMin))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--x-max" || option == "--to" || option == "-t")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.xMax))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--points" || option == "-p")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseSize(value, options.points))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--width" || option == "-W")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseSize(value, options.plotConfig.width))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--height" || option == "-H")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseSize(value, options.plotConfig.height))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--jump-factor")
        {
            if (!readOptionValue(argc, argv, index, option, value) || !parseDouble(value, options.plotConfig.discontinuityJumpFactor))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--no-connect")
        {
            options.plotConfig.connectPoints = false;
            continue;
        }

        if (option == "--curve-symbol" || option == "--x-axis-symbol" || option == "--y-axis-symbol" || option == "--origin-symbol")
        {
            char symbol = '\0';

            if (!readOptionValue(argc, argv, index, option, value) || !parseCharacter(value, symbol))
            {
                std::cerr << "Invalid value for " << option << ": expected one character\n";
                return false;
            }

            if (option == "--curve-symbol") options.plotConfig.curveSymbol = symbol;
            else if (option == "--x-axis-symbol") options.plotConfig.xAxisSymbol = symbol;
            else if (option == "--y-axis-symbol") options.plotConfig.yAxisSymbol = symbol;
            else options.plotConfig.originSymbol = symbol;

            continue;
        }

        if (option == "--output" || option == "-o")
        {
            if (!readOptionValue(argc, argv, index, option, value) || value.empty())
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }

            options.outputPath = value;
            continue;
        }

        if (option == "--y-min")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.plotConfig.yMin))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        if (option == "--y-max")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.plotConfig.yMax))
            {
                std::cerr << "Invalid value for " << option << '\n';
                return false;
            }
            continue;
        }

        std::cerr << "Unknown option: " << option << '\n';
        return false;
    }

    if (!options.helpRequested && !options.interactive && options.expression.empty() && !readExpressionFromStdin(options.expression))
    {
        printUsage(std::cerr, argc > 0 ? argv[0] : "Graph");
        return false;
    }

    return true;
}

bool validateOptions(const Options& options)
{
    if (options.xMin >= options.xMax)
    {
        std::cerr << "Invalid x range: --x-min must be less than --x-max\n";
        return false;
    }

    if (options.points < 2 || options.points > 4096)
    {
        std::cerr << "Invalid --points: expected a value from 2 to 4096\n";
        return false;
    }

    if (options.plotConfig.width < 20 || options.plotConfig.width > 240)
    {
        std::cerr << "Invalid --width: expected a value from 20 to 240\n";
        return false;
    }

    if (options.plotConfig.height < 5 || options.plotConfig.height > 100)
    {
        std::cerr << "Invalid --height: expected a value from 5 to 100\n";
        return false;
    }

    if (!std::isfinite(options.plotConfig.discontinuityJumpFactor) || options.plotConfig.discontinuityJumpFactor <= 0.0)
    {
        std::cerr << "Invalid --jump-factor: expected a positive finite value\n";
        return false;
    }

    if (!options.autoY && options.plotConfig.yMin >= options.plotConfig.yMax)
    {
        std::cerr << "Invalid y range: --y-min must be less than --y-max\n";
        return false;
    }

    if (options.interactive && !options.outputPath.empty())
    {
        std::cerr << "Invalid options: --interactive cannot be combined with --output\n";
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

diagnostics::Errors::Error makeCliError(diagnostics::ErrorDomain domain, diagnostics::ErrorCode code, std::string message)
{
    diagnostics::Errors::Error error;
    error.domain = domain;
    error.code = code;
    error.errorLevel = diagnostics::ErrorLevel::Error;
    error.message = std::move(message);
    return error;
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

int renderExpression(const Options& options)
{
    plot::PlotConfig plotConfig = options.plotConfig;
    auto compileResult = expressionEngine::Engine::compile(options.expression);

    if (!compileResult)
    {
        printError(compileResult.error(), options.expression);
        return 1;
    }

    auto sampleResult = expressionEngine::Engine::sample(*compileResult.value(), options.xMin, options.xMax, options.points);

    if (!sampleResult)
    {
        printError(sampleResult.error(), options.expression);
        return 1;
    }

    if (options.autoY && !chooseAutomaticYRange(sampleResult.value(), plotConfig))
    {
        printError(makeCliError(diagnostics::ErrorDomain::UI, diagnostics::ErrorCode::InvalidArgument, "Cannot determine automatic y range: no finite sample values"), options.expression);
        return 1;
    }

    auto plotResult = plot::renderAscii(sampleResult.value(), plotConfig);

    if (!plotResult)
    {
        printError(plotResult.error(), options.expression);
        return 1;
    }

    if (options.outputPath.empty())
    {
        std::cout << plotResult.value();
        return 0;
    }

    std::ofstream output(options.outputPath);

    if (!output)
    {
        printError(makeCliError(diagnostics::ErrorDomain::IO, diagnostics::ErrorCode::InvalidArgument, "Cannot open output file: " + options.outputPath), options.expression);
        return 1;
    }

    output << plotResult.value();

    if (!output)
    {
        printError(makeCliError(diagnostics::ErrorDomain::IO, diagnostics::ErrorCode::InvalidArgument, "Cannot write output file: " + options.outputPath), options.expression);
        return 1;
    }

    return 0;
}

}

int run(int argc, char** argv)
{
    Options options;

    if (!parseOptions(argc, argv, options))
    {
        return 2;
    }

    if (!options.helpRequested && !validateOptions(options))
    {
        return 2;
    }

    if (options.helpRequested)
    {
        printUsage(std::cout, argv[0]);
        return 0;
    }

    if (!options.interactive) return renderExpression(options);

    std::string expression;

    while (std::getline(std::cin, expression))
    {
        if (!trimExpression(expression)) continue;

        Options currentOptions = options;
        currentOptions.expression = expression;
        const int result = renderExpression(currentOptions);

        if (result != 0) return result;

        std::cout << '\n';
    }

    return 0;
}

}
