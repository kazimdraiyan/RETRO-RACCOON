small = [1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 36, 40, 45, 48, 60, 72, 80, 90, 120, 144, 180, 240, 360, 720] # 720 divisors
big = [1, 2, 4, 5, 8, 10, 16, 20, 32, 40, 64, 80, 128, 160, 256, 320, 640, 1280] # 1280 divisors

for small_num in small:
    eqv_big_num = small_num / 9 * 16
    if (eqv_big_num in big):
        print(small_num, int(eqv_big_num))