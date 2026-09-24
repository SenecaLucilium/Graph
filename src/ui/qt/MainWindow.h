#pragma once

#include "core/ExpressionEngine/Parser/ExpressionNode.h"
#include "core/ExpressionEngine/Graph/GraphSampler.h"
#include "common/Diagnostics/Error/Error.h"

#include <QMainWindow>

#include <memory>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;

namespace src::ui::qt
{

class PlotWidget;
class AudioPlayer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void buildGraph();
    void startAnimatedBuild();
    void advanceAnimation();
    void toggleMute();
    void handleVolumeChanged(int value);
    void handleAnimationSpeedChanged(int value);
    void handleRangeChanged();
    void handleViewportChanged(double xMin, double xMax, double yMin, double yMax);

private:
    void sampleCurrentExpression();
    void stopAnimation(bool unlockPlot);
    void updateAnimationSound();
    void showError(const src::common::Diagnostics::Errors::Error& error);

    QLineEdit* expressionEdit_ = nullptr;
    QDoubleSpinBox* xMinSpin_ = nullptr;
    QDoubleSpinBox* xMaxSpin_ = nullptr;
    QDoubleSpinBox* yMinSpin_ = nullptr;
    QDoubleSpinBox* yMaxSpin_ = nullptr;
    QSpinBox* pointsSpin_ = nullptr;
    QCheckBox* autoYCheck_ = nullptr;
    QPushButton* buildButton_ = nullptr;
    QPushButton* animatedBuildButton_ = nullptr;
    QPushButton* muteButton_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QSlider* animationSpeedSlider_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    PlotWidget* plotWidget_ = nullptr;
    QTimer* animationTimer_ = nullptr;
    AudioPlayer* audioPlayer_ = nullptr;
    bool audioMuted_ = false;
    std::size_t animationIndex_ = 0;
    std::size_t animationStep_ = 1;
    int animationSpeed_ = 100;
    double soundYMax_ = 1.0;
    src::core::ExpressionEngine::Graph::SamplePoints animationPoints_;
    std::unique_ptr<src::core::ExpressionEngine::Parser::ExpressionTree> tree_;
};

}
