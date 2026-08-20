/*
 * File        mdparser.hpp
 * Author      ZirconiumTian
 * Last Edited 2026/08/20 18:56
 * Description MarkDown Parser.
 */

#pragma once

#include <cstdint>
#include <concepts>
#include <string>
#include <string_view>
#include <vector>

namespace markedpdf {
namespace mdparser {
std::vector<std::u32string> getSourceLines(std::u32string_view);

namespace textstyles {
inline const uint8_t
	Regular           = 0,
	Emphasis          = 1 << 0,
	StrongEmphasis    = 1 << 1,
	CombinedEmphasis  = 1 << 2,
	StrikeThough      = 1 << 3,
	inlineCode        = 1 << 4;
}

class TextItem {
public:
	std::u32string textContent;
	uint8_t textStyle = textstyles::Regular;
	TextItem(std::u32string_view);

	template<typename... Args>
	requires (sizeof...(Args) > 0 && (std::same_as<Args, uint8_t> && ...))
	TextItem(std::u32string_view str, Args... styles):
		textContent(str),
		textStyle(static_cast<uint8_t>((0 | ... | styles))) {}
};

// Document blocks currently supported...
enum class BlockTypes {
	Heading,
	Paragraph,
	CodeBlock,
	SplitLine
};

/*
 * Base Class  BlockItem
 * Description Stores different contents in documents.
 */
class BlockItem {
public:
	BlockTypes type;
	BlockItem(BlockTypes t) : type(t) {}
	virtual ~BlockItem() = default;
};

class Document {
public:
	std::vector<BlockItem*> content;
	Document() = default;
	~Document();
	Document(Document&&) = default;
	Document& operator=(Document&&) = default;
	Document& operator=(Document&) = delete;
	Document(const Document&) = delete;
};

class Heading : public BlockItem {
public:
	std::u32string textContent;
	int headingLevel = 0; // 0 means not allocated, not a level number.
	bool drawSplitLineBelow;
	Heading(std::u32string_view, int);
};

class Paragraph : public BlockItem {
public:
	std::vector<TextItem> text;
	Paragraph(const std::u32string_view);
	Paragraph(const std::vector<TextItem>&);
};

class CodeBlock : public BlockItem {
public:
	std::vector<std::u32string> codeLines;
	std::u32string language;
	bool showLanguageIdentifier;
	bool showLanguageFullName;
	CodeBlock(const std::vector<std::u32string>&, std::u32string_view);
};

class SplitLine : public BlockItem { public: SplitLine() : BlockItem(BlockTypes::SplitLine) {} };

/*
 * Class       InlineLexer
 * Constructor InlineLexer(std::u32string_view line)
 * Description Generates TextItems with different styles. Suitable for use in heading
 *             and paragraph blocks.
 */
class InlineLexer {
private:
	std::vector<TextItem> items;
	TextItem currentItem;
	std::u32string readLine;
	size_t pos;
	char32_t current() const;
	char32_t peek(int = 1) const;
	void forward();

public:
	InlineLexer(std::u32string_view);
	TextItem getNextItem();
	std::vector<TextItem> getItems();
};

/*
 * Class       MDParser
 * Constructor MDParser(std::u32string_view source)
 * Description Main class of the markdown parser.
 */
class MDParser {
private:
	std::vector<std::u32string> sourceContent;
	size_t currentLineNum = 0;

public:
	MDParser(std::u32string_view);
	~MDParser();
	BlockItem* getNextBlock();
	Document getParserResult();
};

} // namespace markedpdf::mdparser
} // namespace markedpdf