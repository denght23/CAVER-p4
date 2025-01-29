def create_32bit_value(high16, mid8, low8):
    return (high16 << 16) | (mid8 << 8) | low8

def extract_values_from_32bit(value):
    high16 = (value >> 16) & 0xFFFF
    mid8 = (value >> 8) & 0xFF
    low8 = value & 0xFF
    return high16, mid8, low8

if __name__ == "__main__":
    list = ["10.1.0.1", "0", "2", "4"]
    pipe_0_list = [
        ["10.1.0.1", "0", "2", "4"],
        ["10.1.0.2", "0", "3", "5"],
        ["10.1.0.3", "0", "2", "6"],
        ["10.1.0.4", "0", "3", "7"]
    ]
    pipe_1_list = [
        ["10.0.0.1", "0", "0", "4"],
        ["10.0.0.2", "0", "1", "5"],
        ["10.0.0.3", "0", "0", "6"],
        ["10.0.0.4", "0", "1", "7"]
    ]
    for list in pipe_0_list:
        print(list[0], create_32bit_value(0, int(list[2]), int(list[3])))
    for list in pipe_1_list:
        print(list[0], create_32bit_value(0, int(list[2]), int(list[3])))
        
    # print(create_32bit_value(60, 0, 1))