"""Calculate best DCT weights for lossless embedding"""
import sys
import argparse
import numpy as np
from math import floor
from scipy.fft import dctn, idctn, dct
import cvxpy as cp

from utils import cformat8by8

# import matplotlib.pyplot as plt


def dct2(array):
    """2D discrete cosine transform"""
    return dctn(array, type=2, norm="ortho")


def idct2(array):
    """2D inverse discrete cosine transform"""
    return idctn(array, type=2, norm="ortho")


def compute_dct_matrix():
    """
    Matrix representation of 2D DCT.
    Take the transpose if you need its inverse
    """
    dct_1d = dct(np.eye(8), axis=0, norm="ortho")
    return np.kron(dct_1d, dct_1d)


default_luminance_qtable = np.array(
    [
        [16, 11, 10, 16, 24, 40, 51, 61],
        [12, 12, 14, 19, 26, 58, 60, 55],
        [14, 13, 16, 24, 40, 57, 69, 56],
        [14, 17, 22, 29, 51, 87, 80, 62],
        [18, 22, 37, 56, 68, 109, 103, 77],
        [24, 35, 55, 64, 81, 104, 113, 92],
        [49, 64, 78, 87, 103, 121, 120, 101],
        [72, 92, 95, 98, 112, 100, 103, 99],
    ],
    dtype=int,
)

mms_luminance_table = np.array(
    [
       [ 2,  2,  2,  3,  5,  6,  8, 10],
       [ 2,  2,  2,  3,  5,  6,  8, 10],
       [ 2,  2,  3,  5,  6,  8, 10, 12],
       [ 3,  3,  5,  6,  8, 10, 12, 14],
       [ 5,  5,  6,  8, 10, 12, 14, 15],
       [ 6,  6,  8, 10, 12, 14, 15, 15],
       [ 8,  8, 10, 12, 14, 15, 15, 15],
       [10, 10, 12, 14, 15, 15, 15, 15]
    ],
    dtype=int,
)


def solve_exact(quant_table, verbose=False, relgap=0.0002):
    """Find best weights using mixed-integer conic optimization"""
    try:
        import mosek
    except ImportError as err:
        msg = "You need MOSEK to solve the integer program"
        raise ValueError(msg) from err

    qtable_flat = quant_table.flatten()

    # normalization
    alphas = np.ones(8) * (2 ** (0.5))
    alphas[0] = 2 ** (-0.5)
    alpha_2d_matrix = np.outer(alphas, alphas)
    # dctmat = compute_dct_matrix()

    prog_coef = qtable_flat * alpha_2d_matrix.flatten()
    weights = cp.Variable(64, integer=True)

    constraints = [weights >= 0]
    constraints += [prog_coef @ weights.T / 8 <= 127]
    constraints += [prog_coef @ weights.T / 8 >= -128]

    prob = cp.Problem(
        cp.Maximize(cp.sum(cp.log(2 * weights + 1) / np.log(2))), constraints
    )

    mparams = {mosek.dparam.mio_tol_rel_gap: relgap}
    prob.solve(solver=cp.MOSEK, mosek_params=mparams, verbose=verbose)
    best_weights = np.array(np.round(weights.value.reshape((8, 8))), dtype="int")
    print(f"{prob.solution.opt_val} bits per 8x8 block", file=sys.stderr)
    print(f"Max luminance is {128 + prog_coef @ weights.value.T / 8}")
    return best_weights


def solve_approx(quant_table, verbose=False):
    """
    Find approximate best weights by removing integrality constraint
    from the original problem (see solve_exact).
    """
    qtable_flat = quant_table.flatten()

    # normalization
    alphas = np.ones(8) * (2 ** (0.5))
    alphas[0] = 2 ** (-0.5)
    alpha_2d_matrix = np.outer(alphas, alphas)
    # dctmat = compute_dct_matrix()

    prog_coef = qtable_flat * alpha_2d_matrix.flatten()
    weights = cp.Variable(64)

    constraints = [weights >= 0]
    constraints += [prog_coef @ weights.T / 8 <= 127]
    constraints += [prog_coef @ weights.T / 8 >= -128]

    prob = cp.Problem(
        cp.Maximize(cp.sum(cp.log(2 * weights + 1) / np.log(2))), constraints
    )
    prob.solve(verbose=verbose)
    best_weights_integer = np.array(
        np.floor(weights.value.reshape((8, 8))), dtype="int"
    )
    bits = np.sum(np.log(2 * best_weights_integer + 1) / np.log(2))
    print(f"{bits} bits per 8x8 block", file=sys.stderr)
    print(f"Max luminance is {128 + prog_coef @ best_weights_integer.flatten() / 8}")
    return best_weights_integer

def solve_pow2greedy(quant_table, verbose=False):
    qtable_flat = quant_table.flatten()

    # normalization
    alphas = np.ones(8) * (2 ** (0.5))
    alphas[0] = 2 ** (-0.5)
    alpha_2d_matrix = np.outer(alphas, alphas)
    # dctmat = compute_dct_matrix()

    prog_coef = qtable_flat * alpha_2d_matrix.flatten()
    unused = set(range(64))
    wtable = np.zeros(64, dtype=int)
    upper_budget = 127.0
    lower_budget = 128.0
    capacity = 0
    done = False
    while not done:
        maxE=0
        best_ind = -1
        best_bits = 0
        for ind in unused:
            up_pow2 = floor(np.log2(upper_budget / (8*prog_coef[ind]) + 1))
            lo_pow2 = floor(np.log2(lower_budget * (8*prog_coef[ind])))
            bits = min(up_pow2, lo_pow2)
            efficiency = bits / (prog_coef[ind] * (2**bits))
            if efficiency > maxE:
                maxE = efficiency
                best_ind = ind
                best_bits = bits
        if best_ind >= 0:
            upper_budget -= floor(np.log2(upper_budget / (8*prog_coef[best_ind]) + 1))
            lower_budget -= floor(np.log2(lower_budget / (8*prog_coef[best_ind])))
            if upper_budget < 0 or lower_budget < 0:
                break
            wtable[best_ind] = 2**best_bits
            unused.remove(best_ind)
            capacity += best_bits
        else:
            done = True
    print(f"{capacity} bits per block")
    print(f"{prog_coef @ wtable.T}")
    return wtable.reshape((8,8))

def jpeg_linqual(quality):
    """
    See int jpeg_quality_scaling(int quality)
    in libjpeg-turbo source (file ijg/jcparam.c).
    Convert quality rating to percentage scaling factor
    used to scale the quantization table.
    """
    quality = max(quality, 1)
    quality = min(quality, 100)
    if quality < 50:
        quality = 5000 // quality
    else:
        quality = 200 - quality * 2
    return quality


def jpeg_qtable(linear_quality):
    """Compute luminance quantization table for a given quality"""
    temp = np.floor((default_luminance_qtable * linear_quality + 50) / 100)
    temp[temp <= 0] = 1
    temp[temp > 255] = 255
    return np.array(temp, dtype="int")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Calculate best DCT weights for lossless embedding"
    )
    parser.add_argument('--exact', action='store_true', default=False)
    parser.add_argument("--approx", action="store_true", default=False)
    parser.add_argument('--pow2greedy', action="store_true", default=False)
    parser.add_argument("--cformat", action="store_true", default=False)
    parser.add_argument(
        "--zigzag",
        action="store_true",
        default=False,
        help="Print matrix in zigzag format",
    )
    parser.add_argument("--quality", type=int, dest="quality", default=50)
    parser.add_argument("--mmsquality", action="store_true", default=False)
    parser.add_argument("--relgap", type=float, default=0.0002)
    parser.add_argument(
        "--printqtable",
        action="store_true",
        default=False,
        help="Print quantization table",
    )
    parser.add_argument("--verbose", action="store_true", default=False)
    args = parser.parse_args()

    linqual = jpeg_linqual(args.quality)

    qtable = jpeg_qtable(linqual)
    if args.mmsquality:
      qtable = mms_luminance_table

    if args.pow2greedy:
        ans = solve_pow2greedy(qtable, verbose=args.verbose)

    if args.exact:
        ans = solve_exact(qtable, verbose=args.verbose, relgap=args.relgap)
    if args.approx:
        ans = solve_approx(qtable, verbose=args.verbose)

    if args.cformat:
        print(cformat8by8(ans, "weight_table_" + str(args.quality)))
    else:
        print(ans)

    if args.printqtable:
        if args.cformat:
            print(cformat8by8(qtable, "qtable_" + str(args.quality)))
        else:
            print(qtable)
