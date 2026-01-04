# tron-vanity

TRON 靓号地址生成器 - A high performance vanity address generator for **TRON** network. Create customized addresses with your favorite patterns! Powered by GPU using OpenCL.

![Screenshot](/img/screenshot.png?raw=true "Wow! That's a lot of zeros!")

## TRON 靓号功能 (TRON Vanity Address Features)

### 🐆 豹子号 (Leopard Number)
支持生成末尾n位相同的地址
- 例如: `T...AAAA`, `T...8888`, `T...aaaa`
- 命令: `--tron-repeat` 或 `-R`

### 📈 顺子号 (Sequential Number)
支持生成末尾为连续递增或递减的顺子号
- 例如: `T...12345`, `T...54321`, `T...abcde`
- 命令: `--tron-sequential` 或 `-S`

### 🎯 自定义后缀 (Custom Suffix)
支持自定义任意后缀匹配，使用 `X` 作为通配符，支持多个后缀用逗号分隔
- 单个后缀: `T...5211314`, `T...888XXX`
- 多个后缀: `888,999,666` (匹配任意一个)
- 命令: `--tron-suffix <pattern>` 或 `-T <pattern>`

### 🍀 谐音靓号 (Lucky Number Patterns)
支持生成中国传统吉祥数字谐音靓号
- `5211314` - 我爱你一生一世
- `1314521` - 一生一世我爱你
- `168888` - 一路发发发发
- `888888` - 发发发发发发
- `666666` - 六六大顺
- `520` - 我爱你
- `1314` - 一生一世
- 命令: `--tron-lucky` 或 `-L`

# Important to know

A previous version of this project has a known critical issue due to a bad source of randomness. The issue enables attackers to recover private key from public key: https://blog.1inch.io/a-vulnerability-disclosed-in-profanity-an-ethereum-vanity-address-tool

This project "profanity2" was forked from the original project and modified to guarantee **safety by design**. This means source code of this project do not require any audits, but still guarantee safe usage.

Project "profanity2" is not generating key anymore, instead it adjusts user-provided public key until desired vanity address will be discovered. Users provide seed public key in form of 128-symbol hex string with `-z` parameter flag. Resulting private key should be used to be added to seed private key to achieve final private key of the desired vanity address (private keys are just 256-bit numbers). Running "profanity2" can even be outsourced to someone completely unreliable - it is still safe by design.

## Getting public key for mandatory `-z` parameter

Generate private key and public key via openssl in terminal (remove prefix "04" from public key):
```bash
$ openssl ecparam -genkey -name secp256k1 -text -noout -outform DER | xxd -p -c 1000 | sed 's/41534e31204f49443a20736563703235366b310a30740201010420/Private Key: /' | sed 's/a00706052b8104000aa144034200/\'$'\nPublic Key: /'
```

Derive public key from existing private key via openssl in terminal (remove prefix "04" from public key):
```bash
$ openssl ec -inform DER -text -noout -in <(cat <(echo -n "302e0201010420") <(echo -n "PRIVATE_KEY_HEX") <(echo -n "a00706052b8104000a") | xxd -r -p) 2>/dev/null | tail -6 | head -5 | sed 's/[ :]//g' | tr -d '\n' && echo
```

## Adding private keys (never use online calculators!)

### Terminal:

Use private keys as 64-symbol hexadecimal string WITHOUT `0x` prefix:
```bash
(echo 'ibase=16;obase=10' && (echo '(PRIVATE_KEY_A + PRIVATE_KEY_B) % FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F' | tr '[:lower:]' '[:upper:]')) | bc
```

### Python

Use private keys as 64-symbol hexadecimal string WITH `0x` prefix:
```bash
$ python3
>>> hex((PRIVATE_KEY_A + PRIVATE_KEY_B) % 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F)
```

# TRON Usage (TRON 靓号使用方法)

## Quick Start (快速开始)

现在程序会自动生成密钥对，您只需要选择想要的靓号模式即可：

```bash
# 编译
make

# 直接运行 - 程序会自动生成密钥对
./tron-vanity --tron-repeat
```

程序会输出：
- **Seed Private Key**: 种子私钥（请妥善保存！）
- **Seed Public Key**: 种子公钥  
- **Result Private Key**: 找到的地址对应的私钥偏移

最终私钥 = (Seed Private Key + Result Private Key) mod N

## TRON Examples (TRON 靓号示例)

```bash
# 豹子号 - 寻找末尾重复字符的地址 (e.g., T...8888)
./tron-vanity --tron-repeat

# 顺子号 - 寻找末尾连续字符的地址 (e.g., T...12345)
./tron-vanity --tron-sequential

# 自定义后缀 - 精确匹配特定后缀 (e.g., T...5211314)
./tron-vanity --tron-suffix 5211314

# 自定义后缀带通配符 - X表示任意字符 (e.g., T...888abc)
./tron-vanity --tron-suffix 888XXX

# 多个自定义后缀 - 用逗号分隔，匹配任意一个
./tron-vanity --tron-suffix 888,999,666,5211314

# 谐音靓号 - 自动匹配中国吉祥数字模式
./tron-vanity --tron-lucky
```

## Advanced Usage (高级用法 - 使用自己的公钥)

如果您想使用自己的公钥，可以通过 `-z` 参数指定：

```bash
# 使用 OpenSSL 生成密钥对
openssl ecparam -genkey -name secp256k1 -text -noout -outform DER | xxd -p -c 1000 | sed 's/41534e31204f49443a20736563703235366b310a30740201010420/Private Key: /' | sed 's/a00706052b8104000aa144034200/\'$'\nPublic Key: /'

# 使用您的公钥运行 (去掉 "04" 前缀)
./tron-vanity --tron-repeat -z YOUR_128_CHAR_PUBLIC_KEY
```

## How It Works

1. **自动生成/提供密钥**: 程序自动生成或使用您提供的密钥对
2. **GPU搜索**: 程序使用GPU搜索符合条件的靓号地址
3. **合并私钥**: 将找到的私钥偏移加到种子私钥上
4. **验证**: 务必验证最终地址是否符合预期

# Usage
```
usage: ./profanity2 [OPTIONS]

  Mandatory args:
    -z                      Seed public key to start, add it's private key
                            to the "profanity2" resulting private key.

  Basic modes:
    --benchmark             Run without any scoring, a benchmark.
    --zeros                 Score on zeros anywhere in hash.
    --letters               Score on letters anywhere in hash.
    --numbers               Score on numbers anywhere in hash.
    --mirror                Score on mirroring from center.
    --leading-doubles       Score on hashes leading with hexadecimal pairs
    -b, --zero-bytes        Score on hashes containing the most zero bytes

  Modes with arguments:
    --leading <single hex>  Score on hashes leading with given hex character.
    --matching <hex string> Score on hashes matching given hex string.

  Advanced modes:
    --contract              Instead of account address, score the contract
                            address created by the account's zeroth transaction.
    --leading-range         Scores on hashes leading with characters within
                            given range.
    --range                 Scores on hashes having characters within given
                            range anywhere.

  Range:
    -m, --min <0-15>        Set range minimum (inclusive), 0 is '0' 15 is 'f'.
    -M, --max <0-15>        Set range maximum (inclusive), 0 is '0' 15 is 'f'.

  Device control:
    -s, --skip <index>      Skip device given by index.
    -n, --no-cache          Don't load cached pre-compiled version of kernel.

  Tweaking:
    -w, --work <size>       Set OpenCL local work size. [default = 64]
    -W, --work-max <size>   Set OpenCL maximum work size. [default = -i * -I]
    -i, --inverse-size      Set size of modular inverses to calculate in one
                            work item. [default = 255]
    -I, --inverse-multiple  Set how many above work items will run in
                            parallell. [default = 16384]

  Examples:
    ./profanity2 --leading f -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --matching dead -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --matching badXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXbad -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --leading-range -m 0 -M 1 -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --leading-range -m 10 -M 12 -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --range -m 0 -M 1 -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --contract --leading 0 -z HEX_PUBLIC_KEY_128_CHARS_LONG
    ./profanity2 --contract --zero-bytes -z HEX_PUBLIC_KEY_128_CHARS_LONG

  About:
    profanity2 is a vanity address generator for Ethereum that utilizes
    computing power from GPUs using OpenCL.

  Forked "profanity2":
    Author: 1inch Network <info@1inch.io>
    Disclaimer:
      This project "profanity2" was forked from the original project and
      modified to guarantee "SAFETY BY DESIGN". This means source code of
      this project doesn't require any audits, but still guarantee safe usage.

  From original "profanity":
    Author: Johan Gustafsson <profanity@johgu.se>
    Beer donations: 0x000dead000ae1c8e8ac27103e4ff65f42a4e9203
    Disclaimer:
      Always verify that a private key generated by this program corresponds to
      the public key printed by importing it to a wallet of your choice. This
      program like any software might contain bugs and it does by design cut
      corners to improve overall performance.
```

### Benchmarks - Current version
|Model|Clock Speed|Memory Speed|Modified straps|Speed|Time to match eight characters
|:-:|:-:|:-:|:-:|:-:|:-:|
|GTX 1070 OC|1950|4450|NO|179.0 MH/s| ~24s
|GTX 1070|1750|4000|NO|163.0 MH/s| ~26s
|RX 480|1328|2000|YES|120.0 MH/s| ~36s
|RTX 4090|-|-|-|1096 MH/s| ~3s
|Apple Silicon M1<br/>(8-core GPU)|-|-|-|45.0 MH/s| ~97s
|Apple Silicon M1 Max<br/>(32-core GPU)|-|-|-|172.0 MH/s| ~25s
|Apple Silicon M3 Pro<br/>(18-core GPU)|-|-|-|97 MH/s| ~45s
|Apple Silicon M4 Max<br/>(40-core GPU)|-|-|-|350 MH/s| ~12s

