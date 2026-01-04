#include "Mode.hpp"
#include <stdexcept>

Mode::Mode() : score(0) {

}

Mode Mode::benchmark() {
	Mode r;
	r.name = "benchmark";
	r.kernel = "profanity_score_benchmark";
	return r;
}

Mode Mode::zeros() {
	Mode r = range(0, 0);
	r.name = "zeros";
	return r;
}

static std::string::size_type hexValueNoException(char c) {
	if (c >= 'A' && c <= 'F') {
		c -= 'A' - 'a';
	}

	const std::string hex = "0123456789abcdef";
	const std::string::size_type ret = hex.find(c);
	return ret;
}

static std::string::size_type hexValue(char c) {
	const std::string::size_type ret = hexValueNoException(c);
	if(ret == std::string::npos) {
		throw std::runtime_error("bad hex value");
	}

	return ret;
}

Mode Mode::matching(const std::string strHex) {
	Mode r;
	r.name = "matching";
	r.kernel = "profanity_score_matching";

	std::fill( r.data1, r.data1 + sizeof(r.data1), cl_uchar(0) );
	std::fill( r.data2, r.data2 + sizeof(r.data2), cl_uchar(0) );

	auto index = 0;
	
	for( size_t i = 0; i < strHex.size(); i += 2 ) {
		const auto indexHi = hexValueNoException(strHex[i]);
		const auto indexLo = i + 1 < strHex.size() ? hexValueNoException(strHex[i+1]) : std::string::npos;

		const auto valHi = (indexHi == std::string::npos) ? 0 : indexHi << 4;
		const auto valLo = (indexLo == std::string::npos) ? 0 : indexLo;

		const auto maskHi = (indexHi == std::string::npos) ? 0 : 0xF << 4;
		const auto maskLo = (indexLo == std::string::npos) ? 0 : 0xF;

		r.data1[index] = maskHi | maskLo;
		r.data2[index] = valHi | valLo;

		++index;
	}

	return r;
}

Mode Mode::leading(const char charLeading) {

	Mode r;
	r.name = "leading";
	r.kernel = "profanity_score_leading";
	r.data1[0] = static_cast<cl_uchar>(hexValue(charLeading));
	return r;
}

Mode Mode::range(const cl_uchar min, const cl_uchar max) {
	Mode r;
	r.name = "range";
	r.kernel = "profanity_score_range";
	r.data1[0] = min;
	r.data2[0] = max;
	return r;
}

Mode Mode::zeroBytes() {
	Mode r;
	r.name = "zeroBytes";
	r.kernel = "profanity_score_zerobytes";
	return r;
}

Mode Mode::letters() {
	Mode r = range(10, 15);
	r.name = "letters";
	return r;
}

Mode Mode::numbers() {
	Mode r = range(0, 9);
	r.name = "numbers";
	return r;
}

std::string Mode::transformKernel() const {
	switch (this->target) {
		case ADDRESS:
			return "";
		case CONTRACT:
			return "profanity_transform_contract";
		default:
			throw "No kernel for target";
	}
}

std::string Mode::transformName() const {
	switch (this->target) {
		case ADDRESS:
			return "Address";
		case CONTRACT:
			return "Contract";
		default:
			throw "No name for target";
	}
}

Mode Mode::leadingRange(const cl_uchar min, const cl_uchar max) {
	Mode r;
	r.name = "leadingrange";
	r.kernel = "profanity_score_leadingrange";
	r.data1[0] = min;
	r.data2[0] = max;
	return r;
}

Mode Mode::mirror() {
	Mode r;
	r.name = "mirror";
	r.kernel = "profanity_score_mirror";
	return r;
}

Mode Mode::doubles() {
	Mode r;
	r.name = "doubles";
	r.kernel = "profanity_score_doubles";
	return r;
}

// TRON vanity address modes

Mode Mode::tronRepeat() {
	Mode r;
	r.name = "tron-repeat (豹子号)";
	r.kernel = "profanity_score_tron_repeat";
	std::fill(r.data1, r.data1 + sizeof(r.data1), cl_uchar(0));
	std::fill(r.data2, r.data2 + sizeof(r.data2), cl_uchar(0));
	return r;
}

Mode Mode::tronSequential() {
	Mode r;
	r.name = "tron-sequential (顺子号)";
	r.kernel = "profanity_score_tron_sequential";
	std::fill(r.data1, r.data1 + sizeof(r.data1), cl_uchar(0));
	std::fill(r.data2, r.data2 + sizeof(r.data2), cl_uchar(0));
	return r;
}

Mode Mode::tronSuffix(const std::string suffix) {
	Mode r;
	r.name = "tron-suffix (自定义后缀)";
	r.kernel = "profanity_score_tron_suffix";

	std::fill(r.data1, r.data1 + sizeof(r.data1), cl_uchar(0));
	std::fill(r.data2, r.data2 + sizeof(r.data2), cl_uchar(0));

	// Parse comma-separated patterns
	// Store patterns in data1, separated by 0x00
	// data2[0] = total length including separators
	// data2[1] = number of patterns

	size_t dataPos = 0;
	size_t patternCount = 0;
	size_t i = 0;

	while (i < suffix.length() && dataPos < 19) {
		// Skip leading commas
		while (i < suffix.length() && suffix[i] == ',') {
			i++;
		}

		if (i >= suffix.length()) break;

		// Find pattern end
		size_t patternStart = i;
		while (i < suffix.length() && suffix[i] != ',') {
			i++;
		}
		size_t patternLen = i - patternStart;

		if (patternLen > 0) {
			// Check if pattern fits
			if (dataPos + patternLen + 1 > 20) {
				// Truncate if needed
				patternLen = 20 - dataPos - 1;
				if (patternLen <= 0) break;
			}

			// Copy pattern
			for (size_t j = 0; j < patternLen; ++j) {
				r.data1[dataPos++] = static_cast<cl_uchar>(suffix[patternStart + j]);
			}

			// Add null separator
			r.data1[dataPos++] = 0;
			patternCount++;
		}
	}

	r.data2[0] = static_cast<cl_uchar>(dataPos);
	r.data2[1] = static_cast<cl_uchar>(patternCount);

	return r;
}

Mode Mode::tronLucky() {
	Mode r;
	r.name = "tron-lucky (谐音靓号)";
	r.kernel = "profanity_score_tron_lucky";
	std::fill(r.data1, r.data1 + sizeof(r.data1), cl_uchar(0));
	std::fill(r.data2, r.data2 + sizeof(r.data2), cl_uchar(0));
	return r;
}

