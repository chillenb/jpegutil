import numpy as np

def cformatnbyn(matrix, name="table", tablen=4, n=8):
    """Format an nxn matrix of ints as a C array"""
    matrix2d = np.reshape(matrix, (n, n))
    length = int(np.floor(np.log10(np.amax(matrix2d)))) + 1
    maxentry_first_col = np.amax(matrix2d[:, 0])
    diff = length - (int(np.floor(np.log10(maxentry_first_col))) + 1)
    tab = " " * tablen
    it = np.nditer(matrix2d, flags=["multi_index"])
    strentries = [
        f"{x:{length - diff if it.multi_index[1] == 0 else length}}" for x in it
    ]
    rows = [strentries[i : i + n] for i in range(0, n*n, n)]
    body = (",\n" + tab).join([", ".join(r) for r in rows])
    return f"const int {name}[] = {{\n{tab}{body}}};"

def cformat8by8(matrix, name="table", tablen=4):
    """Format an 8x8 matrix of ints as a C array"""
    return cformatnbyn(matrix, name=name, tablen=tablen, n=8)

def zigzagind(n):
  for i in range(n*2):
    s = 0 if i < n else i - n + 1
    for j in range(s, min(i+1, n)):
      yield j * (n-1) + i if i % 2 != 0 else (i-j) * n + j

def zigzag(n):
    mat = np.zeros(n*n, dtype='int')
    for k, v in enumerate(zigzagind(n)):
      mat[v] = k
    return mat.reshape((n,n))

def zigzaginv(n):
  return np.array(list(zigzagind(n)))