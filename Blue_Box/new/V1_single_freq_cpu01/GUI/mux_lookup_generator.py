import pandas as pd
import numpy as np

# Lookup table for exc_out, sns_ap_out, sns_an_out, and ret_out as requested.
lookup_table = []

for exc_out in range(16):
    ret_out = (exc_out + 8) % 16  # ret_out opposite of exc_out
    for i in range(16):
        sns_ap_out = i  # sns_ap_out starts from zero if not conflicts
        sns_an_out = (sns_ap_out + 1) % 16  # sns_an_out follows sns_ap_out
        # Ensure no overlap between exc_out, sns_ap_out, sns_an_out, and ret_out
        if sns_ap_out != exc_out and sns_ap_out != ret_out and sns_an_out != exc_out and sns_an_out != ret_out:
            lookup_table.append([exc_out, sns_ap_out, sns_an_out, ret_out])

# Print the table in C format
print("const MuxConfig tomo_mux_lookup_table[NUM_CONFIGS] = {")
for row in lookup_table:
    print(f"    {{{row[0]}, {row[1]}, {row[2]}, {row[3]}}},")
print("};")

input()