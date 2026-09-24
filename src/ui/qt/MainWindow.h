#pragma once

#include "core/ExpressionEngine/Parser/ExpressionNode.h"
#include "common/Diagnostics/Error/Error.h"

#include <QMainWindow>

#include <memory>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace src::ui::qt
{

class PlotWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void buildGraph();
    void handleRangeChanged();
    void handleViewportChanged(double xMin, double xMax, double yMin, double yMax);

private:
    void sampleCurrentExpression();
    void showError(const src::common::Diagnostics::Errors::Error& error);

    QLineEdit* expressionEdit_ = nullptr;
    QDoubleSpinBox* xMinSpin_ = nullptr;
    QDoubleSpinBox* xMaxSpin_ = nullptr;
    QDoubleSpinBox* yMinSpin_ = nullptr;
    QDoubleSpinBox* yMaxSpin_ = nullptr;
    QSpinBox* pointsSpin_ = nullptr;
    QCheckBox* autoYCheck_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    PlotWidget* plotWidget_ = nullptr;
    std::unique_ptr<src::core::ExpressionEngine::Parser::ExpressionTree> tree_;
};

}
