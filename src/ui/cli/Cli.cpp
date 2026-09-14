#include "Cli.h"

#include "AsciiPlot.h"
#include "GraphSampler.h"
#include "Lexer.h"
#include "Parser.h"

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
    double from = -10.0;
    double to = 10.0;
    std::size_t points = 80;

    plot::PlotConfig plotConfig;
};

void printUsage(std::ostream& output, std::string_view programName)
{
    output
        << "Usage: " << programName << " \"expression\" [options]\n\n"
        << "Options:\n"
        << "  --from VALUE       Start of x range (default: -10)\n"
        << "  --to VALUE         End of x range (default: 10)\n"
        << "  --points COUNT     Number of samples (default: 80)\n"
        << "  --width COUNT      Plot width (default: 80)\n"
        << "  --height COUNT     Plot height (default: 24)\n"
        << "  --y-min VALUE      Minimum y value (default: -10)\n"
        << "  --y-max VALUE      Maximum y value (default: 10)\n"
        << "  --no-axes          Do not draw coordinate axes\n"
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

bool parseOptions(int argc, char** argv, Options& options)
{
    if (argc == 2 && std::string_view(argv[1]) == "--help")
    {
        options.helpRequested = true;
        return true;
    }

    if (argc < 2)
    {
        printUsage(std::cerr, argc > 0 ? argv[0] : "Graph");
        return false;
    }

    options.expression = argv[1];

    for (int index = 2; index < argc; ++index)
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

        std::string_view value;

        if (option == "--from")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.from))
            {
                std::cerr << "Invalid value for --from\n";
                return false;
            }
            continue;
        }

        if (option == "--to")
        {
            if (!readOptionValue(argc, argv, index, option, value) ||
                !parseDouble(value, options.to))
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

void printError(const diagnostics::Errors::Error& error)
{
    std::cerr
        << errorDomainToString(error.domain)
        << " error ("
        << errorCodeToString(error.code)
        << "): "
        << error.message;

    if (error.location.has_value())
        std::cerr << " at position " << error.location->position;

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
        printError(tokenizeResult.error());
        return 1;
    }

    auto tokens = std::move(tokenizeResult).value();
    auto parseResult = parser::parse(tokens);

    if (!parseResult)
    {
        printError(parseResult.error());
        return 1;
    }

    auto sampleResult = sampler::sample(
        *parseResult.value(),
        options.from,
        options.to,
        options.points
    );

    if (!sampleResult)
    {
        printError(sampleResult.error());
        return 1;
    }

    auto plotResult = plot::renderAscii(
        sampleResult.value(),
        options.plotConfig
    );

    if (!plotResult)
    {
        printError(plotResult.error());
        return 1;
    }

    std::cout << plotResult.value();
    return 0;
}

}
