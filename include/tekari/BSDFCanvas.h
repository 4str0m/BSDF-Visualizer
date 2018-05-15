#pragma once

#include <nanogui/glcanvas.h>
#include <nanogui/widget.h>
#include <nanogui/label.h>
#include <nanogui/window.h>
#include <memory>
#include "common.h"
#include "Axis.h"
#include "RadialGrid.h"
#include "SelectionBox.h"
#include "DataSample.h"

TEKARI_NAMESPACE_BEGIN

class BSDFCanvas : public nanogui::GLCanvas
{
public:
    // usefull types
    enum ViewAngles
    {
        FRONT, BACK, UP, DOWN, LEFT, RIGHT
    };
    enum MouseMode
    {
        ROTATE,
        TRANSLATE,
        SELECTION,
        MOUSE_MODE_COUNT
    };

private:
    // view state constants
    static const nanogui::Vector3f VIEW_ORIGIN;
    static const nanogui::Vector3f VIEW_UP;
    static const nanogui::Vector3f VIEW_RIGHT;
    static const nanogui::Matrix4f VIEW;

public:
    BSDFCanvas(Widget *parent);

    // nanogui specific methods
    virtual bool mouseMotionEvent(const nanogui::Vector2i &p,
                                  const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
    virtual void performLayout(NVGcontext*) override { m_Arcball.setSize(mSize); }
    virtual void draw(NVGcontext* ctx) override;
    virtual void drawGL() override;

    // data sample addition/removale/selection
    void selectDataSample(std::shared_ptr<DataSample> dataSample);
    void addDataSample(std::shared_ptr<DataSample> dataSample);
    void removeDataSample(std::shared_ptr<DataSample> dataSample);

    void snapToSelectionCenter();


    void setOrthoMode(bool orthoMode) { m_OrthoMode = orthoMode; }
    void setViewAngle(ViewAngles viewAngle);
    void setSelectionCallback(std::function<void(const nanogui::Matrix4f&, const SelectionBox&,
        const nanogui::Vector2i&, SelectionMode)> callback) { m_SelectCallback = callback; }

    // Setters/Getters
    const RadialGrid& grid() const { return m_Grid; }
    RadialGrid& grid() { return m_Grid; }

    int drawFlags() const { return m_DrawFlags; }
    void setDrawFlags(int flags) { m_DrawFlags = flags; }
    void setDrawFlag(int flag, bool state) { m_DrawFlags = state ? m_DrawFlags | flag : m_DrawFlags & ~flag; }

    void setColorMap(std::shared_ptr<ColorMap> colorMap) { m_ColorMap = colorMap; }
    const std::shared_ptr<const ColorMap> colorMap() const { return m_ColorMap; }

    void setPointSizeScale(float pointSizeScale) { m_PointSizeScale = pointSizeScale; }

    void setMouseMode(MouseMode mode) { m_MouseMode = mode; }
    MouseMode mouseMode() const { return m_MouseMode; }

private:
    SelectionBox getSelectionBox() const;
    nanogui::Matrix4f getProjectionMatrix() const;
    int rotationMouseButton(bool dragging) const;
    int translationMouseButton(bool dragging) const;
    int selectionMouseButton(bool dragging) const;

    // data samples
    std::vector<std::shared_ptr<DataSample>>    m_DataSamplesToDraw;
    std::shared_ptr<DataSample>                 m_SelectedDataSample;

    RadialGrid          m_Grid;
    nanogui::Arcball    m_Arcball;

    // view state
    nanogui::Vector3f m_Translation;
    float m_Zoom;
    float m_PointSizeScale;
    bool m_OrthoMode;
    MouseMode m_MouseMode;

    // selection
    std::pair<nanogui::Vector2i, nanogui::Vector2i> m_SelectionRegion;
    std::function<void(const nanogui::Matrix4f&, const SelectionBox&,
        const nanogui::Vector2i&, SelectionMode)> m_SelectCallback;

    // global state for sample display
    int m_DrawFlags;
    std::shared_ptr<ColorMap> m_ColorMap;

};

TEKARI_NAMESPACE_END