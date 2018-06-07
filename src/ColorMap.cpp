#include "tekari/ColorMap.h"

using namespace std;

TEKARI_NAMESPACE_BEGIN

const string ColorMap::FOLDER_PATH = "../resources/color_maps/";
const vector<pair<const string, const string>> ColorMap::PREDEFINED_MAPS =
{
    { "Jet",            "jet.png" },
    { "BRG",            "brg.png" },
    { "CMR Map",        "CMRmap.png" },
    { "Cube Helix",     "cubehelix.png" },
    { "Gist Earth",     "gist_earth.png" },
    { "Gist Ncar",      "gist_ncar.png" },
    { "Gist Rainbow",   "gist_rainbow.png" },
    { "Gist Stern",     "gist_stern.png" },
    { "GNU Plot",       "gnu_plot.png" },
    { "GNU Plot 2",     "gnu_plot2.png" },
    { "HSV",            "hsv.png" },
    { "Inferno",        "inferno.png" },
    { "Numpy Spectral", "npy_spectral.png" },
    { "Ocean",          "ocean.png" },
    { "Prism",          "prism.png" },
    { "Rainbow",        "rainbow.png" },
    { "Terrain",        "terrain.png" },
};

ColorMap::ColorMap(const string& name, const string& filePath)
:   mName(name)
{
    glGenTextures(1, &mRenderId);
    glBindTexture(GL_TEXTURE_1D, mRenderId);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, numChanels;
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &numChanels, 0);
    if (!data)
    {
        throw runtime_error("Unable to open color map " + filePath);
    }
    if (h != 1)
    {
        throw runtime_error("Wrong color map format " + filePath + " (height should be 1)");
    }
    GLenum format = GL_RGB;
    if (numChanels == 3) format = GL_RGB;
    if (numChanels == 4) format = GL_RGBA;
    glTexImage1D(GL_TEXTURE_1D, 0, format, w, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_1D, 0);
    stbi_image_free(data);
}

void ColorMap::bind(unsigned int target)
{
    glActiveTexture(GL_TEXTURE0 + target);
    glBindTexture(GL_TEXTURE_1D, mRenderId);
}
void ColorMap::unbind(unsigned int target)
{
    glActiveTexture(GL_TEXTURE0 + target);
    glBindTexture(GL_TEXTURE_1D, 0);
}

TEKARI_NAMESPACE_END