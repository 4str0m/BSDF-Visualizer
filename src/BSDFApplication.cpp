#include "BSDFApplication.h"

#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/popupbutton.h>
#include <nanogui/colorwheel.h>
#include <nanogui/checkbox.h>
#include <nanogui/slider.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/messagedialog.h>
#include <nanogui/label.h>

#include <algorithm>
#include <string>

using namespace nanogui;
using namespace std;

BSDFApplication::BSDFApplication()
:   nanogui::Screen(Vector2i(1200, 750), "BSDF Visualizer", true)
,   m_SelectedDataSample(nullptr)
,	m_MetadataWindow(nullptr)
,	m_HelpWindow(nullptr)
{
    m_3DView = new Widget{this};
    m_3DView->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill });

    // the canvas and footer
    m_BSDFCanvas = new BSDFCanvas{ m_3DView };
    m_BSDFCanvas->setBackgroundColor({ 50, 50, 50, 255 });
    m_BSDFCanvas->setSelectionCallback([this](const Matrix4f& mvp, const Vector2i& topLeft,
        const Vector2i& size, const Vector2i& canvasSize, DataSample::SelectionMode mode) {
        for (auto& dataSample : m_DataSamples)
        {
            if (dataSample != m_SelectedDataSample)
                dataSample->deselectAllPoints();
        }
        if (hasSelectedDataSample())
        {
            if (size.x() == 0 && size.y() == 0)
            {
                m_SelectedDataSample->deselectAllPoints();
            }
            else
            {
                m_SelectedDataSample->selectPoints(mvp, topLeft, size, canvasSize, mode);
            }
        }
    });

    m_Footer = new Widget{ m_3DView };
    m_Footer->setLayout(new GridLayout{ Orientation::Horizontal, 3, Alignment::Fill});
    
    auto makeFooterInfo = [this](string label) {
        auto container = new Widget{ m_Footer };
        container->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill });
        container->setFixedWidth(width() / 3);
        new Label{ container, label };
        auto info = new Label{ container, "-" };
        return info;
    };

    m_DataSampleName = makeFooterInfo("Data Sample Name : ");
    m_DataSamplePointsCount = makeFooterInfo("Points Count : ");
    m_DataSampleAverageHeight = makeFooterInfo("AverageIntensity : ");

    m_ToolWindow = new Window(this, "Tools");
    m_ToolWindow->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 5, 5});
    m_ToolWindow->setVisible(true);
    m_ToolWindow->setPosition({ 20, 20 });

    m_HelpButton = new Button(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_HELP);
    m_HelpButton->setCallback([this]() { toggleHelpWindow(); });
    m_HelpButton->setFontSize(15);
    m_HelpButton->setTooltip("Information about using BSDF Vidualizer (H)");
    m_HelpButton->setPosition({20, 0});

    m_HiddenOptionsButton = new PopupButton(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_TOOLS);
    m_HiddenOptionsButton->setBackgroundColor(Color{0.6f, 0.1f, 0.1f, 1.0f});
    
    auto addHiddenOptionToggle = [this](const string& label, const string& tooltip, bool checked,
        const std::function<void(bool)> &callback) {
        auto panel = new Widget{ m_HiddenOptionsButton->popup() };
        panel->setFixedHeight(20);
        panel->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill });
        new Label{panel, label};
        auto checkbox = new CheckBox{ panel, "", callback };
        checkbox->setChecked(checked);
    };

    addHiddenOptionToggle("Shadows", "Enable/Disable Data Shadowing", true, [](bool) {});

    // grid view otpions
    {
        auto label = new Label(m_ToolWindow, "View Options", "sans-bold", 25);
        label->setTooltip(
            "Various view modes. Hover on them to learn what they do."
        );

        auto panel = new Widget(m_ToolWindow);
        panel->setLayout(new GridLayout(Orientation::Horizontal, 3, Alignment::Fill));

        m_GridViewToggle = new Button(panel, "Grid");
        m_GridViewToggle->setFlags(Button::Flags::ToggleButton);
        m_GridViewToggle->setTooltip("Display/Hide grid (G)");
        m_GridViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->grid().setVisible(checked); });
        m_GridViewToggle->setPushed(true);

        m_OrthoViewToggle = new Button(panel, "Ortho");
        m_OrthoViewToggle->setFlags(Button::Flags::ToggleButton);
        m_OrthoViewToggle->setTooltip("Enable/Disable orthogonal projection (O)");
        m_OrthoViewToggle->setChangeCallback([this](bool checked) { m_BSDFCanvas->setOrthoMode(checked); });
        m_OrthoViewToggle->setPushed(false);

        auto gridColorPopupButton = new PopupButton(panel, "", ENTYPO_ICON_BUCKET);

        gridColorPopupButton->setFontSize(15);
        gridColorPopupButton->setChevronIcon(0);
        gridColorPopupButton->setTooltip("Grid Color");

        // Background color popup
        {
            auto popup = gridColorPopupButton->popup();
            popup->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 10});

            new Label{popup, "Grid Color"};
            auto colorwheel = new ColorWheel{popup, m_BSDFCanvas->grid().color()};

            new Label{popup, "Grid Alpha"};
            auto gridAlphaSlider = new Slider{popup};
            gridAlphaSlider->setRange({0.0f, 1.0f});
            gridAlphaSlider->setCallback([this](float value) {
                auto col = m_BSDFCanvas->grid().color();
                m_BSDFCanvas->grid().setColor(Color{
                    col.r(),
                    col.g(),
                    col.b(),
                    value,
                });
            });

            gridAlphaSlider->setValue(1.0);

            colorwheel->setCallback([gridAlphaSlider, this](const Color& value) {
                m_BSDFCanvas->grid().setColor(Color{
                    value.r(),
                    value.g(),
                    value.b(),
                    gridAlphaSlider->value(),
                });
            });
        }
    }

    // Open, save screenshot, save data
    {
        new Label(m_ToolWindow, "Data Samples", "sans-bold", 25);
        auto tools = new Widget{ m_ToolWindow };
        tools->setLayout(new GridLayout{Orientation::Horizontal, 4, Alignment::Fill});

        auto makeToolButton = [&](bool enabled, function<void()> callback, int icon = 0, string tooltip = "") {
            auto button = new Button{tools, "", icon};
            button->setCallback(callback);
            button->setTooltip(tooltip);
            button->setFontSize(15);
            button->setEnabled(enabled);
            return button;
        };

        makeToolButton(true, [this] { openDataSampleDialog(); }, ENTYPO_ICON_FOLDER, "Open data sample (CTRL+O)");
        makeToolButton(true, [this] { saveScreenShot(); }, ENTYPO_ICON_IMAGE, "Save image (CTRL+P)");
        makeToolButton(true, [this] { cout << "Data saved\n"; }, ENTYPO_ICON_SAVE, "Save data (CTRL+S)");
        makeToolButton(true, [this]() { toggleMetadataWindow(); }, ENTYPO_ICON_INFO, "Show selected dataset infos (I)");
    }

    // Data sample selection
    {
        m_DataSamplesScrollPanel = new VScrollPanel{ m_ToolWindow };

        m_ScrollContent = new Widget{ m_DataSamplesScrollPanel };
        m_ScrollContent->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill));

        m_DataSampleButtonContainer = new Widget{ m_ScrollContent };
        m_DataSampleButtonContainer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 0));
    }

    m_ColorMaps.push_back(make_shared<ColorMap>("../resources/color_maps/inferno.png"));
    m_ColorMaps.push_back(make_shared<ColorMap>("../resources/color_maps/jet.png"));

    setResizeCallback([this](Vector2i) { requestLayoutUpdate(); });
    requestLayoutUpdate();
}

BSDFApplication::~BSDFApplication()
{
    m_Framebuffer.free();
}

bool BSDFApplication::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;
    if (action == GLFW_PRESS)
    {
        bool alt = modifiers & GLFW_MOD_ALT;
        // control options
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            switch (key)
            {
            case GLFW_KEY_O:
                openDataSampleDialog();
                return true;
            case GLFW_KEY_S:
                cout << "Save data\n";
                return true;
            case GLFW_KEY_P:
                saveScreenShot();
                return true;
            case GLFW_KEY_1: if (!alt) break;
            case GLFW_KEY_KP_1:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::BACK);
                return true;
            case GLFW_KEY_3: if (!alt) break;
            case GLFW_KEY_KP_3:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::RIGHT);
                return true;
            case GLFW_KEY_7: if (!alt) break;
            case GLFW_KEY_KP_7:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::DOWN);
            }
        }
        else
        {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                if (hasSelectedDataSample())
                {
                    m_SelectedDataSample->deselectAllPoints();
                }
                return true;
            case GLFW_KEY_Q:
                setVisible(false);
                return true;
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4: case GLFW_KEY_5:
            case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
                selectDataSample(key - GLFW_KEY_1);
                return true;
            case GLFW_KEY_DELETE:
                    deleteDataSample(m_SelectedDataSample);
                    return true;
                break;
            case GLFW_KEY_UP: case GLFW_KEY_W:
                selectDataSample(selectedDataSampleIndex() - 1, false);
                return true;
            case GLFW_KEY_DOWN: case GLFW_KEY_S:
                selectDataSample(selectedDataSampleIndex() + 1, false);
                return true;
            case GLFW_KEY_ENTER:
                if (hasSelectedDataSample())
                {
                    correspondingButton(m_SelectedDataSample)->toggleView();
                    return true;
                }
                break;
            case GLFW_KEY_N:
                toggleView(DataSample::Views::NORMAL, m_SelectedDataSample, !m_SelectedDataSample->displayView(DataSample::Views::NORMAL));
                return true;
            case GLFW_KEY_L:
                toggleView(DataSample::Views::LOG, m_SelectedDataSample, !m_SelectedDataSample->displayView(DataSample::Views::LOG));
                return true;
            case GLFW_KEY_P:
                toggleView(DataSample::Views::PATH, m_SelectedDataSample, !m_SelectedDataSample->displayView(DataSample::Views::PATH));
                return true;
            case GLFW_KEY_G:
                toggleToolButton(m_GridViewToggle, false);
                return true;
            case GLFW_KEY_O: case GLFW_KEY_KP_5:
                toggleToolButton(m_OrthoViewToggle, false);
                return true;
            case GLFW_KEY_I:
                toggleMetadataWindow();
                return true;
            case GLFW_KEY_H:
                toggleHelpWindow();
                return true;
            case GLFW_KEY_KP_1:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_KP_3:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_KP_7:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::UP);
                return true;
            default:
                break;
            }
        }
    }
    return false;
}

void BSDFApplication::drawContents() {
    if (m_RequiresLayoutUpdate)
    {
        updateLayout();
        m_RequiresLayoutUpdate = false;
    }
}

void BSDFApplication::draw(NVGcontext * ctx)
{
    Screen::draw(ctx);
    /*nvgFontSize(ctx, 30.0f);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, nanogui::Color(1.0f, 0.8f));
    nvgText(ctx, mSize.x() / 2, mSize.y() / 2, "Hello world", nullptr);*/
}

void BSDFApplication::updateLayout()
{
    m_3DView->setFixedSize(mSize);
    
    m_Footer->setFixedSize(Vector2i{ mSize.x(), 20 });
    for(auto& footerInfos: m_Footer->children())
        footerInfos->setFixedWidth(width() / 3);

    m_BSDFCanvas->setFixedSize(Vector2i{ mSize.x(), mSize.y() - 20 });
    m_ToolWindow->setFixedSize({ 210, 400 });

    m_DataSamplesScrollPanel->setFixedHeight(
        m_ToolWindow->height() - m_DataSamplesScrollPanel->position().y()
    );

    performLayout();

    // With a changed layout the relative position of the mouse
    // within children changes and therefore should get updated.
    // nanogui does not handle this for us.
    double x, y;
    glfwGetCursorPos(mGLFWWindow, &x, &y);
    cursorPosCallbackEvent(x, y);
}

void BSDFApplication::openDataSampleDialog()
{
    string dataSamplePath = file_dialog(
    {
        {"txt",  "Data samples"},
    }, false);

    if (!dataSamplePath.empty())
    {
        try
        {
            shared_ptr<DataSample> newDataSample = make_shared<DataSample>(m_ColorMaps[1], dataSamplePath);
            m_DataSamples.push_back(newDataSample);
            addDataSampleButton(m_DataSamples.size() - 1, newDataSample);
        }
        catch (exception e)
        {
            auto errorMsgDialog = new MessageDialog(this, MessageDialog::Type::Warning, "Error loading data",
                e.what(), "Retry", "Cancel", true);
            errorMsgDialog->setCallback([this](int index)
            {
                if (index == 0) { openDataSampleDialog(); }
            });
        }
    }
    // Make sure we gain focus after seleting a file to be loaded.
    glfwFocusWindow(mGLFWWindow);
}

void BSDFApplication::saveScreenShot()
{
    if (!m_Framebuffer.ready())
    {
        m_Framebuffer.init(m_BSDFCanvas->size(), 1);
    }
    m_Framebuffer.bind();
    m_BSDFCanvas->draw(nvgContext());
    m_Framebuffer.downloadTGA("test.tga");
    m_Framebuffer.release();
}

void BSDFApplication::toggleMetadataWindow()
{
    if (m_MetadataWindow)
    {
        m_MetadataWindow->dispose();
        m_MetadataWindow = nullptr;
    }
    else
    {
        if (hasSelectedDataSample())
        {
            m_MetadataWindow = new MetadataWindow(this, &selectedDataSample()->metadata(), [this]() { toggleMetadataWindow(); });
            m_MetadataWindow->center();
            m_MetadataWindow->requestFocus();
        }
        else
        {
            auto errorWindow = new MessageDialog(this, MessageDialog::Type::Warning, "Metadata",
                "No data sample selected.", "close");
            errorWindow->setCallback([this](int index) { m_MetadataWindow = nullptr; });
            m_MetadataWindow = errorWindow;
        }
    }
}

void BSDFApplication::toggleHelpWindow()
{
    if (m_HelpWindow)
    {
        m_HelpWindow->dispose();
        m_HelpWindow = nullptr;
    }
    else
    {
        m_HelpWindow = new HelpWindow(this, [this]() {toggleHelpWindow(); });
        m_HelpWindow->center();
        m_HelpWindow->requestFocus();
    }
}

int BSDFApplication::dataSampleIndex(const shared_ptr<const DataSample> dataSample) const
{
    auto pos = static_cast<size_t>(distance(m_DataSamples.begin(), find(m_DataSamples.begin(), m_DataSamples.end(), dataSample)));
    return pos >= m_DataSamples.size() ? -1 : static_cast<int>(pos);
}

void BSDFApplication::selectDataSample(int index, bool clamped)
{
    if (m_DataSamples.empty())
        return;

    if (clamped)
        index = max(0, min(static_cast<int>(m_DataSamples.size()-1), index));
    else if (index < 0 || index >= m_DataSamples.size())
        return;

    selectDataSample(m_DataSamples[index]);
}

void BSDFApplication::selectDataSample(shared_ptr<DataSample> dataSample)
{
    // de-select previously selected button
    if (hasSelectedDataSample())
    {
        DataSampleButton* oldButton = correspondingButton(m_SelectedDataSample);
        oldButton->setIsSelected(false);
        oldButton->popup()->setVisible(false);
    }

    m_SelectedDataSample = dataSample;
    m_BSDFCanvas->selectDataSample(dataSample);
    if (hasSelectedDataSample())
    {
        auto button = correspondingButton(m_SelectedDataSample);
        button->setIsSelected(true);
        button->popup()->setVisible(true);

        m_DataSampleName->setCaption(m_SelectedDataSample->name());
        m_DataSamplePointsCount->setCaption(std::to_string(m_SelectedDataSample->pointsCount()));
        m_DataSampleAverageHeight->setCaption(std::to_string(m_SelectedDataSample->averageHeight()));
    }
    else
    {
        m_DataSampleName->setCaption("-");
        m_DataSamplePointsCount->setCaption("-");
        m_DataSampleAverageHeight->setCaption("-");
    }
    

    requestLayoutUpdate();
}

void BSDFApplication::deleteDataSample(shared_ptr<DataSample> dataSample)
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return;

    // erase data sample and corresponding button
    auto button = correspondingButton(dataSample);
    button->popup()->parent()->removeChild(button->popup());
    m_DataSampleButtonContainer->removeChild(index);
    
    m_BSDFCanvas->removeDataSample(dataSample);
    m_DataSamples.erase(find(m_DataSamples.begin(), m_DataSamples.end(), dataSample));

    // clear focus path and drag widget pointer, since it may refer to deleted button
    mDragWidget = nullptr;
    mDragActive = false;
    mFocusPath.clear();

    // select next valid one
    shared_ptr<DataSample> dataSampleToSelect = nullptr;
    if (index >= m_DataSamples.size()) --index;
    if (index >= 0)
    {
        dataSampleToSelect = m_DataSamples[index];
    }
    // Make sure no button is selected
    m_SelectedDataSample = nullptr;
    selectDataSample(dataSampleToSelect);
}

void BSDFApplication::addDataSampleButton(int index, shared_ptr<DataSample> dataSample)
{
    string cleanName = dataSample->metadata().sampleName;
    replace(cleanName.begin(), cleanName.end(), '_', ' ');
    auto dataSampleButton = new DataSampleButton(m_DataSampleButtonContainer, cleanName);
    dataSampleButton->setFixedHeight(30);

    dataSampleButton->setCallback([this, dataSample]() {
        selectDataSample(dataSample);
    });

    dataSampleButton->setDeleteCallback([this, dataSample]() {
        deleteDataSample(dataSample);
    });

    dataSampleButton->setToggleViewCallback([this, dataSample](bool checked) {
        int index = dataSampleIndex(dataSample);
        if (checked)    m_BSDFCanvas->addDataSample(m_DataSamples[index]);
        else            m_BSDFCanvas->removeDataSample(m_DataSamples[index]);
    });

    dataSampleButton->setToggleCallback([this, dataSample, dataSampleButton](bool checked) {
        for (int i = DataSample::Views::NORMAL; i != DataSample::Views::VIEW_COUNT; ++i)
        {
            DataSample::Views view = static_cast<DataSample::Views>(i);
            toggleView(view, dataSample, dataSampleButton->isButtonToggled(view));
        }
    });

    selectDataSample(dataSample);
    // by default toggle view for the new data samples
    m_BSDFCanvas->addDataSample(selectedDataSample());
}

void BSDFApplication::toggleToolButton(nanogui::Button* button, bool needsSelectedDataSample)
{
    if (!needsSelectedDataSample || hasSelectedDataSample())
    {
        button->setPushed(!button->pushed());
        button->changeCallback()(button->pushed());
    }
}

void BSDFApplication::toggleView(DataSample::Views view, shared_ptr<DataSample> dataSample, bool toggle)
{
    if (dataSample)
    {
        dataSample->toggleView(view, toggle);
        correspondingButton(dataSample)->toggleButton(view, dataSample->displayView(view));
    }
}

DataSampleButton* BSDFApplication::correspondingButton(const std::shared_ptr<const DataSample> dataSample)
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(m_DataSampleButtonContainer->childAt(index));
}

const DataSampleButton* BSDFApplication::correspondingButton(const std::shared_ptr<const DataSample> dataSample) const
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(m_DataSampleButtonContainer->childAt(index));
}