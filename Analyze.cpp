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

#include "Analyze.h"

#include "ExpectiMax.h"

#include <iostream>

template<typename A, typename B>
inline bool CompareFirst(const std::pair<A, B>& a, const std::pair<A, B>& b) { return (a.first < b.first); }

void Analyze_Test1(BoardDB* boarddb) {

	unsigned int binsize = 10;

	std::vector<std::pair<unsigned int, unsigned int> > table;
	for(BoardScore &result : boarddb->m_results) {
		unsigned int value = 0;
		for(unsigned int location = 0; location < 16; ++location) {
			unsigned int cell = GetCell(result.m_board, location);
			if(cell != 0)
				value += 1 << cell;
		}
		table.push_back(std::make_pair(value, result.m_score));
	}
	std::sort(table.begin(), table.end(), CompareFirst<unsigned int, unsigned int>);

	std::vector<std::pair<unsigned int, unsigned int> > table_avg;
	unsigned int cur_value = 0;
	uint64_t cur_score = 0;
	unsigned int cur_duplicates = 0;
	for(auto &p : table) {
		if(cur_value != p.first / binsize) {
			if(cur_duplicates != 0) {
				unsigned int value = cur_value * binsize + binsize / 2;
				unsigned int score = (cur_score + cur_duplicates / 2) / cur_duplicates;
				table_avg.push_back(std::make_pair(value, score));
			}
			cur_value = p.first / binsize;
			cur_score = 0;
			cur_duplicates = 0;
		}
		cur_score += p.second;
		++cur_duplicates;
	}
	if(cur_duplicates != 0) {
		unsigned int value = cur_value * binsize + binsize / 2;
		unsigned int score = (cur_score + cur_duplicates / 2) / cur_duplicates;
		table_avg.push_back(std::make_pair(value, score));
	}

	std::cout << "analyze_value = array([\n\t";
	for(unsigned int i = 0; i < table_avg.size(); ++i) {
		std::cout << table_avg[i].first;
		if(i != table.size() - 1) {
			if(i % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

	std::cout << "analyze_score = array([\n\t";
	for(unsigned int i = 0; i < table_avg.size(); ++i) {
		std::cout << table_avg[i].second;
		if(i != table.size() - 1) {
			if(i % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

}

void Analyze_Test2(BoardDB* boarddb) {

	constexpr unsigned int pbins = 4;
	unsigned int binsize = 20;
	unsigned int pairbins[16] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};

	std::vector<std::pair<unsigned int, unsigned int> > table[pbins];
	for(BoardScore &result : boarddb->m_results) {
		unsigned int histogram[16] = {0};
		unsigned int value = 0;
		for(unsigned int location = 0; location < 16; ++location) {
			unsigned int cell = GetCell(result.m_board, location);
			if(cell != 0)
				value += 1 << cell;
			++histogram[cell];
		}
		unsigned int maxpbin = 0;
		for(unsigned int i = 0; i < 16; ++i) {
			if(histogram[i] > 1)
				maxpbin = pairbins[i];
		}
		for(unsigned int pbin = 0; pbin <= maxpbin; ++pbin) {
			table[pbin].push_back(std::make_pair(value, result.m_score));
		}
	}

	std::vector<unsigned int> table_score[pbins];
	unsigned int max_size = 0;
	for(unsigned int pbin = 0; pbin < pbins; ++pbin) {
		std::sort(table[pbin].begin(), table[pbin].end(), CompareFirst<unsigned int, unsigned int>);

		unsigned int cur_value = 0;
		uint64_t cur_score = 0;
		unsigned int cur_duplicates = 0;
		for(auto &t : table[pbin]) {
			if(cur_value != t.first / binsize) {
				if(cur_duplicates != 0) {
					unsigned int score = (cur_score + cur_duplicates / 2) / cur_duplicates;
					if(table_score[pbin].size() <= cur_value)
						table_score[pbin].resize(cur_value + 1, 0);
					table_score[pbin][cur_value] = score;
				}
				cur_value = t.first / binsize;
				cur_score = 0;
				cur_duplicates = 0;
			}
			cur_score += t.second;
			++cur_duplicates;
		}
		if(cur_duplicates != 0) {
			unsigned int score = (cur_score + cur_duplicates / 2) / cur_duplicates;
			if(table_score[pbin].size() <= cur_value)
				table_score[pbin].resize(cur_value + 1);
			table_score[pbin][cur_value] = score;
		}
		if(table_score[pbin].size() > max_size)
			max_size = table_score[pbin].size();

	}

	std::cout << "analyze_value = array([\n\t";
	for(unsigned int i = 0; i < max_size; ++i) {
		unsigned int value = i * binsize + binsize / 2;
		std::cout << value;
		if(i != max_size - 1) {
			if(i % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

	std::cout << "analyze_score = array([\n\t";
	for(unsigned int i = 0; i < max_size; ++i) {
		std::cout << "[";
		for(unsigned int p = 0; p < pbins; ++p) {
			std::cout << ((i < table_score[p].size())? table_score[p][i] : 0);
			if(p != pbins - 1) {
				std::cout << ", ";
			}
		}
		std::cout << "]";
		if(i != max_size - 1) {
			if(i % (20 / pbins) == (20 / pbins) - 1)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

}

void Analyze_Test3(BoardDB* boarddb) {

	constexpr unsigned int pbins = 4;
	unsigned int binsize = 200;
	unsigned int pbinoverhead[pbins] = {0, 50, 100, 200};

	std::vector<std::pair<unsigned int, unsigned int> > table[pbins];
	for(BoardScore &result : boarddb->m_results) {
		unsigned int histogram[16] = {0};
		for(unsigned int location = 0; location < 16; ++location) {
			unsigned int cell = GetCell(result.m_board, location);
			++histogram[cell];
		}
		unsigned int value = 0, overhead = 0;
		for(unsigned int i = 1; i < 16; ++i) {
			value += histogram[i] << i;
			if(value > (2u << i)) {
				unsigned int extra = value - (2u << i);
				//if(extra > overhead)
				//	overhead = extra;
				overhead += extra;
			}
		}
		for(unsigned int pbin = 0; pbin < pbins; ++pbin) {
			if(overhead >= pbinoverhead[pbin])
				table[pbin].push_back(std::make_pair(value, result.m_score));
		}
	}

	std::vector<unsigned int> table_score[pbins];
	unsigned int max_size = 0;
	for(unsigned int pbin = 0; pbin < pbins; ++pbin) {
		std::sort(table[pbin].begin(), table[pbin].end(), CompareFirst<unsigned int, unsigned int>);

		unsigned int cur_value = 0;
		uint64_t cur_score = 0;
		unsigned int cur_duplicates = 0;
		for(auto &t : table[pbin]) {
			if(cur_value != t.first / binsize) {
				if(cur_duplicates != 0) {
					unsigned int score = (cur_score + cur_duplicates / 2) / cur_duplicates;
					if(table_score[pbin].size() <= cur_value)
						table_score[pbin].resize(cur_value + 1, 0);
					table_score[pbin][cur_value] = score;
				}
				cur_value = t.first / binsize;
				cur_score = 0;
				cur_duplicates = 0;
			}
			cur_score += t.second;
			++cur_duplicates;
		}
		if(cur_duplicates != 0) {
			unsigned int score = (cur_score + cur_duplicates / 2) / cur_duplicates;
			if(table_score[pbin].size() <= cur_value)
				table_score[pbin].resize(cur_value + 1);
			table_score[pbin][cur_value] = score;
		}
		if(table_score[pbin].size() > max_size)
			max_size = table_score[pbin].size();

	}

	std::cout << "analyze_value = array([\n\t";
	for(unsigned int i = 0; i < max_size; ++i) {
		unsigned int value = i * binsize + binsize / 2;
		std::cout << value;
		if(i != max_size - 1) {
			if(i % 20 == 19)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

	std::cout << "analyze_score = array([\n\t";
	for(unsigned int i = 0; i < max_size; ++i) {
		std::cout << "[";
		for(unsigned int p = 0; p < pbins; ++p) {
			std::cout << ((i < table_score[p].size())? table_score[p][i] : 0);
			if(p != pbins - 1) {
				std::cout << ", ";
			}
		}
		std::cout << "]";
		if(i != max_size - 1) {
			if(i % (20 / pbins) == (20 / pbins) - 1)
				std::cout << ",\n\t";
			else
				std::cout << ", ";
		}
	}
	std::cout << "])" << std::endl;

}

/*void Analyze_Test4(BoardDB* boarddb) {

	constexpr unsigned int bins = 200;
	double table_mu[bins] = {0.0}, table_sigma[bins] = {0.0};
	unsigned int table_number[bins] = {0};

	HeuristicParameters params;
	unsigned int weight_table[16];
	GetDefaultHeuristicParameters(&params);
	for(unsigned int i = 0; i < 16; ++i) {
		unsigned int weight3 = i * params.m_values[PARAM_CENTEROFMASS4];
		unsigned int weight2 = i * (params.m_values[PARAM_CENTEROFMASS3] + weight3);
		unsigned int weight1 = i * (params.m_values[PARAM_CENTEROFMASS2] + weight2);
		unsigned int weight0 = i * (params.m_values[PARAM_CENTEROFMASS1] + weight1);
		weight_table[i] = weight0;
	}

	for(BoardScore &result : boarddb->m_results) {

		unsigned int score = params.m_values[PARAM_STILLALIVE];
		unsigned int freecell = params.m_values[PARAM_FREECELL];
		int wx = 0, wy = 0;

		for(unsigned int j = 0; j < 4; ++j) {
			for(unsigned int i = 0; i < 4; ++i) {
				unsigned int value = GetCell(result.m_board, i, j);
				if(value == 0) {
					score += freecell;
					freecell >>= 1;
				} else {
					unsigned int weight = weight_table[value];
					wx += ((int) i * 2 - 3) * (int) weight;
					wy += ((int) j * 2 - 3) * (int) weight;
				}
			}
		}

		score += (abs(wx) + abs(wy)) >> 10;

		unsigned int bin = std::min(bins - 1, result.m_score / 1000);
		table_mu[bin] += (double) score;
		++table_number[bin];

	}
	for(unsigned int i = 0; i < bins; ++i) {
		table_mu[i] /= (double) table_number[bins];
	}



}*/
