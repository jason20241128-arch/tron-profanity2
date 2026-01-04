#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#else
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

#define CL_DEVICE_PCI_BUS_ID_NV  0x4008
#define CL_DEVICE_PCI_SLOT_ID_NV 0x4009

#include "Dispatcher.hpp"
#include "ArgParser.hpp"
#include "Mode.hpp"
#include "help.hpp"
#include "KeyGenerator.hpp"

static std::string readFile(const char * const szFilename) {
	std::ifstream in(szFilename, std::ios::in | std::ios::binary);
	std::ostringstream contents;
	contents << in.rdbuf();
	return contents.str();
}

static std::vector<cl_device_id> getAllDevices(cl_device_type deviceType = CL_DEVICE_TYPE_GPU) {
	std::vector<cl_device_id> vDevices;

	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, NULL, &platformIdCount);

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), NULL);

	for (const auto & platformId : platformIds) {
		cl_uint countDevice;
		clGetDeviceIDs(platformId, deviceType, 0, NULL, &countDevice);

		std::vector<cl_device_id> deviceIds(countDevice);
		clGetDeviceIDs(platformId, deviceType, countDevice, deviceIds.data(), &countDevice);

		std::copy(deviceIds.begin(), deviceIds.end(), std::back_inserter(vDevices));
	}

	return vDevices;
}

template <typename T, typename U, typename V, typename W>
static T clGetWrapper(U function, V param, W param2) {
	T t;
	function(param, param2, sizeof(t), &t, NULL);
	return t;
}

template <typename U, typename V, typename W>
static std::string clGetWrapperString(U function, V param, W param2) {
	size_t len;
	function(param, param2, 0, NULL, &len);
	char * const szString = new char[len];
	function(param, param2, len, szString, NULL);
	std::string r(szString);
	delete[] szString;
	return r;
}

template <typename T, typename U, typename V, typename W>
static std::vector<T> clGetWrapperVector(U function, V param, W param2) {
	size_t len;
	function(param, param2, 0, NULL, &len);
	len /= sizeof(T);
	std::vector<T> v;
	if (len > 0) {
		T * pArray = new T[len];
		function(param, param2, len * sizeof(T), pArray, NULL);
		for (size_t i = 0; i < len; ++i) {
			v.push_back(pArray[i]);
		}
		delete[] pArray;
	}
	return v;
}

static std::vector<std::string> getBinaries(cl_program & clProgram) {
	std::vector<std::string> vReturn;
	auto vSizes = clGetWrapperVector<size_t>(clGetProgramInfo, clProgram, CL_PROGRAM_BINARY_SIZES);
	if (!vSizes.empty()) {
		unsigned char ** pBuffers = new unsigned char *[vSizes.size()];
		for (size_t i = 0; i < vSizes.size(); ++i) {
			pBuffers[i] = new unsigned char[vSizes[i]];
		}

		clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, vSizes.size() * sizeof(unsigned char *), pBuffers, NULL);
		for (size_t i = 0; i < vSizes.size(); ++i) {
			std::string strData(reinterpret_cast<char *>(pBuffers[i]), vSizes[i]);
			vReturn.push_back(strData);
			delete[] pBuffers[i];
		}

		delete[] pBuffers;
	}

	return vReturn;
}

static unsigned int getUniqueDeviceIdentifier(const cl_device_id & deviceId) {
#if defined(CL_DEVICE_TOPOLOGY_AMD)
	auto topology = clGetWrapper<cl_device_topology_amd>(clGetDeviceInfo, deviceId, CL_DEVICE_TOPOLOGY_AMD);
	if (topology.raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD) {
		return (topology.pcie.bus << 16) + (topology.pcie.device << 8) + topology.pcie.function;
	}
#endif
	cl_int bus_id = clGetWrapper<cl_int>(clGetDeviceInfo, deviceId, CL_DEVICE_PCI_BUS_ID_NV);
	cl_int slot_id = clGetWrapper<cl_int>(clGetDeviceInfo, deviceId, CL_DEVICE_PCI_SLOT_ID_NV);
	return (bus_id << 16) + slot_id;
}

template <typename T>
static bool printResult(const T & t, const cl_int & err) {
	std::cout << ((t == NULL) ? toString(err) : "OK") << std::endl;
	return t == NULL;
}

static bool printResult(const cl_int err) {
	std::cout << ((err != CL_SUCCESS) ? toString(err) : "OK") << std::endl;
	return err != CL_SUCCESS;
}

static std::string getDeviceCacheFilename(cl_device_id & d, const size_t & inverseSize) {
	const auto uniqueId = getUniqueDeviceIdentifier(d);
	return "cache-opencl." + toString(inverseSize) + "." + toString(uniqueId);
}

int main(int argc, char ** argv) {
	try {
		ArgParser argp(argc, argv);

		// 基本参数
		bool bHelp = false;
		bool bBenchmark = false;
		std::string strPublicKey;

		// TRON 模式参数
		bool bModeTronRepeat = false;
		bool bModeTronSequential = false;
		std::string strModeTronSuffix;
		bool bModeTronLucky = false;

		// 设备和性能参数
		std::vector<size_t> vDeviceSkipIndex;
		size_t worksizeLocal = 64;
		size_t worksizeMax = 0;
		bool bNoCache = false;
		size_t inverseSize = 255;
		size_t inverseMultiple = 16384;

		// 注册参数
		argp.addSwitch('h', "help", bHelp);
		argp.addSwitch('0', "benchmark", bBenchmark);
		argp.addSwitch('z', "publicKey", strPublicKey);

		// TRON 模式
		argp.addSwitch('R', "tron-repeat", bModeTronRepeat);
		argp.addSwitch('S', "tron-sequential", bModeTronSequential);
		argp.addSwitch('T', "tron-suffix", strModeTronSuffix);
		argp.addSwitch('L', "tron-lucky", bModeTronLucky);

		// 设备控制
		argp.addMultiSwitch('s', "skip", vDeviceSkipIndex);
		argp.addSwitch('n', "no-cache", bNoCache);

		// 性能调优
		argp.addSwitch('w', "work", worksizeLocal);
		argp.addSwitch('W', "work-max", worksizeMax);
		argp.addSwitch('i', "inverse-size", inverseSize);
		argp.addSwitch('I', "inverse-multiple", inverseMultiple);

		if (!argp.parse()) {
			std::cout << "错误: 参数解析失败" << std::endl;
			return 1;
		}

		if (bHelp) {
			std::cout << g_strHelp << std::endl;
			return 0;
		}

		// 选择模式
		Mode mode = Mode::benchmark();
		if (bBenchmark) {
			mode = Mode::benchmark();
		} else if (bModeTronRepeat) {
			mode = Mode::tronRepeat();
		} else if (bModeTronSequential) {
			mode = Mode::tronSequential();
		} else if (!strModeTronSuffix.empty()) {
			mode = Mode::tronSuffix(strModeTronSuffix);
		} else if (bModeTronLucky) {
			mode = Mode::tronLucky();
		} else {
			std::cout << g_strHelp << std::endl;
			return 0;
		}
		
		// 自动生成密钥对
		std::string generatedPrivateKey;
		if (strPublicKey.empty()) {
			std::cout << "未提供公钥，自动生成密钥对..." << std::endl;
			KeyGenerator keyGen;
			keyGen.generate();
			strPublicKey = keyGen.publicKey;
			generatedPrivateKey = keyGen.privateKey;
			std::cout << "种子私钥: 0x" << generatedPrivateKey << std::endl;
			std::cout << "种子公钥: " << strPublicKey << std::endl;
			std::cout << std::endl;
		}

		if (strPublicKey.length() != 128) {
			std::cout << "错误: 公钥必须是128位十六进制字符" << std::endl;
			return 1;
		}

		std::cout << "模式: " << mode.name << std::endl;

		// 获取设备
		std::vector<cl_device_id> vFoundDevices = getAllDevices();
		std::vector<cl_device_id> vDevices;
		std::map<cl_device_id, size_t> mDeviceIndex;

		std::vector<std::string> vDeviceBinary;
		std::vector<size_t> vDeviceBinarySize;
		cl_int errorCode;
		bool bUsedCache = false;

		std::cout << "设备:" << std::endl;
		for (size_t i = 0; i < vFoundDevices.size(); ++i) {
			if (std::find(vDeviceSkipIndex.begin(), vDeviceSkipIndex.end(), i) != vDeviceSkipIndex.end()) {
				continue;
			}

			cl_device_id & deviceId = vFoundDevices[i];
			const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
			const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
			const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);
			bool precompiled = false;

			if (!bNoCache) {
				std::ifstream fileIn(getDeviceCacheFilename(deviceId, inverseSize), std::ios::binary);
				if (fileIn.is_open()) {
					vDeviceBinary.push_back(std::string((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>()));
					vDeviceBinarySize.push_back(vDeviceBinary.back().size());
					precompiled = true;
				}
			}

			std::cout << "  GPU" << i << ": " << strName << ", " << (globalMemSize / 1024 / 1024) << " MB, "
			          << computeUnits << " CU" << (precompiled ? " [cached]" : "") << std::endl;
			vDevices.push_back(vFoundDevices[i]);
			mDeviceIndex[vFoundDevices[i]] = i;
		}

		if (vDevices.empty()) {
			std::cout << "错误: 未找到可用的GPU设备" << std::endl;
			return 1;
		}

		std::cout << std::endl;
		std::cout << "初始化 OpenCL..." << std::endl;
		std::cout << "  创建上下文..." << std::flush;
		auto clContext = clCreateContext(NULL, vDevices.size(), vDevices.data(), NULL, NULL, &errorCode);
		if (printResult(clContext, errorCode)) {
			return 1;
		}

		cl_program clProgram;
		if (vDeviceBinary.size() == vDevices.size()) {
			bUsedCache = true;
			std::cout << "  加载缓存内核..." << std::flush;
			const unsigned char ** pKernels = new const unsigned char *[vDevices.size()];
			for (size_t i = 0; i < vDeviceBinary.size(); ++i) {
				pKernels[i] = reinterpret_cast<const unsigned char *>(vDeviceBinary[i].data());
			}
			cl_int * pStatus = new cl_int[vDevices.size()];
			clProgram = clCreateProgramWithBinary(clContext, vDevices.size(), vDevices.data(), vDeviceBinarySize.data(), pKernels, pStatus, &errorCode);
			delete[] pKernels;
			delete[] pStatus;
			if (printResult(clProgram, errorCode)) {
				return 1;
			}
		} else {
			std::cout << "  编译内核..." << std::flush;
			const std::string strKeccak = readFile("keccak.cl");
			const std::string strVanity = readFile("profanity.cl");
			const char * szKernels[] = { strKeccak.c_str(), strVanity.c_str() };

			clProgram = clCreateProgramWithSource(clContext, sizeof(szKernels) / sizeof(char *), szKernels, NULL, &errorCode);
			if (printResult(clProgram, errorCode)) {
				return 1;
			}
		}

		std::cout << "  构建程序..." << std::flush;
		const std::string strBuildOptions = "-D PROFANITY_INVERSE_SIZE=" + toString(inverseSize) + " -D PROFANITY_MAX_SCORE=" + toString(PROFANITY_MAX_SCORE);
		if (printResult(clBuildProgram(clProgram, vDevices.size(), vDevices.data(), strBuildOptions.c_str(), NULL, NULL))) {
			return 1;
		}

		if (!bUsedCache && !bNoCache) {
			std::cout << "  保存缓存..." << std::flush;
			auto binaries = getBinaries(clProgram);
			for (size_t i = 0; i < binaries.size(); ++i) {
				std::ofstream fileOut(getDeviceCacheFilename(vDevices[i], inverseSize), std::ios::binary);
				fileOut.write(binaries[i].data(), binaries[i].size());
			}
			std::cout << "OK" << std::endl;
		}

		std::cout << std::endl;

		Dispatcher d(clContext, clProgram, mode, worksizeMax == 0 ? inverseSize * inverseMultiple : worksizeMax, inverseSize, inverseMultiple, 0, strPublicKey, generatedPrivateKey);
		for (auto & i : vDevices) {
			d.addDevice(i, worksizeLocal, mDeviceIndex[i]);
		}

		d.run();
		clReleaseContext(clContext);
		return 0;

	} catch (std::runtime_error & e) {
		std::cout << "运行时错误: " << e.what() << std::endl;
	} catch (...) {
		std::cout << "未知错误" << std::endl;
	}

	return 1;
}

