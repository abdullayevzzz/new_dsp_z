import pandas as pd

# Load the CSV file
file_path = "1k_res_freq_sweep_raw.csv"
df = pd.read_csv(file_path)

# Select relevant columns
df_relevant = df[['BIOZ1', 'EXC_FREQ_KHZ']]

# Group by frequency and calculate statistics for the first 100 values of each frequency
result = []
for freq, group in df_relevant.groupby('EXC_FREQ_KHZ'):
    first_100 = group.head(100)
    avg = first_100['BIOZ1'].mean()
    min_val = first_100['BIOZ1'].min()
    max_val = first_100['BIOZ1'].max()
    std_dev = first_100['BIOZ1'].std()
    result.append({'EXC_FREQ_KHZ': freq, 'mean': avg, 'min': min_val, 'max': max_val, 'std': std_dev})

# Convert the result to a DataFrame
statistics_df = pd.DataFrame(result)
print(statistics_df)

input('')
