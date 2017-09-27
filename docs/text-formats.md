# Text formats used by the desktop executable

## Ground truth text format

The file begins with the integers `W`, `H`, `F`, `L` on a separate line. These denote: width, height, number of frames, and the number of non-empty frames.

Following are `L` lines with the following format: integer `I`, `1 <= I <= F`, specifies the frame number that is being described on this line. Integer `N`, `1 <= N <= W*H`, specifies the number of pixels that will be listed. The following `N` integers are pixel indices in range `1` to `W*H` inclusive. Empty frames are not listed.

Both frame numbers and pixel indices use MATLAB's one-based indexing. The following conversions apply:
```
x = (i - 1) div H
y = (i - 1) mod H
```
where `i` is the one-based index, and `{x,y}` are zero-based coordinates.

## Evaluation text format

Evaluation data starts with the string /FMO/EVALUATION/V2/ on a separate line. Everything before this line is ignored. On the next line there is the integer `N`, specifying the number of sequences.

The following pattern of 5 lines is repeated `N` times. The first line contains a sequence name, and the integer `F`, denoting the number of frames in the sequence. The sequence name must be stripped of any directories or extensions (remove _gt, .mat, .txt, .avi, .mp4, .mov, etc.) and spaces must be replaced with underscores. The next four lines contain information about FN, FP, TN, TP, respectively and strictly in this order. Each of these lines contains the name of the result (FN, FP, TN or TP) followed by `F` integers separated by spaces, specifying the number of the particular kind of result in each frame.

Example file:
```
/FMO/EVALUATION/V2/
2
example 5
FN 0 0 0 0 4
FP 0 0 2 0 0
TN 1 1 0 0 1
TP 0 0 0 1 0
example_2 2
FN 0 0
FP 0 0
TN 1 0
TP 0 1
```
