/*
 * Project     MarkedPDF
 * Author      ZirconiumTian
 * License     GNU General Public License 3.0
 * 3rd Party   PoDoFo, MPL 2.0, https://github.com/podofo/podofo
 * Description A powerful CLI tool for converting MarkDown file to PDF (?_?)
 * Repository  https://github.com/zirconiumtian/markedpdf
 */

/*
 * File        main.cpp
 * Author      ZirconiumTian
 * Last Edited 2026/08/21 23:40
 */

#include "markedpdf/mdparser.hpp"
#include "markedpdf/output.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

std::string readFile(std::string_view path)
{
	std::ifstream file(path.data());
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open file.");
	}
	std::string res, line;
	while (std::getline(file, line)) {
		res += line + '\n';
	}
	return res;
}

int main(int argc, char** argv)
{
	using namespace markedpdf::mdparser;
	using namespace markedpdf::output;
	if (argc != 2) {
		std::cout << "Usage: markedpdf [FileName]" << std::endl;
		return 0;
	}
	MDParser parser(fromUTF8toU32(readFile(argv[1])));
	std::string outputName = argv[1];
	outputName += ".pdf";
	PDFitem pdf(outputName);
	pdf.setFontFamily("DengXian");
	pdf.loadFromDocument(parser.getParserResult());
	pdf.saveFile();
	return 0;
}