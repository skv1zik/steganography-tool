#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string>
bool encodeMessage(GdkPixbuf* pixbuf, const std::string& message);
std::string decodeMessage(GdkPixbuf* pixbuf);
long getMaxMessageCapacity(GdkPixbuf* pixbuf);
#endif
