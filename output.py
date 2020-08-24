import subprocess
import csv

data = []
with open("data_chart.csv", "w+") as output:
	subprocess.call(["python3", "./startupScript.py"], stdout=output);
	file_output = output.read().split('\n')

for row in file_output:
	data.append(row.split(','))

print(data)
