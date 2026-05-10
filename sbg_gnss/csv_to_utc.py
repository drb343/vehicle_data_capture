import pandas as pd
import numpy as np
import os

# Fit internal microsecond to UTC mapping from anchor file
utc = pd.read_csv('csv_logs3/utcTime.csv', header=0, skiprows=[1])
utc['utc_unix_us'] = pd.to_datetime({
    'year': utc['year'], 'month': utc['month'], 'day': utc['day'],
    'hour': utc['hour'], 'minute': utc['minute'], 'second': utc['second'],
}).view('int64') // 1000
utc['internal_us'] = utc['timestamp'].astype('int64')

slope, intercept = np.polyfit(utc['internal_us'], utc['utc_unix_us'], 1)

def internal_to_utc(ts_us):
    return pd.to_datetime((slope * ts_us + intercept) * 1000, unit='ns', utc=True)

# Convert all CSVs and save to ./converted/
CSV_DIR = 'csv_logs3'

os.makedirs('converted', exist_ok=True)
for fname in [f for f in os.listdir(CSV_DIR) if f.endswith('.csv') and f != 'utcTime.csv']:
    try:
        df = pd.read_csv(f'{CSV_DIR}/{fname}', header=0, skiprows=[1])
        df['utc_timestamp'] = internal_to_utc(df['timestamp'].astype('int64'))
        df['utc_timestamp_str'] = df['utc_timestamp'].dt.strftime('%Y-%m-%d %H:%M:%S.%f')
        df.to_csv(f'converted/{fname}', index=False)
        print(f"✓ {fname}")
    except Exception as e:
        print(f"✗ {fname}: {e}")