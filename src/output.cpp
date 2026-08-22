/*
 * File        output.cpp
 * Author      ZirconiumTian
 * Last Edited 2026/08/21 23:40
 */

#include "markedpdf/output.hpp"
#include "markedpdf/mdparser.hpp"

#include <stdexcept>

std::string markedpdf::output::fromU32toUTF8(std::u32string_view str)
{
	std::string result;
	for (char32_t cp : str) {
		if (cp < 0x80) result += static_cast<char>(cp);
		else if (cp < 0x800) {
			result += static_cast<char>(0xC0 | (cp >> 6));
			result += static_cast<char>(0x80 | (cp & 0x3F));
		} else if (cp < 0x10000) {
			if (cp >= 0xD800 && cp <= 0xDFFF) continue;
			result += static_cast<char>(0xE0 | (cp >> 12));
			result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (cp & 0x3F));
		} else if (cp <= 0x10FFFF) {
			result += static_cast<char>(0xF0 | (cp >> 18));
			result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (cp & 0x3F));
		}
	}
	return result;
}

std::u32string markedpdf::output::fromUTF8toU32(std::string_view str)
{
	std::u32string result;
	size_t i = 0;
	while (i < str.size()) {
		unsigned char c = static_cast<unsigned char>(str[i]);
		char32_t cp = 0;
		size_t len = 0;
		if (c < 0x80) { cp = c; len = 1; }
		else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
		else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
		else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
		else { i++; continue; }
		if (i + len > str.size()) break;
		bool valid = true;
		for (size_t k = 1; k < len; k++) {
			unsigned char cc = static_cast<unsigned char>(str[i + k]);
			if ((cc & 0xC0) != 0x80) {
				valid = false;
				break;
			}
			cp = (cp << 6) | (cc & 0x3F);
		}
		if (!valid) {
			i++;
			continue;
		}
		if ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000)) { i++; continue; }
		if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
			i++;
			continue; 
		}
		result += cp;
		i += len;
	}
	return result;
}

// Page layout constants (pt)
static constexpr double Margin     = 50.0;    // page margin
static constexpr double PageWidth  = 595.0;   // A4 width
static constexpr double PageHeight = 842.0;   // A4 height
static constexpr double LineHeight = 14.0;    // default line spacing

// Measure the width of a UTF-8 string with the given font and size.
static double measureWidth(const PoDoFo::PdfFont& font, const std::string& utf8, double fontSize)
{
	PoDoFo::PdfTextState ts;
	ts.FontSize = fontSize;
	double width = 0;
	for (char32_t cp : markedpdf::output::fromUTF8toU32(utf8)) {
		width += font.GetCharLength(cp, ts);
	}
	return width;
}

// PoDoFo cannot map '\t' to any glyph in CID fonts: expand to spaces first.
static std::string expandTabs(const std::string& s)
{
	std::string out = s;
	for (size_t p = out.find('\t'); p != std::string::npos; p = out.find('\t', p + 4)) {
		out.replace(p, 1, "    ");
	}
	return out;
}

markedpdf::output::Color& markedpdf::output::Color::fore(const uint8_t(&RGBcolor)[3])
{
	foreground = {
		RGBcolor[0], RGBcolor[1], RGBcolor[2]
	};
	return *this;
}

markedpdf::output::Color& markedpdf::output::Color::fore(uint8_t R, uint8_t G, uint8_t B)
{
	foreground = {R, G, B};
	return *this;
}

markedpdf::output::Color& markedpdf::output::Color::back(const uint8_t(&RGBcolor)[3])
{
	background = {
		RGBcolor[0], RGBcolor[1], RGBcolor[2]
	};
	return *this;
}

markedpdf::output::Color& markedpdf::output::Color::back(uint8_t R, uint8_t G, uint8_t B)
{
	background = {R, G, B};
	return *this;
}

markedpdf::output::Color& markedpdf::output::Color::reset()
{
	foreground = {0, 0, 0};
	background = {255, 255, 255};
	return *this;
}

markedpdf::output::Text::Text(std::string_view text) : content(text), size(10) {}
markedpdf::output::Text& markedpdf::output::Text::bold()
{
	styleBold = true;
	return *this;
}

markedpdf::output::Text& markedpdf::output::Text::italic()
{
	styleItalic = true;
	return *this;
}

markedpdf::output::Text& markedpdf::output::Text::fontSize(uint setSize)
{
	size = setSize;
	return *this;
}

PoDoFo::PdfFont* markedpdf::output::PDFitem::getFont(const Text& text)
{
	PoDoFo::PdfFont*& slot = text.styleBold
		? (text.styleItalic ? fontBoldItalic : fontBold)
		: (text.styleItalic ? fontItalic : fontRegular);
	if (!slot) {
		PoDoFo::PdfFontSearchParams sp;
		sp.Style = static_cast<PoDoFo::PdfFontStyle>(
			(text.styleBold   ? (int)PoDoFo::PdfFontStyle::Bold   : 0) |
			(text.styleItalic ? (int)PoDoFo::PdfFontStyle::Italic : 0));
		slot = document.GetFonts().SearchFont(usingFontFamily, sp);
		if (!slot) slot = document.GetFonts().SearchFont(usingFontFamily);
		if (!slot) {
			throw std::runtime_error("Font not found: " + usingFontFamily);
		}
	}
	return slot;
}

markedpdf::output::PDFitem::PDFitem(std::string_view filePath):
	cursorX(Margin), cursorY(PageHeight - Margin),
	fileName(filePath)

{
	currentColor.fore({0, 0, 0});
	currentColor.back({255, 255, 255});
	auto& page = document.GetPages().CreatePage(
		PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::PdfPageSize::A4)
	);
	painter.SetCanvas(page);
}

markedpdf::output::PDFitem::~PDFitem()
{
	try { painter.FinishDrawing(); } catch (...) {}
}

void markedpdf::output::PDFitem::loadFromDocument(const mdparser::Document& doc)
{
	for (auto* block : doc.content) {
		switch (block->type) {
		case mdparser::BlockTypes::Heading: {
			auto* h = static_cast<mdparser::Heading*>(block);
			*this << Text(fromU32toUTF8(h->textContent)).bold().fontSize(26)
			      << EndLine{};
			if (h->drawSplitLineBelow) *this << SplitLine{};
			break;
		}
		case mdparser::BlockTypes::Paragraph: {
			auto* p = static_cast<mdparser::Paragraph*>(block);
			for (auto& item : p->text) {
				Text t(fromU32toUTF8(item.textContent));
				if (item.textStyle & mdparser::textstyles::StrongEmphasis) t.bold();
				if (item.textStyle & mdparser::textstyles::Emphasis) t.italic();
				*this << t;
			}
			*this << EndLine{};
			break;
		}
		case mdparser::BlockTypes::CodeBlock: {
			auto* c = static_cast<mdparser::CodeBlock*>(block);
			double h = c->codeLines.size() * LineHeight + 16;
			painter.GraphicsState.SetFillColor(PoDoFo::PdfColor(0.93, 0.93, 0.93));
			painter.DrawRectangle(Margin, cursorY - h, PageWidth - Margin * 2, h,
				PoDoFo::PdfPathDrawMode::Fill);
			const RGB& fg = currentColor.useDefaultColor ? RGB{0, 0, 0} : currentColor.foreground;
			painter.GraphicsState.SetFillColor(PoDoFo::PdfColor(fg.red / 255.0, fg.green / 255.0, fg.blue / 255.0));
			PoDoFo::PdfFont* mono = document.GetFonts().SearchFont("JetBrains Mono");
			if (!mono) throw std::runtime_error("Font not found: JetBrains Mono");
			double y = cursorY - 12;
			for (auto& ln : c->codeLines) {
				painter.TextState.SetFont(*mono, 10);
				painter.DrawText(expandTabs(fromU32toUTF8(ln)), Margin + 8, y);
				y -= LineHeight;
			}
			cursorY -= h + 8;
			break;
		}
		case mdparser::BlockTypes::SplitLine:
			*this << SplitLine{};
			break;
		}
	}
}

markedpdf::output::PDFitem& markedpdf::output::PDFitem::operator<<(const Color& color)
{
	currentColor = color;
	const RGB& fg = color.useDefaultColor ? RGB{0, 0, 0} : color.foreground;
	painter.GraphicsState.SetFillColor(
		PoDoFo::PdfColor(fg.red / 255.0, fg.green / 255.0, fg.blue / 255.0)
	);
	return *this;
}

markedpdf::output::PDFitem& markedpdf::output::PDFitem::operator<<(const SplitLine&)
{
	painter.GraphicsState.SetStrokeColor(PoDoFo::PdfColor(24 / 255.0, 24 / 255.0, 24 / 255.0));
	painter.GraphicsState.SetLineWidth(0.5);
	painter.DrawLine(Margin, cursorY, PageWidth - Margin, cursorY);
	return *this;
}

markedpdf::output::PDFitem& markedpdf::output::PDFitem::operator<<(const EndLine&)
{
	cursorX = Margin;
	cursorY -= LineHeight;
	return *this;
}

markedpdf::output::PDFitem& markedpdf::output::PDFitem::operator<<(const NewPage&)
{
	painter.FinishDrawing();
	auto& page = document.GetPages().CreatePage(PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::PdfPageSize::A4));
	painter.SetCanvas(page);
	cursorX = Margin;
	cursorY = PageHeight - Margin;
	return *this;
}

markedpdf::output::PDFitem& markedpdf::output::PDFitem::operator<<(const Text& textItem)
{
	const std::string content = expandTabs(textItem.content);
	auto* font = getFont(textItem);
	painter.TextState.SetFont(*font, textItem.size);
	painter.DrawText(content, cursorX, cursorY);
	cursorX += measureWidth(*font, content, textItem.size);
	return *this;
}

void markedpdf::output::PDFitem::setFontFamily(std::string_view fontFamilyName)
{
	usingFontFamily = fontFamilyName;
	fontRegular = fontBold = fontItalic = fontBoldItalic = nullptr;
}

void markedpdf::output::PDFitem::saveFile()
{
	painter.FinishDrawing();
	document.Save(fileName);
}