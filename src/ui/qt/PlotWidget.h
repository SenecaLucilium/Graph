#pragma once

#include "core/ExpressionEngine/Graph/GraphSampler.h"

#include <QWidget>

#include <cstddef>

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

namespace src::ui::qt
{

class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr);
    void setSamples(const src::core::ExpressionEngine::Graph::SamplePoints& points);
    void beginAnimation(const src::core::ExpressionEngine::Graph::SamplePoints& points);
    void setAnimationProgress(std::size_t visiblePointCount);
    void setInteractionLocked(bool locked);
    void setViewport(double xMin, double xMax, double yMin, double yMax);

signals:
    void viewportChanged(double xMin, double xMax, double yMin, double yMax);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    src::core::ExpressionEngine::Graph::SamplePoints points_;
    double xMin_ = -10.0;
    double xMax_ = 10.0;
    double yMin_ = -10.0;
    double yMax_ = 10.0;
    QPoint lastMousePosition_;
    bool panning_ = false;
    std::size_t visiblePointCount_ = 0;
    bool animationMode_ = false;
    bool interactionLocked_ = false;
};

}
