import math

def calculate_log2(number):
    if number <= 0:
        return "Number must be greater than 0."
    return math.log2(number)

# Example usage


Bandwidth = 100 #Gbps
Tdre = 50 #us
alpha = 0.2
bit_num = 8
ratio_div = Bandwidth * 1e9 * Tdre * (1e-6) / (alpha * 8)
print("ratio_div = {}".format(ratio_div))
CE_div = ratio_div / (2 ** bit_num - 1)
print("CE_div = {}".format(CE_div))
print
CE_div_bit = calculate_log2(CE_div)
print("CE_div_bit = {}".format(CE_div_bit))