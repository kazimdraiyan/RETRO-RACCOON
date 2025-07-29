collider_ids = """0-6
9-15
20-26
28-32
40-43
47-52
60-63
69-69
71-72
80-83
89-95
100-106
109-109
113-115
120-123
130-130
132-135
140-143
150-150"""
collider_id_ranges = collider_ids.split("\n")
collider_id_list = []
for ran in collider_id_ranges:
    for i in range(int(ran.split("-")[0]), int(ran.split("-")[1]) + 1):
        collider_id_list.append(i)

FLIPPED_HORIZONTALLY_FLAG = 0x80000000
FLIPPED_VERTICALLY_FLAG = 0x40000000
DOES_COLLIDE_FLAG = 0x10000000

for level in range(1, 6):
    try:
        level_metadata_file = open(f"levels/level{level}/metadata.txt")
        level_count = int(level_metadata_file.read().split("\n")[0])
        level_metadata_file.close()

        for layer in range(level_count):
            file = open(f"levels/level{level}/layer_{layer}.csv", "r")
            content = file.read()
            lines = [line.split(",") for line in content.split("\n")[:-1]]

            int_lines = []
            for line in lines:
                int_line = []
                for cell in line:
                    int_line.append(int(cell))
                int_lines.append(int_line)

            def does_collide(gid):
                return gid in collider_id_list

            new_lines = []
            for int_line in int_lines:
                new_line = []
                for raw_gid in int_line:
                    if raw_gid == -1:
                        new_line.append(str(raw_gid))
                        continue

                    # Decode current GID
                    flipped_horizontally = bool(
                        raw_gid & FLIPPED_HORIZONTALLY_FLAG)
                    flipped_vertically = bool(raw_gid & FLIPPED_VERTICALLY_FLAG)
                    base_gid = raw_gid & ~(
                        FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | DOES_COLLIDE_FLAG)

                    # Now re-encode
                    new_gid = base_gid
                    if flipped_horizontally:
                        new_gid |= FLIPPED_HORIZONTALLY_FLAG
                    if flipped_vertically:
                        new_gid |= FLIPPED_VERTICALLY_FLAG
                    if does_collide(base_gid):
                        new_gid |= DOES_COLLIDE_FLAG

                    new_line.append(str(new_gid))
                new_lines.append(",".join(new_line))

            # Save back to file
            with open(f"levels/level{level}/layer_{layer}_customized.csv", "w") as f:
                for line in new_lines:
                    f.write(line + "\n")

            file.close()

        print(f"Level {level} Encoding Successful")
    except FileNotFoundError:
        print(f"Level {level} not found")