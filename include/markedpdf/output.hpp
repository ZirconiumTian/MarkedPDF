/*
 * File        output.hpp
 * Author      ZirconiumTian
 * Last Edited 2026/08/21
 * Description Output as PDF file...
 */

#pragma once

#include "mdparser.hpp"

#include <cstdint>
#include <podofo/podofo.h>
#include <string>
#include <string_view>

namespace markedpdf {
namespace output {

// Functions for conversion.
// UTF8 string <-> UTF32 string
std::string fromU32toUTF8(std::u32string_view);
std::u32string fromUTF8toU32(std::string_view);

struct NewPage {};
struct EndLine {};
struct SplitLine {};
struct RGB {
	uint8_t red, green, blue;
};

struct Color {
	RGB foreground, background;
	bool useDefaultColor = true;

	Color() = default;
	Color& fore(const uint8_t(&)[3]);
	Color& fore(uint8_t, uint8_t, uint8_t);
	Color& back(const uint8_t(&)[3]);
	Color& back(uint8_t, uint8_t, uint8_t);
	Color& reset();
};

struct Text {
	std::string content;
	uint size;
	bool styleItalic, styleBold;
	Text(std::string_view);
	Text& bold();
	Text& italic();
	Text& fontSize(uint);
};

/*
 * Class       PDFitem
 * Constructor PDFitem(std::string_view filePath)
 * Description Writes content into a PDF file using stream operators.
 */
class PDFitem {
private:
	PoDoFo::PdfMemDocument document;
	PoDoFo::PdfPainter painter;
	double cursorX, cursorY;
	std::string usingFontFamily;
	PoDoFo::PdfFont* fontRegular;
	PoDoFo::PdfFont* fontBold;
	PoDoFo::PdfFont* fontItalic;
	PoDoFo::PdfFont* fontBoldItalic;
	PoDoFo::PdfFont* getFont(const Text&);

public:
	PDFitem(std::string_view);
	~PDFitem();

	// Loads the parsed Document (output of the MarkDown parser) into the PDF.
	void loadFromDocument(const mdparser::Document&);

	PDFitem& operator<<(const Color&);
	PDFitem& operator<<(const SplitLine&);
	PDFitem& operator<<(const EndLine&);
	PDFitem& operator<<(const NewPage&);
	PDFitem& operator<<(const Text&);
	void setFontFamily(std::string_view);
	void saveFile();
};

} // namespace markedpdf::output
} // namespace markedpdf