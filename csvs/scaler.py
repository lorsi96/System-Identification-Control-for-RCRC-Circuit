import csv
import os
import sys

def scale_csv(csv_file, scaling_factor):
    # Calculate the reciprocal scaling factor
    reciprocal_factor = 1 / scaling_factor
    
    # Open the CSV file and read the data
    with open(csv_file, 'r') as f:
        reader = csv.reader(f)
        data = list(reader)
    
    # Extract the headers and data separately
    headers = data[0]
    data = data[1:]
    
    # Scale the data in each column using the reciprocal factor
    for i in range(len(data)):
        for j in range(1, len(data[i])):
            data[i][j] = float(data[i][j]) * reciprocal_factor
    
    # Rename the original file with a prefix
    os.rename(csv_file, 'old_' + csv_file)
    
    # Write the new scaled data to a new CSV file, including the headers
    with open(csv_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(headers)
        writer.writerows(data)
    
    print(f"The file '{csv_file}' has been reciprocally scaled by {scaling_factor} and saved to '{csv_file}'. The original file has been renamed to 'old_{csv_file}'.")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python scale_csv.py <csv_file_path> <scaling_factor>")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    scaling_factor = float(sys.argv[2])
    
    scale_csv(csv_file, scaling_factor)
