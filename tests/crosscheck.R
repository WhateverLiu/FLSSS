# R side of the R-vs-Python cross-check.
#
# Usage: Rscript crosscheck.R <instance_file> <output_file>
#
# The instance file (written by crosscheck_r_py.py) is plain text:
#   line 1: nrow ncol len
#   line 2: lo   (ncol integers)
#   line 3: hi   (ncol integers)
#   next nrow lines: the integer matrix, one row per line
#
# FLSSS bounds a subset sum by (target, ME); a band [lo, hi] maps to
# target = (lo + hi) / 2 and ME = (hi - lo) / 2. We ask for far more
# solutions than exist and give a generous time limit so the solver
# enumerates the complete qualifying set. Output: one solution per line,
# sorted 1-based indices, comma-separated ("EMPTY" if none).
suppressMessages(library(FLSSS))

args <- commandArgs(trailingOnly = TRUE)
con <- file(args[1], "r")
hdr <- scan(con, what = integer(), n = 3, quiet = TRUE)
nrow <- hdr[1]; ncol <- hdr[2]; len <- hdr[3]
lo <- scan(con, what = double(), n = ncol, quiet = TRUE)
hi <- scan(con, what = double(), n = ncol, quiet = TRUE)
flat <- scan(con, what = double(), n = nrow * ncol, quiet = TRUE)
close(con)

X <- matrix(flat, nrow = nrow, ncol = ncol, byrow = TRUE)
target <- (lo + hi) / 2
ME <- (hi - lo) / 2

NEED <- 10000000L
TLIMIT <- 120

if (ncol == 1L) {
  # One-dimensional FLSSS expects a sorted vector, so sort and remember
  # the permutation to translate indices back to the original rows.
  v <- X[, 1]
  ord <- order(v)
  vs <- v[ord]
  rst <- FLSSS(len = len, v = vs, target = target, ME = ME,
               solutionNeed = NEED, tlimit = TLIMIT)
  rst <- lapply(rst, function(ix) ord[ix])
} else {
  rst <- mFLSSSpar(maxCore = 4L, len = len, mV = X,
                   mTarget = target, mME = ME,
                   solutionNeed = NEED, tlimit = TLIMIT,
                   dl = ncol, du = ncol, avgThreadLoad = 8L)
}

out <- file(args[2], "w")
if (length(rst) == 0) {
  writeLines("EMPTY", out)
} else {
  for (s in rst) writeLines(paste(sort(s), collapse = ","), out)
}
close(out)
