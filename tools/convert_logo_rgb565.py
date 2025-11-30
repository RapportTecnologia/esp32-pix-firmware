from pathlib import Path

from PIL import Image

# Caminhos baseados na estrutura atual do projeto
BASE_DIR = Path(__file__).resolve().parents[1]
IMG_DIR = BASE_DIR / "main" / "images"

SRC = IMG_DIR / "rapport-pix.png"
DST_H = IMG_DIR / "rapport_pix_logo.h"

MAX_WIDTH = 128  # largura do display ST7735


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    """Converte um pixel RGB888 (8 bits por canal) para RGB565 (16 bits)."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main() -> None:
    if not SRC.exists():
        raise SystemExit(f"Imagem de entrada nao encontrada: {SRC}")

    img = Image.open(SRC).convert("RGB")

    # Redimensiona mantendo proporcao para largura MAX_WIDTH
    w, h = img.size
    new_w = MAX_WIDTH
    new_h = int(h * (new_w / w))
    img = img.resize((new_w, new_h), Image.LANCZOS)

    pixels = list(img.getdata())

    DST_H.parent.mkdir(parents=True, exist_ok=True)

    with DST_H.open("w", encoding="utf-8") as f:
        f.write("// Generated from rapport-pix.png\n")
        f.write("#ifndef RAPPORT_PIX_LOGO_H\n#define RAPPORT_PIX_LOGO_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define RAPPORT_PIX_LOGO_WIDTH  {new_w}\n")
        f.write(f"#define RAPPORT_PIX_LOGO_HEIGHT {new_h}\n\n")
        f.write("static const uint16_t RAPPORT_PIX_LOGO_DATA[] = {\n")

        for i, (r, g, b) in enumerate(pixels):
            color = rgb888_to_rgb565(r, g, b)
            if i % 12 == 0:
                f.write("    ")
            f.write(f"0x{color:04X}, ")
            if i % 12 == 11:
                f.write("\n")

        f.write("\n};\n\n#endif // RAPPORT_PIX_LOGO_H\n")

    print(f"Logo convertido com sucesso para RGB565: {DST_H}")
    print(f"Dimensoes: {new_w}x{new_h}")


if __name__ == "__main__":
    main()
