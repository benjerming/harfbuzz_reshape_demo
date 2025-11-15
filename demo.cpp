#include <cstring>
#include <filesystem>
#include <ft2build.h>
#include <hb-ft.h>
#include <hb.h>
#include <print>
#include <vector>
#include FT_FREETYPE_H

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// 图像缓冲区结构
struct Image {
  int width;
  int height;
  std::vector<unsigned char> data; // RGBA格式

  Image(int w, int h) : width(w), height(h), data(w * h * 4, 255) {}

  void set_pixel(int x, int y, unsigned char r, unsigned char g,
                 unsigned char b, unsigned char a) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      int idx = (y * width + x) * 4;
      data[idx] = r;
      data[idx + 1] = g;
      data[idx + 2] = b;
      data[idx + 3] = a;
    }
  }

  void blend_pixel(int x, int y, unsigned char gray) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      int idx = (y * width + x) * 4;
      // 简单的alpha混合 - 黑色文字在白色背景上
      float alpha = gray / 255.0f;
      data[idx] = static_cast<unsigned char>(255 * (1 - alpha));
      data[idx + 1] = static_cast<unsigned char>(255 * (1 - alpha));
      data[idx + 2] = static_cast<unsigned char>(255 * (1 - alpha));
      data[idx + 3] = 255;
    }
  }

  bool save_png(const char *filename) {
    return stbi_write_png(filename, width, height, 4, data.data(), width * 4) !=
           0;
  }
};

// 渲染指定数量的字形到图像（包含原始文字和整形后文字两行）
void render_glyphs(FT_Face ft_face, const char *text, hb_buffer_t *buf_shaped,
                   int glyph_count, const char *output_filename) {
  hb_glyph_info_t *glyph_info_shaped =
      hb_buffer_get_glyph_infos(buf_shaped, nullptr);
  hb_glyph_position_t *glyph_pos_shaped =
      hb_buffer_get_glyph_positions(buf_shaped, nullptr);

  // 创建图像 (增加高度以容纳两行文字)
  int img_width = 800;
  int img_height = 400;
  Image img(img_width, img_height);

  // 第一行：原始文字（不经过整形，直接用字符码点渲染）
  int baseline_y_original = 80;
  int pen_x = 50;
  int pen_y = baseline_y_original;

  // 解析UTF-8文本，获取Unicode码点
  const char *p = text;
  const char *end = text + strlen(text);
  int char_count = 0;

  while (p < end && char_count < glyph_count) {
    uint32_t codepoint;
    if ((*p & 0x80) == 0) {
      codepoint = *p;
      p += 1;
    } else if ((*p & 0xE0) == 0xC0) {
      codepoint = ((*p & 0x1F) << 6) | (*(p + 1) & 0x3F);
      p += 2;
    } else if ((*p & 0xF0) == 0xE0) {
      codepoint =
          ((*p & 0x0F) << 12) | ((*(p + 1) & 0x3F) << 6) | (*(p + 2) & 0x3F);
      p += 3;
    } else if ((*p & 0xF8) == 0xF0) {
      codepoint = ((*p & 0x07) << 18) | ((*(p + 1) & 0x3F) << 12) |
                  ((*(p + 2) & 0x3F) << 6) | (*(p + 3) & 0x3F);
      p += 4;
    } else {
      p += 1;
      continue;
    }

    // 跳过空格
    if (codepoint == 0x20) {
      pen_x += 20; // 简单的空格宽度
      char_count++;
      continue;
    }

    // 使用字符码点直接加载字形（不经过整形）
    FT_UInt glyph_index = FT_Get_Char_Index(ft_face, codepoint);
    if (FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_RENDER)) {
      char_count++;
      continue;
    }

    FT_GlyphSlot slot = ft_face->glyph;
    FT_Bitmap *bitmap = &slot->bitmap;

    int x_pos = pen_x + slot->bitmap_left;
    int y_pos = pen_y - slot->bitmap_top;

    // 绘制字形位图
    for (unsigned int by = 0; by < bitmap->rows; by++) {
      for (unsigned int bx = 0; bx < bitmap->width; bx++) {
        unsigned char pixel = bitmap->buffer[by * bitmap->pitch + bx];
        if (pixel > 0) {
          img.blend_pixel(x_pos + bx, y_pos + by, pixel);
        }
      }
    }

    pen_x += slot->advance.x / 64;
    pen_y += slot->advance.y / 64;
    char_count++;
  }

  // 第二行：整形后的文字（基线位置）
  int baseline_y_shaped = 280;
  pen_x = 50;
  pen_y = baseline_y_shaped;

  // 渲染第二行：整形后文字的前glyph_count个字形
  for (int i = 0; i < glyph_count; i++) {
    if (FT_Load_Glyph(ft_face, glyph_info_shaped[i].codepoint,
                      FT_LOAD_RENDER)) {
      continue;
    }

    FT_GlyphSlot slot = ft_face->glyph;
    FT_Bitmap *bitmap = &slot->bitmap;

    int x_pos = pen_x + slot->bitmap_left + (glyph_pos_shaped[i].x_offset / 64);
    int y_pos = pen_y - slot->bitmap_top + (glyph_pos_shaped[i].y_offset / 64);

    // 绘制字形位图
    for (unsigned int by = 0; by < bitmap->rows; by++) {
      for (unsigned int bx = 0; bx < bitmap->width; bx++) {
        unsigned char pixel = bitmap->buffer[by * bitmap->pitch + bx];
        if (pixel > 0) {
          img.blend_pixel(x_pos + bx, y_pos + by, pixel);
        }
      }
    }

    pen_x += glyph_pos_shaped[i].x_advance / 64;
    pen_y += glyph_pos_shaped[i].y_advance / 64;
  }

  // 添加标签文字
  // 这里可以添加"原始文字"和"整形后文字"的标签，但需要额外的字体渲染

  // 保存图像
  if (img.save_png(output_filename)) {
    std::println("✓ 保存图片: {} (包含{}个字形)", output_filename, glyph_count);
  } else {
    std::println("✗ 保存图片失败: {}", output_filename);
  }
}

void print_glyph_info(hb_buffer_t *buf) {
  unsigned int glyph_count;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos =
      hb_buffer_get_glyph_positions(buf, &glyph_count);

  std::println("\n=== Glyph信息 ===\n");
  std::println("总共 {} 个字形\n\n", glyph_count);

  int current_x = 0;
  for (unsigned int i = 0; i < glyph_count; i++) {
    std::println("字形 {}:", i);
    std::println("  Glyph ID: {}", glyph_info[i].codepoint);
    std::println("  Cluster: {}", glyph_info[i].cluster);
    std::println("  X偏移: {} px", glyph_pos[i].x_offset / 64.0);
    std::println("  Y偏移: {} px", glyph_pos[i].y_offset / 64.0);
    std::println("  X前进: {} px", glyph_pos[i].x_advance / 64.0);
    std::println("  Y前进: {} px", glyph_pos[i].y_advance / 64.0);
    std::println("  位置: {} px", current_x / 64.0);
    current_x += glyph_pos[i].x_advance;
    std::println("\n");
  }
}

int main(int argc, char **argv) {
  // 初始化FreeType
  FT_Library ft_library;
  FT_Face ft_face;

  if (FT_Init_FreeType(&ft_library)) {
    std::println("错误: 无法初始化FreeType库");
    return 1;
  }

  // 加载字体文件（需要一个支持阿拉伯语的字体）
  // 你可以替换为系统中的阿拉伯语字体路径
  const char *font_path =
      argc > 1 ? argv[1]
               : "/usr/share/fonts/google-noto-vf/NotoSansArabic[wght].ttf";

  if (FT_New_Face(ft_library, font_path, 0, &ft_face)) {
    std::println("错误: 无法加载字体: {}", font_path);
    std::println("使用方法: {} [字体文件路径] [自定义文本]", argv[0]);
    std::println("提示: 使用支持阿拉伯语的字体，如 Amiri、Scheherazade 或 Noto "
                 "Sans Arabic");
    FT_Done_FreeType(ft_library);
    return 1;
  }

  // 设置字体大小
  FT_Set_Char_Size(ft_face, 0, 48 * 64, 0, 0);

  // 创建HarfBuzz字体
  hb_font_t *hb_font = hb_ft_font_create(ft_face, NULL);

  // 阿拉伯语文本示例 "مرحبا" (Marhaba - 你好)
  // 这个词在连字时会有字形变化
  const char *text = "اللغة العربية مرحبا";
  std::println("测试文本: {} (阿拉伯语: Marhaba - 你好)", text);
  std::println("方向: RTL (从右到左)");

  // 计算原始文字个数（Unicode字符数）
  int original_char_count = 0;
  const char *p = text;
  const char *end = text + strlen(text);
  while (p < end) {
    if ((*p & 0x80) == 0) {
      p += 1;
    } else if ((*p & 0xE0) == 0xC0) {
      p += 2;
    } else if ((*p & 0xF0) == 0xE0) {
      p += 3;
    } else if ((*p & 0xF8) == 0xF0) {
      p += 4;
    } else {
      p += 1;
    }
    original_char_count++;
  }

  // 创建buffer：整形后的文字（完整shaping）
  hb_buffer_t *buf_shaped = hb_buffer_create();
  hb_buffer_set_direction(buf_shaped, HB_DIRECTION_RTL);
  hb_buffer_set_script(buf_shaped, HB_SCRIPT_ARABIC);
  hb_buffer_set_language(buf_shaped, hb_language_from_string("ar", -1));
  hb_buffer_add_utf8(buf_shaped, text, -1, 0, -1);

  // 执行完整shaping（包含连字处理）
  hb_shape(hb_font, buf_shaped, NULL, 0);

  // 获取整形后的字形个数
  unsigned int shaped_glyph_count;
  hb_buffer_get_glyph_infos(buf_shaped, &shaped_glyph_count);

  // 打印字符/字形个数
  std::println("\n=== 文字统计 ===");
  std::println("原始文字个数: {} 个字符", original_char_count);
  std::println("整形后字形个数: {} 个字形", shaped_glyph_count);
  std::println("说明: 阿拉伯语整形会将多个字符合并或分解为不同数量的字形\n");

  // 打印结果
  // print_glyph_info(buf_shaped);

  // 生成逐步的图片
  std::println("=== 生成逐步图片（两行对比）===\n");
  for (unsigned int i = 1; i <= shaped_glyph_count; i++) {
    const auto filename =
        std::filesystem::path("output") / std::format("step_{:02d}.png", i);
    std::filesystem::remove_all(filename.parent_path());
    std::filesystem::create_directories(filename.parent_path());
    render_glyphs(ft_face, text, buf_shaped, i, filename.string().c_str());
  }

  // 清理
  hb_buffer_destroy(buf_shaped);
  hb_font_destroy(hb_font);
  FT_Done_Face(ft_face);
  FT_Done_FreeType(ft_library);

  std::println("\n演示完成！");
  std::println(
      "注意: 第一行显示原始文字（不经过整形），第二行显示完整整形后的字形。");
  std::println(
      "在阿拉伯语中，字符的形状会根据其在单词中的位置而变化，并会进行连字。");

  return 0;
}
