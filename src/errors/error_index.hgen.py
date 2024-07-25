import json
index_f = open("error_index.json")
index = json.load(index_f)
index_f.close()

print("Building error index!")

output = open("error_index.h", "w+")
output.write("// Error index header\n")
output.write("// Auto generated! Don't edit!\n\n")

for error in index:
    if error[0] == "$":
        output.write("\n// " + error[1:] + "\n\n")
        continue

    output.write("#define " + index[error][0] + " (\"" + error + "\") // " + index[error][1] + "\n")
