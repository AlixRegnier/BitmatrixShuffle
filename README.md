# BitmatrixShuffle

<i>Branch: <u>exp_marcus</u></i>

## Build

To build the recipe, you need to cd to same directory than  ``recipe.def``

```bash
apptainer build image.sif recipe.def
```

## Usage

To use the program, you just need to run this command

```bash
apptainer run <csv> <columns> <rows>
```

Arg|Description
--|--
csv|CSV input file
columns|Number of columns
rows|Number of rows

## Output

```bash
├── file.csv     #File CSV
├── file.col.txt #Column permutation
└── file.row.txt #Row permutation
```
