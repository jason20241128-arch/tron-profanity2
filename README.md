# TRON Vanity Address Generator (TRON 靓号生成器)

高性能 TRON 网络靓号地址生成器，使用 GPU OpenCL 加速计算。

## 功能特点

### 🐆 豹子号 (Leopard Number)
生成末尾重复字符的地址
- 例如: `T...AAAA`, `T...8888`, `T...aaaa`
- 命令: `./tron-vanity --tron-repeat`

### 📈 顺子号 (Sequential Number)
生成末尾连续递增或递减字符的地址
- 例如: `T...12345`, `T...54321`, `T...abcde`
- 命令: `./tron-vanity --tron-sequential`

### 🎯 自定义后缀 (Custom Suffix)
支持自定义任意后缀匹配
- 单个后缀: `./tron-vanity --tron-suffix 888`
- 多个后缀: `./tron-vanity --tron-suffix 888,999,666`
- 使用通配符: `./tron-vanity --tron-suffix 888XXX`

### 🍀 谐音靓号 (Lucky Number)
自动匹配中国传统吉祥数字
- `5211314` - 我爱你一生一世
- `1314521` - 一生一世我爱你  
- `168888` - 一路发发发发
- `888888` - 发发发发发发
- `666666` - 六六大顺
- 命令: `./tron-vanity --tron-lucky`

## 编译

```bash
make
```

## 使用方法

```bash
# 查看帮助
./tron-vanity --help

# 豹子号
./tron-vanity --tron-repeat

# 顺子号
./tron-vanity --tron-sequential

# 自定义后缀
./tron-vanity --tron-suffix 888

# 多个后缀
./tron-vanity --tron-suffix 888,999,666

# 谐音靓号
./tron-vanity --tron-lucky
```

## 输出

生成的地址会自动保存到 `output/` 目录：
- 文件名: 地址 (如 `TW7Kze8zohyiJjk9Y9BDtP4w94ew3So888.txt`)
- 文件内容: 私钥

## 安全说明

本工具基于 profanity2 修改，采用"设计即安全"原则：
- 自动生成种子密钥对
- 私钥 = 种子私钥 + 偏移量
- 无需信任第三方

## 系统要求

- 支持 OpenCL 的 GPU
- macOS / Linux / Windows

## 致谢

- 基于 [profanity2](https://github.com/1inch/profanity2) 修改
- 原始项目 [profanity](https://github.com/johguse/profanity) by Johan Gustafsson
