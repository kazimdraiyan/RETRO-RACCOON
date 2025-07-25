FLIPPED_HORIZONTALLY_FLAG  = 0x80000000;
FLIPPED_VERTICALLY_FLAG    = 0x40000000;
DOES_COLLIDE_FLAG = 0x10000000

level_metadata_file = open("D:\\Projects\\iGraphics\\platformer_game\\levels\\level1\\n.txt")
level_count = int(level_metadata_file.read())
level_metadata_file.close()
for layer in range(level_count):
    file = open(f"D:\\Projects\\iGraphics\\platformer_game\\levels\\level1\\{layer}_customized.csv", "r")
    content = file.read()
    lines = [line.split(",") for line in content.split("\n")[:-1]]
    int_lines = []
    for line in lines:
        int_line = []
        for cell in line:
            int_line.append(int(cell))
        int_lines.append(int_line)

    if (layer == 2):
        print(int_lines)

        row = 0
        for int_line in int_lines:
            print("Row:", row)
            for gid in int_line:
                if gid == -1:
                    print(gid)
                else:
                    flipped_horizontally = bool(gid & FLIPPED_HORIZONTALLY_FLAG);
                    flipped_vertically = bool(gid & FLIPPED_VERTICALLY_FLAG);
                    does_collide = bool(gid & DOES_COLLIDE_FLAG)
                    gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | DOES_COLLIDE_FLAG)
                    print(gid, flipped_horizontally, flipped_vertically, does_collide)
            print()
            row += 1

    file.close()
