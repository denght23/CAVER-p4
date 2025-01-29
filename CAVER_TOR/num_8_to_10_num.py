while True:
    hex_number = input("请输入一个十六进制数（以0x开头）：")

    try:
        # 转换为十进制数
        decimal_number = int(hex_number, 16)
        print("转换为十进制数是{}".format(decimal_number))
    except ValueError:
        print("输入的不是有效的十六进制数，请检查格式。")