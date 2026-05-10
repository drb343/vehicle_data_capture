import csv
from pathlib import Path

# Change file path to your choosing
input_dir = Path("csv_logs3") 

for txt_file in input_dir.glob("*.txt"):
    csv_file = txt_file.with_suffix(".csv")

    with open(txt_file, "r", encoding="utf-8", errors="ignore") as fin, \
         open(csv_file, "w", newline="", encoding="utf-8") as fout:
        writer = csv.writer(fout)

        for line in fin:
            line = line.strip()
            if not line:
                continue

            row = line.split()   # splits on spaces/tabs
            writer.writerow(row)

    txt_file.unlink()
    print(f"Converted {txt_file.name} -> {csv_file.name} and deleted {txt_file.name}")
