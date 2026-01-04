#ifndef HPP_HELP
#define HPP_HELP

#include <string>

const std::string g_strHelp = R"(
TRON Vanity Address Generator (TRON 靓号生成器)
================================================

Usage: ./tron-vanity [OPTIONS]

TRON 靓号模式 (必选其一):
  -R, --tron-repeat       豹子号: 末尾重复字符
                          例如: ...AAAA, ...8888, ...aaaa
  -S, --tron-sequential   顺子号: 末尾连续递增/递减字符
                          例如: ...12345, ...54321, ...abcde
  -T, --tron-suffix <str> 自定义后缀: 匹配指定后缀
                          支持 'X' 作为通配符, 逗号分隔多个后缀
                          例如: 888, 5211314, 888XXX, 888,999,666
  -L, --tron-lucky        谐音靓号: 匹配中国吉祥数字
                          例如: 5211314, 1314521, 168888, 888888, 666666

可选参数:
  -h, --help              显示帮助信息
  -z, --publicKey <key>   使用指定的种子公钥 (128位十六进制)
                          若不提供则自动生成密钥对

设备控制:
  -s, --skip <index>      跳过指定索引的GPU设备
  -n, --no-cache          不加载预编译的OpenCL内核缓存

性能调优:
  -w, --work <size>       OpenCL 本地工作大小 [默认: 64]
  -W, --work-max <size>   OpenCL 最大工作大小 [默认: -i * -I]
  -i, --inverse-size      单个工作项计算的模逆数量 [默认: 255]
  -I, --inverse-multiple  并行运行的工作项数量 [默认: 16384]

使用示例:
  # 豹子号 - 寻找末尾重复字符的地址
  ./tron-vanity --tron-repeat

  # 顺子号 - 寻找末尾连续字符的地址
  ./tron-vanity --tron-sequential

  # 自定义后缀 - 寻找以 888 结尾的地址
  ./tron-vanity --tron-suffix 888

  # 多个后缀 - 寻找以 888 或 999 或 666 结尾的地址
  ./tron-vanity --tron-suffix 888,999,666

  # 谐音靓号 - 寻找包含吉祥数字的地址
  ./tron-vanity --tron-lucky

  # 使用通配符 - 888 后面可以是任意3个字符
  ./tron-vanity --tron-suffix 888XXX

输出说明:
  生成的地址会自动保存到 output/ 目录
  文件名为地址, 内容为对应的私钥

关于:
  基于 profanity2 修改, 专门用于生成 TRON 网络靓号地址
  使用 GPU OpenCL 加速计算)";

#endif /* HPP_HELP */
