#include "steganography.h"
#include <vector>
#include <cstdint>
bool encodeMessage(GdkPixbuf* pixbuf, const std::string& message) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    uint32_t len = static_cast<uint32_t>(message.size());
    std::vector<unsigned char> bits;
    for (int i = 31; i >= 0; i--) bits.push_back(static_cast<unsigned char>((len >> i) & 1));
    for (size_t i = 0; i < message.size(); i++) {
        unsigned char c = static_cast<unsigned char>(message[i]);
        for (int b = 7; b >= 0; b--) bits.push_back(static_cast<unsigned char>((c >> b) & 1));
    }
    long capacity = static_cast<long>(width) * static_cast<long>(height) * 3;
    if (static_cast<long>(bits.size()) > capacity) return false;
    size_t bitIndex = 0;
    for (int y = 0; y < height && bitIndex < bits.size(); y++) {
        guchar* row = pixels + y * rowstride;
        for (int x = 0; x < width && bitIndex < bits.size(); x++) {
            guchar* pixel = row + x * channels;
            for (int c = 0; c < 3 && bitIndex < bits.size(); c++) {
                pixel[c] = static_cast<guchar>((pixel[c] & 0xFE) | bits[bitIndex]);
                bitIndex++;
            }
        }
    }
    return true;
}
std::string decodeMessage(GdkPixbuf* pixbuf) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    std::vector<unsigned char> bits;
    for (int y = 0; y < height; y++) {
        guchar* row = pixels + y * rowstride;
        for (int x = 0; x < width; x++) {
            guchar* pixel = row + x * channels;
            for (int c = 0; c < 3; c++) bits.push_back(static_cast<unsigned char>(pixel[c] & 1));
        }
    }
    if (bits.size() < 32) return "";
    uint32_t len = 0;
    for (int i = 0; i < 32; i++) len = (len << 1) | bits[i];
    if (len == 0) return "";
    long maxLen = (static_cast<long>(bits.size()) - 32) / 8;
    if (static_cast<long>(len) > maxLen) return "";
    std::string message;
    size_t bitIndex = 32;
    for (uint32_t i = 0; i < len; i++) {
        unsigned char c = 0;
        for (int b = 0; b < 8; b++) {
            c = static_cast<unsigned char>((c << 1) | bits[bitIndex]);
            bitIndex++;
        }
        message.push_back(static_cast<char>(c));
    }
    return message;
}
long getMaxMessageCapacity(GdkPixbuf* pixbuf) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    long capacity = static_cast<long>(width) * static_cast<long>(height) * 3;
    long result = (capacity - 32) / 8;
    if (result < 0) return 0;
    return result;
}
