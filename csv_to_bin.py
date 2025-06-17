import sys

def binarize_symbol(i):
    try:
        x = int(i)
        if x <= 0:
            return "0"
        elif int(i) >= 1:
            return "1"
    except ValueError:
        raise ValueError(f"Unknown symbol: '{i}'")

def main():
    if len(sys.argv) != 5:
        print("Usage: csv2bin <input.csv> <columns> <rows> <output.bin>\n\ninput.csv\tCSV file to binarize\ncolumns\t\tNumber of columns\nrows\t\tNumber of rows\noutput.bin\tBinarized matrix\n")
        exit(1)

    input_path = sys.argv[1]
    samples = int(sys.argv[2])
    rows = int(sys.argv[3])
    output_path = sys.argv[4]

    ROW_LENGTH = (samples+7)//8
    COLUMNS = ROW_LENGTH*8
    NB_ROWS = rows
    binary = bytearray(ROW_LENGTH*NB_ROWS)
    print(f"Columns:\t{COLUMNS}\nRows:\t\t{NB_ROWS}\t\nOutput size:\t{ROW_LENGTH*NB_ROWS} bytes")
    print("Running..")

    with open(input_path, "r") as f:
        a = f.readline() #ignore first row with headers

        for row in range(NB_ROWS):

            #Replace each symbol by 0 or 1 (c.f function binarize_symbol())
            a = "".join(map(binarize_symbol, f.readline().rstrip().split(',')))

            #Remove first row (header row)
            a = a[1:]

            if len(a) != samples:
                raise Exception(f"Unexpected row size (row {row} is {len(a)} instead of {samples})")

            #Add possible missing zeroes
            a += "0"*(COLUMNS - len(a))

            for column in range(0, COLUMNS, 8):
                binary[ROW_LENGTH*row + column//8] = int(a[column:column+8], 2)

    with open(output_path, "wb") as f:
        f.write(binary)

    print("Done!")


if __name__ == "__main__":
    main()
