/*
 * File        mdparser.cpp
 * Author      ZirconiumTian
 * Last Edited 2026/08/21 19:34
 */

#include "markedpdf/mdparser.hpp"

#include <cctype>
#include <cstdint>

bool isSpace(char32_t ch)
{
	return ch == U' ' || ch == U'\t' || ch == U'\v' || ch == U'\f';
}

std::vector<std::u32string> markedpdf::mdparser::getSourceLines(std::u32string_view source)
{
	std::vector<std::u32string> result;
	std::u32string line;
	for (char32_t ch : source) {
		if (ch == '\n') {
			result.push_back(line);
			line.clear();
			continue;
		}
		else if (ch == '\r') continue;
		line.push_back(ch);
	}
	if (!line.empty()) {
		result.push_back(line);
	}
	return result;
}

markedpdf::mdparser::TextItem::TextItem(std::u32string_view str):
	textContent(str) {}

markedpdf::mdparser::Document::~Document()
{
	for (auto& ptr : content) {
		delete ptr;
	}
}

markedpdf::mdparser::Heading::Heading(std::u32string_view str, int level):
	BlockItem(BlockTypes::Heading),
	textContent(str),
	headingLevel(level)

{
	drawSplitLineBelow = (headingLevel > 2);
}

markedpdf::mdparser::Paragraph::Paragraph(const std::u32string_view str):
	BlockItem(BlockTypes::Paragraph), text({str}) {}

markedpdf::mdparser::Paragraph::Paragraph(const std::vector<TextItem>& items):
	BlockItem(BlockTypes::Paragraph), text(items) {}

markedpdf::mdparser::CodeBlock::CodeBlock(
	const std::vector<std::u32string>& codeContent,
	std::u32string_view usingLanguage
): BlockItem(BlockTypes::CodeBlock), codeLines(codeContent), language(usingLanguage) {}

char32_t markedpdf::mdparser::InlineLexer::current() const
{
	if (pos >= readLine.size()) {
		return 0;
	}
	return readLine[pos];
}

char32_t markedpdf::mdparser::InlineLexer::peek(int n) const
{
	if (pos + n >= readLine.size()) {
		return 0;
	}
	return readLine[pos + n];
}

void markedpdf::mdparser::InlineLexer::forward()
{
	if (pos < readLine.size()) pos++;
}

markedpdf::mdparser::InlineLexer::InlineLexer(std::u32string_view line):
	currentItem(std::u32string_view()), readLine(line), pos(0) {}

markedpdf::mdparser::TextItem markedpdf::mdparser::InlineLexer::getNextItem()
{
	if (!current()) return TextItem(U"");
	currentItem = TextItem(U"");
	if (current() == U'`') {
		forward();
		while (current() && current() != U'`') {
			currentItem.textContent.push_back(current());
			forward();
		}
		if (current() == U'`') {
			currentItem.textStyle |= textstyles::inlineCode;
			forward();
			return currentItem;
		} else {
			currentItem.textContent = std::u32string(1, U'`') + currentItem.textContent;
		}
		return currentItem;
	}
	if (current() == U'*') {
		int startEmphasisLev = 0;
		int endEmphasisLev = 0;
		while (current() == U'*') {
			startEmphasisLev++;
			forward();
		}
		while (current() && current() != U'*') {
			currentItem.textContent.push_back(current());
			forward();
		}
		while (current() == U'*') {
			endEmphasisLev++;
			forward();
		}
		if (endEmphasisLev != startEmphasisLev || startEmphasisLev > 3) {
			currentItem.textContent =
				std::u32string(startEmphasisLev, U'*') +
				currentItem.textContent + std::u32string(endEmphasisLev, U'*');
			
			return currentItem;
		}
		switch (startEmphasisLev) {
			case 1: currentItem.textStyle |= textstyles::Emphasis; break;
			case 2: currentItem.textStyle |= textstyles::StrongEmphasis; break;
			case 3: currentItem.textStyle |= textstyles::CombinedEmphasis; break;
			default: break;
		}
		return currentItem;
	}
	if (current() == U'~') {
		if (peek() == U'~') {
			forward(); forward();
			std::u32string buf;
			while (current() && !(current() == U'~' && peek() == U'~')) {
				buf += current();
				forward();
			}
			if (current() == U'~') {
				forward(); forward();
				currentItem.textContent = std::move(buf);
				currentItem.textStyle = textstyles::StrikeThough;
				return currentItem;
			}
			currentItem.textContent = U"~~" + buf;
			return currentItem;
		} else {
			currentItem.textContent.push_back(current());
			forward();
		}
	}
	while (current() && current() != U'`' && current() != U'*' && current() != U'~') {
		currentItem.textContent.push_back(current());
		forward();
	}
	return currentItem;
}

std::vector<markedpdf::mdparser::TextItem> markedpdf::mdparser::InlineLexer::getItems()
{
	pos = readLine.find_first_not_of(U" \f\v\t");
	std::vector<TextItem> result;
	while (current()) {
		TextItem item = getNextItem();
		if (item.textContent.empty()) break;
		result.push_back(item);
	}
	return result;
}

markedpdf::mdparser::MDParser::MDParser(std::u32string_view singleLineSource)
{
	sourceContent = getSourceLines(singleLineSource);
}

markedpdf::mdparser::MDParser::~MDParser() {} // I don't know why I wrote this useless (~_~)

markedpdf::mdparser::BlockItem* markedpdf::mdparser::MDParser::getNextBlock()
{
	while (currentLineNum < sourceContent.size() &&
		sourceContent[currentLineNum].find_first_not_of(U" \t\v\f") == std::u32string::npos) {
		currentLineNum++;
	}
	if (currentLineNum >= sourceContent.size()) return nullptr;
	auto& line = sourceContent[currentLineNum];

	// Heading...
	if (line[0] == U'#') {
		size_t level = 0;
		while (level < line.size() && line[level] == U'#') ++level;
		if (level <= 6 && (level == line.size() || line[level] == U' ')) {
			size_t start = level;
			while (start < line.size() && line[start] == U' ') ++start;
			auto text = line.substr(start);
			++currentLineNum;
			return new Heading(text, static_cast<int>(level));
		}
	}

	// Splitline...
	if (line.size() >= 3) {
		bool allDash = true;
		for (char32_t ch : line) if (ch != U'-') { allDash = false; break; }
		if (allDash) {
			++currentLineNum;
			return new SplitLine();
		}
	}

	// Code block...
	if (line.starts_with(U"```")) {
		auto language = line.substr(3);
		++currentLineNum;
		std::vector<std::u32string> codeLines;
		while (currentLineNum < sourceContent.size() &&
			!sourceContent[currentLineNum].starts_with(U"```")) {
			codeLines.push_back(sourceContent[currentLineNum]);
			++currentLineNum;
		}
		if (currentLineNum < sourceContent.size()) ++currentLineNum;
		return new CodeBlock(codeLines, language);
	}

	// Paragraph
	std::u32string paragraphText;
	while (currentLineNum < sourceContent.size()) {
		auto& l = sourceContent[currentLineNum];
		if (l.find_first_not_of(U" \t\v\f") == std::u32string::npos) break;
		if (l[0] == U'#' || l.starts_with(U"```")) break;
		if (!paragraphText.empty()) paragraphText += U' ';
		paragraphText += l;
		++currentLineNum;
	}
	InlineLexer lexer(paragraphText);
	return new Paragraph(lexer.getItems());
}

markedpdf::mdparser::Document markedpdf::mdparser::MDParser::getParserResult()
{
	Document result;
	while (BlockItem* block = getNextBlock()) {
		result.content.push_back(block);
	}
	return result;
}