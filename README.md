# BitmatrixShuffle

## Installing  

### Cloning repo

Make sure to clone this repo using following command:
```bash
git clone https://github.com/AlixRegnier/BitmatrixShuffle.git --branch kmindex --recursive
```

### Requirements

You need to have GNU C++ compiler supporting C++17

### Compiling

```bash
make
```

This will create two binary files named ``bitmatrixshuffle`` and ``reverse_bitmatrixshuffle``.

## Usage

### bitmatrixshuffle

This command will reorder ``kmindex`` binary $k$-mer matrices columns in a more compressive order. Then register the order in ``kmtricks.fof`` and will modify corresponding ``index.json`` entries. It will create in subindex directory a file called ``order.bin`` that is a serialized binary file telling the permutation applied.
```bash
bitmatrixshuffle -i <index_path> [-n <index_name>] [-g <groupsize>] [-s <subsampled_rows>]
```

***Arguments***  

Short name|Long name|Value type|Description
:--:|:--:|:--:|:--
-i|--index|STR|Index path (directory containing symbolic links and index.json)
-g|--groupsize|INT|Size of path TSP instances to reorder columns
-n|--index-name|STR|Name of subindex<br/>**Default**: First registered subindex
-s|--subsampled-rows|INT|Number of subsampled rows to compute a distance<br/>**Default**: 20,000

### reverse_bitmatrixshuffle
This command will reverse the operations made by ``bitmatrixshuffle`` command in order to retrieve original files.

```bash
reverse_bitmatrixshuffle -i <index_path> [-n <index_name>]
```

***Arguments***  

Short name|Long name|Value type|Description
:--:|:--:|:--:|:--
-i|--index|STR|Index path (directory containing symbolic links and index.json)
-n|--index-name|STR|Name of subindex<br/>**Default**: First registered subindex
