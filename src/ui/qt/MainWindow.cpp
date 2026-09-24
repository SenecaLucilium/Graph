#include "MainWindow.h"

#include "PlotWidget.h"
#include "core/ExpressionEngine/ExpressionEngine.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace expressionEngine = src::core::ExpressionEngine;
namespace expressionGraph = src::core::ExpressionEngine::Graph;

namespace src::ui::qt
{
namespace
{

QDoubleSpinBox* createRangeSpinBox(QWidget* parent, double value)
{
    auto* spinBox = new QDoubleSpinBox(parent);
    spinBox->setRange(-1e9, 1e9);
    spinBox->setDecimals(6);
    spinBox->setSingleStep(1.0);
    spinBox->setValue(value);
    return spinBox;
}

bool chooseAutomaticYRange(const expressionGraph::SamplePoints& points, double& minimum, double& maximum)
{
    double finiteMinimum = std::numeric_limits<double>::infinity();
    double finiteMaximum = -std::numeric_limits<double>::infinity();

    for (const expressionGraph::SamplePoint& point : points)
    {
        if (!point.y.has_value() || !std::isfinite(*point.y)) continue;

        finiteMinimum = std::min(finiteMinimum, *point.y);
        finiteMaximum = std::max(finiteMaximum, *point.y);
    }

    if (!std::isfinite(finiteMinimum) || !std::isfinite(finiteMaximum)) return false;

    const double range = finiteMaximum - finiteMinimum;
    const double padding = range > 0.0 ? range * 0.1 : std::max(1.0, std::abs(finiteMinimum) * 0.1);

    if (!std::isfinite(padding)) return false;

    minimum = finiteMinimum - padding;
    maximum = finiteMaximum + padding;
    return std::isfinite(minimum) && std::isfinite(maximum) && minimum < maximum;
}

}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Graph");
    resize(1100, 760);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    auto* expressionLayout = new QHBoxLayout();
    expressionEdit_ = new QLineEdit("sin(x)", centralWidget);
    auto* buildButton = new QPushButton("Build graph", centralWidget);
    expressionLayout->addWidget(new QLabel("Expression:", centralWidget));
    expressionLayout->addWidget(expressionEdit_, 1);
    expressionLayout->addWidget(buildButton);

    auto* settingsBox = new QGroupBox("Graph settings", centralWidget);
    auto* settingsLayout = new QGridLayout(settingsBox);
    xMinSpin_ = createRangeSpinBox(settingsBox, -10.0);
    xMaxSpin_ = createRangeSpinBox(settingsBox, 10.0);
    yMinSpin_ = createRangeSpinBox(settingsBox, -10.0);
    yMaxSpin_ = createRangeSpinBox(settingsBox, 10.0);
    pointsSpin_ = new QSpinBox(settingsBox);
    pointsSpin_->setRange(2, 4096);
    pointsSpin_->setValue(120);
    autoYCheck_ = new QCheckBox("Automatic Y range", settingsBox);
    settingsLayout->addWidget(new QLabel("X min", settingsBox), 0, 0);
    settingsLayout->addWidget(xMinSpin_, 0, 1);
    settingsLayout->addWidget(new QLabel("X max", settingsBox), 0, 2);
    settingsLayout->addWidget(xMaxSpin_, 0, 3);
    settingsLayout->addWidget(new QLabel("Y min", settingsBox), 1, 0);
    settingsLayout->addWidget(yMinSpin_, 1, 1);
    settingsLayout->addWidget(new QLabel("Y max", settingsBox), 1, 2);
    settingsLayout->addWidget(yMaxSpin_, 1, 3);
    settingsLayout->addWidget(new QLabel("Samples", settingsBox), 2, 0);
    settingsLayout->addWidget(pointsSpin_, 2, 1);
    settingsLayout->addWidget(autoYCheck_, 2, 2, 1, 2);

    plotWidget_ = new PlotWidget(centralWidget);
    statusLabel_ = new QLabel("Enter an expression and press Build graph.", centralWidget);
    statusLabel_->setWordWrap(true);
    mainLayout->addLayout(expressionLayout);
    mainLayout->addWidget(settingsBox);
    mainLayout->addWidget(plotWidget_, 1);
    mainLayout->addWidget(statusLabel_);
    setCentralWidget(centralWidget);

    connect(buildButton, &QPushButton::clicked, this, &MainWindow::buildGraph);
    connect(expressionEdit_, &QLineEdit::returnPressed, this, &MainWindow::buildGraph);
    connect(autoYCheck_, &QCheckBox::toggled, this, &MainWindow::handleRangeChanged);
    connect(pointsSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::handleRangeChanged);
    connect(xMinSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::handleRangeChanged);
    connect(xMaxSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::handleRangeChanged);
    connect(yMinSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::handleRangeChanged);
    connect(yMaxSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::handleRangeChanged);
    connect(plotWidget_, &PlotWidget::viewportChanged, this, &MainWindow::handleViewportChanged);
    yMinSpin_->setEnabled(true);
    yMaxSpin_->setEnabled(true);
}

void MainWindow::buildGraph()
{
    auto compileResult = expressionEngine::Engine::compile(expressionEdit_->text().toStdString());

    if (!compileResult)
    {
        showError(compileResult.error());
        return;
    }

    tree_ = std::move(compileResult).value();
    sampleCurrentExpression();
}

void MainWindow::handleRangeChanged()
{
    if (!tree_) return;
    if (xMinSpin_->value() >= xMaxSpin_->value()) return;

    plotWidget_->setViewport(xMinSpin_->value(), xMaxSpin_->value(), yMinSpin_->value(), yMaxSpin_->value());
    sampleCurrentExpression();
}

void MainWindow::handleViewportChanged(double xMin, double xMax, double yMin, double yMax)
{
    const QSignalBlocker xMinBlocker(xMinSpin_);
    const QSignalBlocker xMaxBlocker(xMaxSpin_);
    const QSignalBlocker yMinBlocker(yMinSpin_);
    const QSignalBlocker yMaxBlocker(yMaxSpin_);
    xMinSpin_->setValue(xMin);
    xMaxSpin_->setValue(xMax);
    yMinSpin_->setValue(yMin);
    yMaxSpin_->setValue(yMax);
    sampleCurrentExpression();
}

void MainWindow::sampleCurrentExpression()
{
    if (!tree_) return;
    if (xMinSpin_->value() >= xMaxSpin_->value()) return;

    auto sampleResult = expressionEngine::Engine::sample(*tree_, xMinSpin_->value(), xMaxSpin_->value(), static_cast<std::size_t>(pointsSpin_->value()));

    if (!sampleResult)
    {
        showError(sampleResult.error());
        return;
    }

    auto points = std::move(sampleResult).value();
    double yMin = yMinSpin_->value();
    double yMax = yMaxSpin_->value();

    if (autoYCheck_->isChecked() && !chooseAutomaticYRange(points, yMin, yMax))
    {
        statusLabel_->setText("Evaluation error: no finite values are available for automatic Y range.");
        return;
    }

    if (autoYCheck_->isChecked())
    {
        const QSignalBlocker yMinBlocker(yMinSpin_);
        const QSignalBlocker yMaxBlocker(yMaxSpin_);
        yMinSpin_->setValue(yMin);
        yMaxSpin_->setValue(yMax);
    }

    plotWidget_->setViewport(xMinSpin_->value(), xMaxSpin_->value(), yMin, yMax);
    plotWidget_->setSamples(points);
    statusLabel_->setText(QString("Rendered %1 samples. Mouse wheel: zoom, left mouse button: pan.").arg(points.size()));
}

void MainWindow::showError(const src::common::Diagnostics::Errors::Error& error)
{
    statusLabel_->setText(QString("Error (%1): %2").arg(static_cast<int>(error.code)).arg(QString::fromStdString(error.message)));
}

}
