import os
from PIL import Image, ImageDraw, ImageFont

def get_font_paths():
    win_font_dir = r"C:\Windows\Fonts"
    font_map = {
        "Times_Regular": os.path.join(win_font_dir, "times.ttf"),
        "Times_Bold": os.path.join(win_font_dir, "timesbd.ttf"),
        "Arial_Regular": os.path.join(win_font_dir, "arial.ttf"),
        "Arial_Bold": os.path.join(win_font_dir, "arialbd.ttf"),
        "Sans_Modern_Regular": os.path.join(win_font_dir, "segoeui.ttf"), 
        "Sans_Modern_Bold": os.path.join(win_font_dir, "segoeuib.ttf")   
    }
    current_dir = os.path.dirname(os.path.abspath(__file__))
    for file in os.listdir(current_dir):
        if file.lower().endswith('.ttf'):
            name = os.path.splitext(file)[0]
            font_map[f"Custom_{name}"] = os.path.join(current_dir, file)
    return font_map

def generate_synthetic_data():
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    OUTPUT_DIR = os.path.join(BASE_DIR, 'Character_Sample')
    
    canvas_size = (224, 224)
    text_color = (0, 0, 0) # 纯黑字
    
    # 【升级点】融入网页数独最常见的几种背景色（白色、选中蓝、提示黄、阴影灰、通过绿）
    bg_colors = [
        (255, 255, 255),  # 纯白
        (220, 235, 255),  # 浅蓝
        (255, 255, 210),  # 浅黄
        (242, 242, 242),  # 浅灰
        (230, 245, 230)   # 浅绿
    ]
    
    font_paths = get_font_paths()
    font_sizes = [110, 125, 140]  # 优化字号跨度
    pixel_offsets = [-4, 0, 4]    # 保持微调

    generated_count = 0
    print("\n正在生成抗背景色干扰的高鲁棒性数字数据集...")
    
    for digit in range(1, 10):
        digit_str = str(digit)
        digit_dir = os.path.join(OUTPUT_DIR, digit_str)
        if not os.path.exists(digit_dir): os.makedirs(digit_dir)
            
        for font_name, font_path in font_paths.items():
            if not os.path.exists(font_path): continue
            for size in font_sizes:
                try:
                    font = ImageFont.truetype(font_path, size)
                except:
                    continue
                for bg in bg_colors: # 循环渲染不同的背景色
                    for ox in pixel_offsets:
                        for oy in pixel_offsets:
                            img = Image.new('RGB', canvas_size, bg)
                            draw = ImageDraw.Draw(img)
                            
                            bbox = draw.textbbox((0, 0), digit_str, font=font)
                            text_w = bbox[2] - bbox[0]
                            text_h = bbox[3] - bbox[1]
                            
                            x = (canvas_size[0] - text_w) // 2 - bbox[0] + ox
                            y = (canvas_size[1] - text_h) // 2 - bbox[1] + oy
                            
                            draw.text((x, y), digit_str, fill=text_color, font=font)
                            
                            # 文件名打上背景色标记防止覆盖
                            file_name = f"{font_name}_sz{size}_bg{bg[0]}_ox{ox}_oy{oy}.png"
                            img.save(os.path.join(digit_dir, file_name))
                            generated_count += 1

    print(f"\n【合成完毕】🎉 数据集已更新！共生成 {generated_count} 张多色背景数字图片。")

if __name__ == "__main__":
    generate_synthetic_data()