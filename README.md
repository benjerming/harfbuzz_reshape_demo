# HarfBuzz RTL 连字功能演示

这是一个使用 HarfBuzz 库测试 RTL（从右到左）语言连字功能的演示程序。

## 功能说明

本演示程序展示了：
- RTL（从右到左）文本的处理
- 阿拉伯语文本的连字（ligature）功能
- 字形（glyph）信息的详细输出
- 字符如何在 shaping 过程中转换为字形
- **逐字形渲染到图片**：将文本整形后，逐步生成包含1个、2个、3个...字形的PNG图片，方便通过滑动图片观看字形变化过程
- **两行对比显示**：每张图片分成两行
  - **第一行**：原始字形（禁用连字特性）
  - **第二行**：完整整形后的字形（包含连字和位置调整）
  - 这样可以直观地看出HarfBuzz整形处理的效果差异

## 构建

### 依赖项

**系统依赖：**

在 Fedora/RHEL 系统上：
```bash
sudo dnf install harfbuzz-devel freetype-devel cmake gcc-c++
```

在 Ubuntu/Debian 系统上：
```bash
sudo apt install libharfbuzz-dev libfreetype-dev cmake g++
```

**第三方库：**
- [stb_image_write.h](https://github.com/nothings/stb) - 用于PNG图片输出（已包含在项目中，无需额外安装）

### 编译

```bash
mkdir -p build
cd build
cmake ..
make
```

## 运行

### 使用默认字体和文本
```bash
./build/demo
```

### 指定字体文件
```bash
./build/demo /path/to/your/font.ttf
```

### 测试自定义文本（如"hello"）
```bash
./build/demo /path/to/your/font.ttf "hello"
```

### 示例

**测试阿拉伯语（使用默认字体）：**
```bash
./build/demo
```

**测试英文"hello"：**
```bash
./build/demo "/usr/share/fonts/google-droid-sans-fonts/DroidSans.ttf" "hello"
```

**测试其他自定义文本：**
```bash
./build/demo "/usr/share/fonts/google-droid-sans-fonts/DroidSans.ttf" "Typography"
```

### 推荐的字体

**阿拉伯语字体：**
- Noto Sans Arabic: `/usr/share/fonts/google-noto-vf/NotoSansArabic[wght].ttf`
- Amiri
- Scheherazade

**英文/拉丁字母字体：**
- Droid Sans: `/usr/share/fonts/google-droid-sans-fonts/DroidSans.ttf`
- Adwaita Sans: `/usr/share/fonts/adwaita-sans-fonts/AdwaitaSans-Regular.ttf`

## 输出说明

### 控制台输出

程序会输出每个字形的详细信息：

- **Glyph ID**: 字体中的字形标识符
- **Cluster**: 字符在原始文本中的字节位置
- **X偏移/Y偏移**: 字形的位置偏移
- **X前进/Y前进**: 光标前进的距离
- **位置**: 字形在文本行中的累计位置

### PNG 图片输出

程序会自动生成逐步的PNG图片，展示字形的累积过程。每张图片包含**两行文字**：
- **第一行**：原始字形（禁用了连字、上下文替换等特性）
- **第二行**：完整整形后的字形（经过HarfBuzz完整处理）

**测试文本（اللغة العربية مرحبا）：**
- `output_step_01.png` - 两行各包含第1个字形
- `output_step_02.png` - 两行各包含前2个字形
- `output_step_03.png` - 两行各包含前3个字形
- ...以此类推

通过对比两行，可以清晰地看到HarfBuzz整形处理的效果，特别是连字如何将多个字符组合成单个字形，以及字符形状如何根据其在单词中的位置而变化。

**其他文本示例（已注释）：**
- `output3_step_01.png` - 包含第1个字形（'h'）
- `output3_step_02.png` - 包含前2个字形（'he'）
- `output3_step_03.png` - 包含前3个字形（'hel'）
- `output3_step_04.png` - 包含前4个字形（'hell'）
- `output3_step_05.png` - 包含前5个字形（'hello'）

💡 **使用技巧**：在图片浏览器中左右滑动这些图片，观看字形如何逐步形成完整的文本。这对于理解连字（ligature）和字形替换过程非常有帮助！

## RTL 和连字说明

### RTL（从右到左）
阿拉伯语、希伯来语等语言从右向左书写。在 HarfBuzz 中：
- 使用 `HB_DIRECTION_RTL` 设置方向
- 字形顺序是从右到左的
- Cluster 值从大到小排列

### 连字（Ligature）
在阿拉伯语中，字符会根据在单词中的位置改变形状：
- **孤立形（Isolated）**: 字符单独出现
- **初始形（Initial）**: 字符在词首
- **中间形（Medial）**: 字符在词中
- **末尾形（Final）**: 字符在词尾

HarfBuzz 的 shaping 过程会自动处理这些变换。

## 代码关键点

### 创建两个buffer用于对比

```cpp
// 1. 创建第一个buffer（原始字形，禁用连字）
hb_buffer_t *buf_original = hb_buffer_create();
hb_buffer_set_direction(buf_original, HB_DIRECTION_RTL);
hb_buffer_set_script(buf_original, HB_SCRIPT_ARABIC);
hb_buffer_set_language(buf_original, hb_language_from_string("ar", -1));
hb_buffer_add_utf8(buf_original, text, -1, 0, -1);

// 禁用连字等特性
hb_feature_t features_original[] = {
  {HB_TAG('l','i','g','a'), 0, 0, static_cast<unsigned int>(-1)},  // 禁用标准连字
  {HB_TAG('c','a','l','t'), 0, 0, static_cast<unsigned int>(-1)},  // 禁用上下文替换
  {HB_TAG('r','l','i','g'), 0, 0, static_cast<unsigned int>(-1)},  // 禁用必需连字
};
hb_shape(hb_font, buf_original, features_original, 3);

// 2. 创建第二个buffer（完整整形）
hb_buffer_t *buf_shaped = hb_buffer_create();
hb_buffer_set_direction(buf_shaped, HB_DIRECTION_RTL);
hb_buffer_set_script(buf_shaped, HB_SCRIPT_ARABIC);
hb_buffer_set_language(buf_shaped, hb_language_from_string("ar", -1));
hb_buffer_add_utf8(buf_shaped, text, -1, 0, -1);

// 执行完整shaping（包含连字处理）
hb_shape(hb_font, buf_shaped, NULL, 0);

// 3. 获取结果并渲染
hb_glyph_info_t *glyph_info_original = hb_buffer_get_glyph_infos(buf_original, nullptr);
hb_glyph_info_t *glyph_info_shaped = hb_buffer_get_glyph_infos(buf_shaped, &glyph_count);
// 渲染两行对比
```

## 测试文本

程序当前测试阿拉伯语文本：
- **اللغة العربية مرحبا** - 包含"阿拉伯语言"和"你好"两个词

代码中还包含其他测试文本的示例（已注释），可以取消注释来测试其他文本。

## 扩展建议

你可以修改代码来测试：
- 其他 RTL 语言（如希伯来语）
- 不同的字体特性（OpenType features）
- 自定义的文本输入
- 更复杂的连字规则

## 参考资源

- [HarfBuzz 官方文档](https://harfbuzz.github.io/)
- [Unicode 双向文本算法](https://unicode.org/reports/tr9/)
- [OpenType 规范](https://docs.microsoft.com/en-us/typography/opentype/spec/)

