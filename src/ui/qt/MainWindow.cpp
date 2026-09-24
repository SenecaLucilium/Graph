#include "MainWindow.h"

#include "AudioPlayer.h"
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
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
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

int animationIntervalForSpeed(int speed)
{
    const double normalizedSpeed = std::clamp(static_cast<double>(speed - 1) / 99.0, 0.0, 1.0);
    const double slowDown = 1.0 - normalizedSpeed;
    return 20 + static_cast<int>(980.0 * slowDown * slowDown);
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
    buildButton_ = new QPushButton("Build graph", centralWidget);
    animatedBuildButton_ = new QPushButton("Build animation + sound", centralWidget);
    expressionLayout->addWidget(new QLabel("Expression:", centralWidget));
    expressionLayout->addWidget(expressionEdit_, 1);
    expressionLayout->addWidget(buildButton_);
    expressionLayout->addWidget(animatedBuildButton_);

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

    auto* audioLayout = new QHBoxLayout();
    muteButton_ = new QPushButton("Mute", centralWidget);
    volumeSlider_ = new QSlider(Qt::Horizontal, centralWidget);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(60);
    audioLayout->addWidget(new QLabel("Animation sound:", centralWidget));
    audioLayout->addWidget(muteButton_);
    audioLayout->addWidget(new QLabel("Volume", centralWidget));
    audioLayout->addWidget(volumeSlider_, 1);

    auto* animationLayout = new QHBoxLayout();
    animationSpeedSlider_ = new QSlider(Qt::Horizontal, centralWidget);
    animationSpeedSlider_->setRange(1, 100);
    animationSpeedSlider_->setValue(animationSpeed_);
    animationLayout->addWidget(new QLabel("Animation speed:", centralWidget));
    animationLayout->addWidget(animationSpeedSlider_, 1);
    animationLayout->addWidget(new QLabel("slow", centralWidget));
    animationLayout->addWidget(new QLabel("fast", centralWidget));

    plotWidget_ = new PlotWidget(centralWidget);
    statusLabel_ = new QLabel("Enter an expression and press Build graph.", centralWidget);
    statusLabel_->setWordWrap(true);
    mainLayout->addLayout(expressionLayout);
    mainLayout->addWidget(settingsBox);
    mainLayout->addLayout(audioLayout);
    mainLayout->addLayout(animationLayout);
    mainLayout->addWidget(plotWidget_, 1);
    mainLayout->addWidget(statusLabel_);
    setCentralWidget(centralWidget);

    audioPlayer_ = new AudioPlayer(this);
    animationTimer_ = new QTimer(this);
    animationTimer_->setInterval(animationIntervalForSpeed(animationSpeed_));
    audioPlayer_->setVolume(0.6);
    connect(buildButton_, &QPushButton::clicked, this, &MainWindow::buildGraph);
    connect(animatedBuildButton_, &QPushButton::clicked, this, &MainWindow::startAnimatedBuild);
    connect(expressionEdit_, &QLineEdit::returnPressed, this, &MainWindow::buildGraph);
    connect(animationTimer_, &QTimer::timeout, this, &MainWindow::advanceAnimation);
    connect(muteButton_, &QPushButton::clicked, this, &MainWindow::toggleMute);
    connect(volumeSlider_, &QSlider::valueChanged, this, &MainWindow::handleVolumeChanged);
    connect(animationSpeedSlider_, &QSlider::valueChanged, this, &MainWindow::handleAnimationSpeedChanged);
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
    stopAnimation(true);
    auto compileResult = expressionEngine::Engine::compile(expressionEdit_->text().toStdString());

    if (!compileResult)
    {
        showError(compileResult.error());
        return;
    }

    tree_ = std::move(compileResult).value();
    sampleCurrentExpression();
}

void MainWindow::startAnimatedBuild()
{
    stopAnimation(false);

    auto compileResult = expressionEngine::Engine::compile(expressionEdit_->text().toStdString());
    if (!compileResult)
    {
        showError(compileResult.error());
        return;
    }

    tree_ = std::move(compileResult).value();
    if (xMinSpin_->value() >= xMaxSpin_->value())
    {
        statusLabel_->setText("X min must be less than X max.");
        return;
    }

    auto sampleResult = expressionEngine::Engine::sample(*tree_, xMinSpin_->value(), xMaxSpin_->value(), static_cast<std::size_t>(pointsSpin_->value()));
    if (!sampleResult)
    {
        showError(sampleResult.error());
        return;
    }

    animationPoints_ = std::move(sampleResult).value();
    soundYMax_ = 0.0;
    for (const expressionGraph::SamplePoint& point : animationPoints_)
    {
        if (!point.y.has_value() || !std::isfinite(*point.y)) continue;
        soundYMax_ = std::max(soundYMax_, std::abs(*point.y));
    }

    if (!std::isfinite(soundYMax_) || soundYMax_ <= 0.0)
    {
        soundYMax_ = 1.0;
    }

    double yMin = yMinSpin_->value();
    double yMax = yMaxSpin_->value();
    if (autoYCheck_->isChecked() && !chooseAutomaticYRange(animationPoints_, yMin, yMax))
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
    plotWidget_->beginAnimation(animationPoints_);
    animationIndex_ = 0;
    animationStep_ = std::max<std::size_t>(1, animationPoints_.size() / 200);
    audioPlayer_->start();
    animationTimer_->start();
    const QString soundStatus = audioPlayer_->isAvailable() ? "Sound is enabled." : "Qt Multimedia is unavailable; animation is silent.";
    statusLabel_->setText(QString("Animation is building %1 samples from X min to X max. %2").arg(animationPoints_.size()).arg(soundStatus));
}

void MainWindow::advanceAnimation()
{
    if (animationIndex_ >= animationPoints_.size())
    {
        stopAnimation(false);
        plotWidget_->setAnimationProgress(animationPoints_.size());
        plotWidget_->setInteractionLocked(true);
        statusLabel_->setText("Animation complete. The graph is locked; press Build graph for an interactive plot.");
        return;
    }

    animationIndex_ = std::min(animationPoints_.size(), animationIndex_ + animationStep_);
    plotWidget_->setAnimationProgress(animationIndex_);
    updateAnimationSound();
}

void MainWindow::toggleMute()
{
    audioMuted_ = !audioMuted_;
    audioPlayer_->setMuted(audioMuted_);
    muteButton_->setText(audioMuted_ ? "Unmute" : "Mute");
}

void MainWindow::handleVolumeChanged(int value)
{
    audioPlayer_->setVolume(static_cast<double>(value) / 100.0);
}

void MainWindow::handleAnimationSpeedChanged(int value)
{
    animationSpeed_ = std::clamp(value, 1, 100);
    animationTimer_->setInterval(animationIntervalForSpeed(animationSpeed_));
}

void MainWindow::handleRangeChanged()
{
    if (animationTimer_->isActive()) return;
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

void MainWindow::stopAnimation(bool unlockPlot)
{
    if (animationTimer_ != nullptr) animationTimer_->stop();
    if (audioPlayer_ != nullptr) audioPlayer_->stop();
    if (unlockPlot && plotWidget_ != nullptr) plotWidget_->setInteractionLocked(false);
}

void MainWindow::updateAnimationSound()
{
    if (animationIndex_ == 0 || animationIndex_ > animationPoints_.size()) return;

    const expressionGraph::SamplePoint& current = animationPoints_[animationIndex_ - 1];
    if (!current.y.has_value() || !std::isfinite(*current.y))
    {
        audioPlayer_->setPitch(120.0);
        return;
    }

    const double normalizedY = std::clamp(std::abs(*current.y) / soundYMax_, 0.0, 1.0);
    const double pitch = 110.0 * std::pow(2.0, normalizedY * 3.7);
    audioPlayer_->setPitch(pitch);
}

void MainWindow::showError(const src::common::Diagnostics::Errors::Error& error)
{
    statusLabel_->setText(QString("Error (%1): %2").arg(static_cast<int>(error.code)).arg(QString::fromStdString(error.message)));
}

}
