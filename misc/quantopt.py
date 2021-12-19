import numpy as np
import scipy.fft
import matplotlib.pyplot as plt
import cvxpy as cp
import argparse
import sys

def dct2(a):
    return scipy.fft.dctn(a, type=2, norm='ortho')
def idct2(a):
    return scipy.fft.idctn(a, type=2, norm='ortho')

def compute_dct_matrix(): # take transpose to get idct
  d0 = scipy.fft.dct(np.eye(8), axis=0, norm='ortho')
  return np.kron(d0,d0)


default_luminance_qtable = np.array([[16, 11, 10, 16, 24, 40, 51, 61],
[12, 12, 14, 19, 26, 58, 60, 55],
[14, 13, 16, 24, 40, 57, 69, 56],
[14, 17, 22, 29, 51, 87, 80, 62],
[18, 22, 37, 56, 68, 109, 103, 77],
[24, 35, 55, 64, 81, 104, 113, 92],
[49, 64, 78, 87, 103, 121, 120, 101],
[72, 92, 95, 98, 112, 100, 103, 99]], dtype=int)


def cformat8by8(matrix, name='table'):
  strentries = [str(v) for v in matrix.flatten()]
  rows = [strentries[i:i+8] for i in range(0, 64, 8)]
  body = ',\n    '.join([', '.join(r) for r in rows])
  return '{} {}[] = {{\n    {}}};'.format("const int", name, body)

def solve_approx(qtable, verbose=False):
  n = 8
  qf = qtable.flatten()

  # normalization
  alphas = np.ones(8) * (2**(0.5))
  alphas[0] = 2**(-0.5)
  alphamat = np.outer(alphas,alphas)
  dctmat = compute_dct_matrix()

  lcoef = qf * alphamat.flatten()
  w = cp.Variable(n*n)

  Constraints = [w >= 0]
  Constraints += [lcoef @ w.T / 8 <= 127]
  Constraints += [lcoef @ w.T / 8  >= -128]

  prob = cp.Problem(cp.Maximize(cp.sum(cp.log(2*w+1) / np.log(2))),
          Constraints)
  prob.solve(verbose=verbose)
  val = w.value
  wv = np.floor(w.value.reshape((8,8)))
  wv = np.array(wv, dtype='int')
  bits = np.sum(np.log(2*wv+1) / np.log(2))
  print(f"{bits} bits per 8x8 block", file=sys.stderr)
  print(f"Max luminance is {128 + lcoef @ wv.flatten() / 8}")
  return wv

def solve_exact(qtable, verbose=False):
  import mosek
  n = 8
  qf = qtable.flatten()

  # normalization
  alphas = np.ones(8) * (2**(0.5))
  alphas[0] = 2**(-0.5)
  alphamat = np.outer(alphas,alphas)
  dctmat = compute_dct_matrix()

  lcoef = qf * alphamat.flatten()
  w = cp.Variable(n*n, integer=True)

  Constraints = [w >= 0]
  Constraints += [lcoef @ w.T / 8 <= 127]
  Constraints += [lcoef @ w.T / 8  >= -128]

  prob = cp.Problem(cp.Maximize(cp.sum(cp.log(2*w+1) / np.log(2))),
          Constraints)


  mparams = {mosek.dparam.mio_tol_rel_gap: 0.0002}
  prob.solve(solver=cp.MOSEK, mosek_params=mparams, verbose=verbose)
  val = w.value
  wv = np.round(w.value.reshape((8,8)))
  wv = np.array(wv, dtype='int')
  print(f"{prob.solution.opt_val} bits per 8x8 block", file=sys.stderr)
  print(f"Max luminance is {128 + lcoef @ val.T / 8}")
  return wv

def jpeg_linqual(quality): # see turbojpeg ijg/jcparam.c
  if(quality <= 0):
    quality = 1
  if(quality > 100):
    quality = 100
  if(quality < 50):
    quality = 5000 // quality
  else:
    quality = 200 - quality*2
  return quality

def jpeg_qtable(linqual):
  temp = np.floor((default_luminance_qtable * linqual + 50) / 100)
  temp[temp <= 0] = 1
  temp[temp > 255] = 255
  return np.array(temp, dtype='int')

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Calculate best DCT weights for lossless embedding')
  parser.add_argument('--approx', action='store_true', default=False)
  parser.add_argument('--cformat', action='store_true', default=False)
  parser.add_argument('--zigzag', action='store_true', default=False, help="Print matrix in zigzag format")
  parser.add_argument('--quality', type=int, dest='quality', default=50)
  parser.add_argument('--printqtable', action='store_true', default=False, help="Print quantization table")
  parser.add_argument('--verbose', action='store_true', default=False)
  args = parser.parse_args()

  linqual = jpeg_linqual(args.quality)

  
  qtable = jpeg_qtable(linqual)

  if not args.approx:
    ans = solve_exact(qtable, verbose=args.verbose)
  else:
    ans = solve_approx(qtable, verbose=args.verbose)
  

  if args.cformat:
    print(cformat8by8(ans, 'weight_table_'+str(args.quality)))
  else:
    print(ans)

  if args.printqtable:
    if args.cformat:
      print(cformat8by8(qtable, 'qtable_'+str(args.quality)))
    else:
      print(qtable)
