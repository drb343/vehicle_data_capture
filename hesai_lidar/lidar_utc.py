import pandas as pd

# Read the csv file and convert each timestamp to UTC
df = pd.read_csv('path_to_csv.csv')
df['utc_start_str'] = pd.to_datetime(df['frame_start_timestamp'], unit='s', utc=True).dt.strftime('%Y-%m-%d %H:%M:%S.%f')
df['utc_end_str']   = pd.to_datetime(df['frame_end_timestamp'],   unit='s', utc=True).dt.strftime('%Y-%m-%d %H:%M:%S.%f')

# Result is an updated file with timestamps in UTC
df.to_csv('timestamps_utc.csv', index=False)

# Print the first few frames for debugging purposes
print(df[['frame_index','utc_start_str','utc_end_str']].head(5))
