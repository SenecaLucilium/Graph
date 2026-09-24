#include "PlotWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <optional>

namespace expressionGraph = src::core::ExpressionEngine::Graph;

namespace src::ui::qt
{
namespace
{

constexpr double leftMargin = 64.0;
constexpr double rightMargin = 18.0;
constexpr double topMargin = 18.0;
constexpr double bottomMargin = 38.0;

QRectF graphRect(const QWidget& widget)
{
    return QRectF(leftMargin, topMargin, std::max(1.0, widget.width() - leftMargin - rightMargin), std::max(1.0, widget.height() - topMargin - bottomMargin));
}

double mapX(double value, double minimum, double maximum, const QRectF& rect)
{
    return rect.left() + (value - minimum) / (maximum - minimum) * rect.width();
}

double mapY(double value, double minimum, double maximum, const QRectF& rect)
{
    return rect.bottom() - (value - minimum) / (maximum - minimum) * rect.height();
}

QString formatCoordinate(double value)
{
    if (std::abs(value) < 1e-10) value = 0.0;
    return QString::number(value, 'g', 6);
}

}

PlotWidget::PlotWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(560, 360);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void PlotWidget::setSamples(const expressionGraph::SamplePoints& points)
{
    points_ = points;
    visiblePointCount_ = points_.size();
    animationMode_ = false;
    update();
}

void PlotWidget::beginAnimation(const expressionGraph::SamplePoints& points)
{
    points_ = points;
    visiblePointCount_ = 0;
    animationMode_ = true;
    setInteractionLocked(true);
    update();
}

void PlotWidget::setAnimationProgress(std::size_t visiblePointCount)
{
    visiblePointCount_ = std::min(visiblePointCount, points_.size());
    update();
}

void PlotWidget::setInteractionLocked(bool locked)
{
    interactionLocked_ = locked;
    panning_ = false;
    if (locked) setCursor(Qt::ArrowCursor);
    else unsetCursor();
}

void PlotWidget::setViewport(double xMin, double xMax, double yMin, double yMax)
{
    if (!std::isfinite(xMin) || !std::isfinite(xMax) || !std::isfinite(yMin) || !std::isfinite(yMax) || xMin >= xMax || yMin >= yMax) return;

    xMin_ = xMin;
    xMax_ = xMax;
    yMin_ = yMin;
    yMax_ = yMax;
    update();
}

void PlotWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), palette().base());

    const QRectF rect = graphRect(*this);
    painter.setPen(QPen(palette().mid(), 1.0));
    painter.drawRect(rect);

    painter.setFont(QFont("Sans", 9));
    const QFontMetrics metrics(painter.font());

    painter.setPen(QPen(palette().midlight(), 1.0, Qt::DashLine));

    for (int index = 0; index <= 10; index++)
    {
        const double fraction = static_cast<double>(index) / 10.0;
        const double x = rect.left() + rect.width() * fraction;
        const double y = rect.top() + rect.height() * fraction;
        painter.drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    painter.setPen(palette().text().color());

    for (int index = 0; index <= 5; index++)
    {
        const double fraction = static_cast<double>(index) / 5.0;
        const double xValue = xMin_ + (xMax_ - xMin_) * fraction;
        const double yValue = yMax_ - (yMax_ - yMin_) * fraction;
        const double x = rect.left() + rect.width() * fraction;
        const double y = rect.top() + rect.height() * fraction;
        const QString xLabel = formatCoordinate(xValue);
        const QString yLabel = formatCoordinate(yValue);
        painter.drawText(QPointF(x - metrics.horizontalAdvance(xLabel) / 2.0, rect.bottom() + metrics.height() + 4.0), xLabel);
        painter.drawText(QPointF(leftMargin - metrics.horizontalAdvance(yLabel) - 8.0, y + metrics.ascent() / 2.0), yLabel);
    }

    painter.setPen(QPen(palette().text().color(), 1.5));

    if (xMin_ <= 0.0 && xMax_ >= 0.0)
    {
        const double axisX = mapX(0.0, xMin_, xMax_, rect);
        painter.drawLine(QPointF(axisX, rect.top()), QPointF(axisX, rect.bottom()));
        painter.drawLine(QPointF(axisX, rect.top()), QPointF(axisX - 5.0, rect.top() + 9.0));
        painter.drawLine(QPointF(axisX, rect.top()), QPointF(axisX + 5.0, rect.top() + 9.0));
    }

    if (yMin_ <= 0.0 && yMax_ >= 0.0)
    {
        const double axisY = mapY(0.0, yMin_, yMax_, rect);
        painter.drawLine(QPointF(rect.left(), axisY), QPointF(rect.right(), axisY));
        painter.drawLine(QPointF(rect.right(), axisY), QPointF(rect.right() - 9.0, axisY - 5.0));
        painter.drawLine(QPointF(rect.right(), axisY), QPointF(rect.right() - 9.0, axisY + 5.0));
    }

    painter.setPen(QPen(QColor(35, 105, 190), 2.0));
    std::optional<QPointF> previousPoint;
    std::optional<double> previousY;
    const double jumpLimit = (yMax_ - yMin_) * 0.75;

    const std::size_t pointCount = animationMode_ ? visiblePointCount_ : points_.size();
    for (std::size_t index = 0; index < pointCount; ++index)
    {
        const expressionGraph::SamplePoint& point = points_[index];
        if (!point.y.has_value() || !std::isfinite(*point.y) || *point.y < yMin_ || *point.y > yMax_)
        {
            previousPoint.reset();
            previousY.reset();
            continue;
        }

        const QPointF currentPoint(mapX(point.x, xMin_, xMax_, rect), mapY(*point.y, yMin_, yMax_, rect));

        if (previousPoint.has_value() && previousY.has_value() && std::abs(*point.y - *previousY) <= jumpLimit)
            painter.drawLine(*previousPoint, currentPoint);

        painter.drawEllipse(currentPoint, 1.8, 1.8);
        previousPoint = currentPoint;
        previousY = point.y;
    }
}

void PlotWidget::wheelEvent(QWheelEvent* event)
{
    if (interactionLocked_)
    {
        event->accept();
        return;
    }

    const QRectF rect = graphRect(*this);
    const QPointF cursor = event->position();
    const double cursorX = xMin_ + (cursor.x() - rect.left()) / rect.width() * (xMax_ - xMin_);
    const double cursorY = yMax_ - (cursor.y() - rect.top()) / rect.height() * (yMax_ - yMin_);
    const double zoom = std::pow(0.85, static_cast<double>(event->angleDelta().y()) / 120.0);
    const double newXMin = cursorX + (xMin_ - cursorX) * zoom;
    const double newXMax = cursorX + (xMax_ - cursorX) * zoom;
    const double newYMin = cursorY + (yMin_ - cursorY) * zoom;
    const double newYMax = cursorY + (yMax_ - cursorY) * zoom;

    setViewport(newXMin, newXMax, newYMin, newYMax);
    emit viewportChanged(xMin_, xMax_, yMin_, yMax_);
    event->accept();
}

void PlotWidget::mousePressEvent(QMouseEvent* event)
{
    if (interactionLocked_)
    {
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    panning_ = true;
    lastMousePosition_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void PlotWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (interactionLocked_)
    {
        event->accept();
        return;
    }

    if (!panning_) return;

    const QRectF rect = graphRect(*this);
    const QPoint delta = event->pos() - lastMousePosition_;
    const double xShift = static_cast<double>(delta.x()) / rect.width() * (xMax_ - xMin_);
    const double yShift = static_cast<double>(delta.y()) / rect.height() * (yMax_ - yMin_);

    setViewport(xMin_ - xShift, xMax_ - xShift, yMin_ + yShift, yMax_ + yShift);
    lastMousePosition_ = event->pos();
    emit viewportChanged(xMin_, xMax_, yMin_, yMax_);
    event->accept();
}

void PlotWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (interactionLocked_)
    {
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    panning_ = false;
    unsetCursor();
    event->accept();
}

}
