/*
Copyright (c) 2014 Maarten Baert <maarten-baert@hotmail.com>

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

#include "BoardDB.h"

#include <fstream>
#include <iomanip>
#include <iostream>

void BoardDB::Load() {
	std::ifstream stream("../Speed2048/boarddb.txt", std::ios_base::in | std::ios_base::binary);
	if(stream.fail())
		return;
	m_results.clear();
	for( ; ; ) {
		BoardScore result;
		stream >> std::hex >> result.m_board.m_data;
		stream >> std::dec >> result.m_score;
		if(stream.eof())
			break;
		m_results.push_back(result);
	}
}

void BoardDB::Save() {
	std::ofstream stream("../Speed2048/boarddb.txt", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if(stream.fail()) {
		std::cout << "Can't save BoardDB!" << std::endl;
		exit(1);
	}
	for(BoardScore &result : m_results) {
		stream << std::hex << std::setw(16) << std::setfill('0') << result.m_board.m_data << " ";
		stream << std::dec << result.m_score << "\n";
	}
}
